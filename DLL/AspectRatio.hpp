#pragma once

// Aspect-ratio correction for Rocksmith's hardcoded 16:9 rendering.

#include <cmath>
#include <string_view>

namespace AspectRatio {

	// The aspect Rocksmith hardcodes into its projections
	inline constexpr float referenceAspect = 16.0f / 9.0f;

	inline float AspectOf(unsigned int width, unsigned int height) {
		return (height == 0) ? 0.0f : static_cast<float>(width) / static_cast<float>(height);
	}

	inline float ClipXScale(unsigned int backBufferWidth, unsigned int backBufferHeight) {
		const float actual = AspectOf(backBufferWidth, backBufferHeight);
		if (actual <= 0.0f)
			return 1.0f;

		const float scale = referenceAspect / actual;

		return (std::fabs(scale - 1.0f) < 1.0e-4f) ? 1.0f : scale;
	}

	/// <summary>
	/// Whether a ClipXScale describes a display wider than the game's 16:9 reference.
	///
	/// Only wider-than-reference displays are correctable here. A narrower display (4:3, 16:10)
	/// produces a scale above 1.0, which widens the camera frustum past the backbuffer and drives
	/// RemapHorizontal to a negative left edge. Those displays would need pillarboxing, which is a
	/// different correction, so the mod stays inert on them instead of half-applying.
	/// </summary>
	inline bool IsWiderThanReference(float clipXScale) {
		return clipXScale > 0.0f && clipXScale < 1.0f;
	}

	inline bool SameAspect(unsigned int width, unsigned int height, unsigned int referenceWidth, unsigned int referenceHeight) {
		const float aspect = AspectOf(width, height);
		const float reference = AspectOf(referenceWidth, referenceHeight);

		if (aspect <= 0.0f || reference <= 0.0f)
			return false;

		return std::fabs(aspect - reference) < 0.01f;
	}

	// ---- The letterbox composite --------------------------------------------------------------------

	struct Rect {
		int left, top, right, bottom;

		int Width() const { return right - left; }
		int Height() const { return bottom - top; }

		bool operator==(const Rect&) const = default;
	};

	inline bool IsCentredLetterbox(const Rect& destination, unsigned int surfaceWidth, unsigned int surfaceHeight) {
		if (surfaceWidth == 0 || surfaceHeight == 0)
			return false;

		if (destination.Width() <= 0 || destination.Height() <= 0)
			return false;

		if (destination.top != 0 || destination.bottom != static_cast<int>(surfaceHeight))
			return false;

		if (destination.Width() >= static_cast<int>(surfaceWidth))
			return false;

		const int span = destination.left * 2 + destination.Width();
		const int surface = static_cast<int>(surfaceWidth);

		return span >= surface - 1 && span <= surface + 1;
	}

	inline bool WidenLetterbox(const Rect& destination, unsigned int surfaceWidth, unsigned int surfaceHeight, Rect& widened) {
		if (!IsCentredLetterbox(destination, surfaceWidth, surfaceHeight))
			return false;

		widened = { 0, 0, static_cast<int>(surfaceWidth), static_cast<int>(surfaceHeight) };
		return true;
	}

	inline void RemapHorizontal(int& left, int& right, unsigned int surfaceWidth, float scale) {
		const float band = (static_cast<float>(surfaceWidth) - static_cast<float>(surfaceWidth) * scale) * 0.5f;

		left = static_cast<int>(std::lround(band + static_cast<float>(left) * scale));
		right = static_cast<int>(std::lround(band + static_cast<float>(right) * scale));
	}

	// ---- Shader constant layouts --------------------------------------------------------------------

	enum class LayoutKind {
		None,         // Nothing this correction understands: Leave the draw alone.
		Matrix,       // A named 4x4 camera projection (possibly fused with world/view): scale its first register.
		Scaleform,    // Scaleform UI: mvp or a vfuniforms[] batch of them: remap the viewport.
		ScreenSpace,  // Positions baked into the vertices in normalised screen space: remap the viewport.
		PassThrough,  // No constants at all: positions were transformed on the CPU with the game's 16:9 projection.
	};

	inline bool RemapPassThrough(unsigned int primitiveCount) {
		return primitiveCount > 2;
	}

	struct ShaderLayout {
		LayoutKind kind = LayoutKind::None;
		unsigned int registerIndex = 0; 
		unsigned int registerCount = 0;

		bool operator==(const ShaderLayout&) const = default;
	};

	/// <summary>
	/// The names the game's shaders give the camera projection.
	/// </summary>
	inline bool IsCameraProjectionName(std::string_view name) {
		return name == "WorldViewProj"
			|| name == "WorldViewProjection"
			|| name == "g_WorldViewProj"
			|| name == "g_WorldViewProjection"
			|| name == "ViewProj"
			|| name == "ViewProjection"
			|| name == "g_ViewProj"
			|| name == "g_viewProj"
			|| name == "ViewProjXf"
			|| name == "ViewProjMatrix";
	}

	inline void AccumulateConstant(ShaderLayout& layout, std::string_view name, unsigned int registerIndex, unsigned int registerCount) {
		if (registerCount == 4 && IsCameraProjectionName(name)) {
			layout = { LayoutKind::Matrix, registerIndex, 4 };
			return;
		}

		if (layout.kind != LayoutKind::None)
			return;

		if ((name == "mvp" && registerCount == 2) || (name == "vfuniforms" && registerCount > 0))
			layout = { LayoutKind::Scaleform, registerIndex, registerCount };
		else if (name == "PhraseInfo")
			layout = { LayoutKind::ScreenSpace, 0, 0 };
	}

	inline bool IsAffine(const float* fourRegisters) {
		return fourRegisters[12] == 0.0f && fourRegisters[13] == 0.0f && fourRegisters[14] == 0.0f && fourRegisters[15] == 1.0f;
	}

	inline bool NeedsViewportRemap(LayoutKind kind) {
		return kind == LayoutKind::Scaleform || kind == LayoutKind::ScreenSpace;
	}

	inline void MarkNoConstants(ShaderLayout& layout) {
		if (layout.kind == LayoutKind::None)
			layout = { LayoutKind::PassThrough, 0, 0 };
	}
}
