#include "../../AspectRatio.hpp"

#include <cmath>
#include <cstdio>
#include <string>

static int g_failures = 0;

static void Expect(bool condition, const std::string& name) {
	if (condition) {
		std::printf("  PASS  %s\n", name.c_str());
	}
	else {
		std::printf("  FAIL  %s\n", name.c_str());
		++g_failures;
	}
}

static void ExpectNear(float actual, float expected, float tolerance, const std::string& name) {
	const bool ok = std::fabs(actual - expected) <= tolerance;

	if (!ok)
		std::printf("        expected %f, got %f\n", expected, actual);

	Expect(ok, name);
}

// ---------------------------------------------------------------------------------------------
// Data below is transcribed from real captures so the tests exercise exactly what the hook sees.
// ---------------------------------------------------------------------------------------------

// The game's main camera at 3440x1440. Identical at 1280x720. The projection ignores resolution.
static const float kGameCamera[16] = {
	2.11395e-07f,  1.20904f,      1.44176e-08f,  879.607f,
	1.87907e-07f, -2.56314e-08f,  2.1494f,      -377.31f,
	-1.001f,       1.75021e-07f,  8.75103e-08f,  186.254f,
	-1.0f,         1.74846e-07f,  8.74228e-08f,  196.068f,
};

// A full-target blit quad's orthographic "WorldViewProjection": maps [0,1] to NDC with the
// half-pixel offsets of an 860x360 target baked in. Named like a camera, but affine.
static const float kBlitQuadOrtho[16] = {
	2.0f,  0.0f,  0.0f,       -1.00116f,
	0.0f, -2.0f,  0.0f,        1.00278f,
	0.0f,  0.0f,  0.00010001f, 0.0f,
	0.0f,  0.0f,  0.0f,        1.0f,
};

// A world transform: identity rotation, translation in .w, last register (0,0,0,1).
static const float kWorldTransform[16] = {
	1.0f, 0.0f, 0.0f, -125.349f,
	0.0f, 1.0f, 0.0f,  61.3734f,
	0.0f, 0.0f, 1.0f,  322.819f,
	0.0f, 0.0f, 0.0f,  1.0f,
};

// ---------------------------------------------------------------------------------------------

static void Test_ClipXScale() {
	Expect(AspectRatio::ClipXScale(1920, 1080) == 1.0f, "1920x1080 is an exact no-op");
	Expect(AspectRatio::ClipXScale(1280, 720) == 1.0f, "1280x720 is an exact no-op");
	Expect(AspectRatio::ClipXScale(3840, 2160) == 1.0f, "3840x2160 is an exact no-op");
	ExpectNear(AspectRatio::ClipXScale(3440, 1440), 0.744186f, 1.0e-5f, "3440x1440 -> 0.744186");
	ExpectNear(AspectRatio::ClipXScale(2560, 1080), 0.750000f, 1.0e-5f, "2560x1080 -> 0.75");
	ExpectNear(AspectRatio::ClipXScale(5120, 1440), 0.500000f, 1.0e-5f, "5120x1440 (32:9) -> 0.5");
	Expect(AspectRatio::ClipXScale(1920, 0) == 1.0f, "zero height is a no-op, not a divide by zero");
	Expect(AspectRatio::ClipXScale(0, 1080) == 1.0f, "zero width is a no-op");
	Expect(AspectRatio::ClipXScale(1024, 768) > 1.0f, "4:3 produces a scale above 1.0");
}

static void Test_SameAspect() {
	Expect(AspectRatio::SameAspect(3440, 1440, 3440, 1440), "the scene target itself");
	Expect(AspectRatio::SameAspect(1720, 720, 3440, 1440), "the half-size depth-of-field pass");
	Expect(AspectRatio::SameAspect(860, 360, 3440, 1440), "a quarter-size pass");
	Expect(!AspectRatio::SameAspect(1024, 1024, 3440, 1440), "a shadow map");
	Expect(!AspectRatio::SameAspect(1024, 512, 3440, 1440), "Scaleform scratch (2:1)");
	Expect(!AspectRatio::SameAspect(3440, 96, 3440, 1440), "a text strip");
	Expect(!AspectRatio::SameAspect(1144, 88, 3440, 1440), "the loading bar");
	Expect(!AspectRatio::SameAspect(0, 0, 3440, 1440), "a zero-sized target");
}

static void Test_TheTwoHalvesCancel() {
	const float projectionStretch = 3440.0f / 1440.0f / AspectRatio::referenceAspect; // 1.34375
	const float letterboxSqueeze = 2560.0f / 3440.0f;                                  // 0.744186

	ExpectNear(projectionStretch * letterboxSqueeze, 1.0f, 1.0e-4f, "the stretch and the letterbox squeeze cancel exactly");
	ExpectNear(letterboxSqueeze, AspectRatio::ClipXScale(3440, 1440), 1.0e-5f, "the correction factor equals the squeeze it replaces");
}

static void Test_IsAffine() {
	Expect(!AspectRatio::IsAffine(kGameCamera), "the camera is perspective");
	Expect(AspectRatio::IsAffine(kBlitQuadOrtho), "a blit quad's ortho is affine, even under a camera's name");
	Expect(AspectRatio::IsAffine(kWorldTransform), "a world transform is affine");
}

static void Test_CameraProjectionNames() {
	for (const char* name : { "WorldViewProj", "WorldViewProjection", "ViewProj", "g_ViewProj", "g_viewProj", "ViewProjXf" })
		Expect(AspectRatio::IsCameraProjectionName(name), std::string("accepts ") + name);

	for (const char* name : { "World", "g_World", "g_worldTransform", "InvViewProjection", "InvWorldViewProj", "ProjTranspose",
		"g_ProjectedShadowMapWorldProjectionTransform0", "UVSet2_TexTransform", "Bone", "g_SkinBoneMatrix3", "PhraseInfo", "mvp", "vfuniforms" })
		Expect(!AspectRatio::IsCameraProjectionName(name), std::string("rejects ") + name);
}

struct Constant { const char* name; unsigned int reg; unsigned int count; };

static AspectRatio::ShaderLayout LayoutOf(std::initializer_list<Constant> table) {
	AspectRatio::ShaderLayout layout;
	for (const Constant& c : table)
		AspectRatio::AccumulateConstant(layout, c.name, c.reg, c.count);
	return layout;
}

static void Test_LayoutsFromRealConstantTables() {
	using AspectRatio::LayoutKind;
	using AspectRatio::ShaderLayout;

	Expect(LayoutOf({ {"WorldViewProjection", 0, 4} }) == ShaderLayout{ LayoutKind::Matrix, 0, 4 },
		"note highway: WorldViewProjection at c0");

	Expect(LayoutOf({ {"g_AmbientLight", 11, 1}, {"g_MaterialAmbient", 10, 1}, {"g_MaterialDiffuse", 9, 1}, {"g_MaterialEmissive", 8, 1}, {"g_ViewProj", 4, 4}, {"g_World", 0, 4} })
		== ShaderLayout{ LayoutKind::Matrix, 4, 4 },
		"venue material: g_ViewProj at c4, g_World at c0 ignored");

	Expect(LayoutOf({ {"World", 0, 4}, {"WorldViewProj", 4, 4} }) == ShaderLayout{ LayoutKind::Matrix, 4, 4 },
		"World + WorldViewProj at c4");

	Expect(LayoutOf({ {"Bone", 0, 60}, {"TexTransform00", 64, 2}, {"TexTransform01", 66, 2}, {"ViewProjXf", 60, 4} })
		== ShaderLayout{ LayoutKind::Matrix, 60, 4 },
		"skinned: ViewProjXf after the bone palette at c60");

	Expect(LayoutOf({ {"g_SkinBoneMatrix3", 0, 90}, {"g_ViewProj", 90, 4}, {"g_MaterialEmissive", 94, 1} })
		== ShaderLayout{ LayoutKind::Matrix, 90, 4 },
		"skinned: g_ViewProj at c90");

	Expect(LayoutOf({ {"g_ProjectedShadowMapWorldProjectionTransform0", 8, 4}, {"g_ViewProj", 4, 4}, {"g_World", 0, 4} })
		== ShaderLayout{ LayoutKind::Matrix, 4, 4 },
		"shadow receiver: g_ViewProj corrected, the shadow transform left alone");

	Expect(LayoutOf({ {"cxadd", 0, 1}, {"cxmul", 1, 1}, {"mvp", 2, 2} }) == ShaderLayout{ LayoutKind::Scaleform, 2, 2 },
		"Scaleform: mvp at c2");

	Expect(LayoutOf({ {"mvp", 0, 2}, {"texgen", 2, 2} }) == ShaderLayout{ LayoutKind::Scaleform, 0, 2 },
		"Scaleform with texgen: mvp at c0");

	Expect(LayoutOf({ {"vfuniforms", 0, 96} }) == ShaderLayout{ LayoutKind::Scaleform, 0, 96 },
		"Scaleform batch: vfuniforms[96]");

	Expect(LayoutOf({ {"PhraseInfo", 0, 109}, {"bRenderBarColor", 109, 1}, {"fScreenX", 110, 1}, {"fScreenY", 111, 1} }).kind == LayoutKind::ScreenSpace,
		"phrase bars: screen-space, handled through the viewport");

	Expect(LayoutOf({ {"PhraseInfo", 0, 109} }).kind == LayoutKind::ScreenSpace, "phrase bars (second variant): screen-space");

	Expect(LayoutOf({ {"ApplyRectBias", 0, 1} }).kind == LayoutKind::None, "blit quad: left alone");
	Expect(LayoutOf({}).kind == LayoutKind::None, "empty table: left alone");

	Expect(AspectRatio::NeedsViewportRemap(LayoutKind::Scaleform) && AspectRatio::NeedsViewportRemap(LayoutKind::ScreenSpace),
		"UI kinds are remapped through the viewport");
	Expect(!AspectRatio::NeedsViewportRemap(LayoutKind::Matrix) && !AspectRatio::NeedsViewportRemap(LayoutKind::None),
		"3D and unknown kinds are not");

	ShaderLayout empty;
	AspectRatio::MarkNoConstants(empty);
	Expect(empty.kind == LayoutKind::PassThrough, "a constant-free shader is pass-through");

	ShaderLayout camera = LayoutOf({ {"WorldViewProjection", 0, 4} });
	AspectRatio::MarkNoConstants(camera);
	Expect(camera.kind == LayoutKind::Matrix, "marking never overrides a real layout");

	Expect(!AspectRatio::RemapPassThrough(2), "a two-triangle pass-through draw is a full-screen quad: left alone");
	Expect(!AspectRatio::RemapPassThrough(1), "a single triangle is left alone");
	Expect(AspectRatio::RemapPassThrough(3), "anything with more geometry is CPU-projected content: remapped");
	Expect(AspectRatio::RemapPassThrough(40), "a sprite mesh is remapped");
}

static void Test_RemapHorizontal() {
	const float scale = AspectRatio::ClipXScale(3440, 1440);

	int left = 0, right = 3440;
	AspectRatio::RemapHorizontal(left, right, 3440, scale);
	Expect(left == 440 && right == 3000, "the full display maps onto the centred 16:9 region");

	// The Riff Repeater sub-movie viewport as the game sets it.
	left = 589; right = 589 + 2050;
	AspectRatio::RemapHorizontal(left, right, 3440, scale);
	Expect(left == 878 && right == 2404, "a sub-movie viewport lands where the letterbox used to put it");

	left = 0; right = 1920;
	AspectRatio::RemapHorizontal(left, right, 1920, AspectRatio::ClipXScale(1920, 1080));
	Expect(left == 0 && right == 1920, "at 16:9 the remap is the identity");
}

static void Test_WidenLetterbox() {
	AspectRatio::Rect widened{};

	//   StretchRect from 3440x1440 { 0,0 -> 3440,1440 } to 3440x1440 { 440,0 -> 3000,1440 }
	Expect(AspectRatio::WidenLetterbox({ 440, 0, 3000, 1440 }, 3440, 1440, widened) && widened == AspectRatio::Rect{ 0, 0, 3440, 1440 },
		"recognises the captured 2560x1440 letterbox and widens it to the whole surface");

	Expect(!AspectRatio::WidenLetterbox({ 0, 0, 1920, 1080 }, 1920, 1080, widened), "leaves a full-surface destination alone (the 16:9 no-op)");
	Expect(!AspectRatio::WidenLetterbox({ 100, 0, 2660, 1440 }, 3440, 1440, widened), "leaves an off-centre rectangle alone");
	Expect(!AspectRatio::WidenLetterbox({ 440, 100, 3000, 1340 }, 3440, 1440, widened), "leaves a rectangle that is not full-height alone");
	Expect(!AspectRatio::WidenLetterbox({ 0, 0, 512, 512 }, 512, 512, widened), "leaves a square off-screen surface alone");
	Expect(!AspectRatio::WidenLetterbox({ 440, 0, 3000, 1440 }, 0, 0, widened), "rejects a zero-sized surface");
	Expect(!AspectRatio::WidenLetterbox({ 3000, 0, 440, 1440 }, 3440, 1440, widened), "rejects an inverted rectangle");
	Expect(AspectRatio::WidenLetterbox({ 440, 0, 2999, 1439 }, 3439, 1439, widened), "tolerates one pixel of centring error on an odd width");
}

int main() {
	std::printf("=== AspectRatioTests ===\n");

	Test_ClipXScale();
	Test_SameAspect();
	Test_TheTwoHalvesCancel();
	Test_IsAffine();
	Test_CameraProjectionNames();
	Test_LayoutsFromRealConstantTables();
	Test_RemapHorizontal();
	Test_WidenLetterbox();

	if (g_failures == 0) {
		std::printf("\nALL PASS\n");
		return 0;
	}

	std::printf("\n%d FAILURE(S)\n", g_failures);
	return 1;
}
