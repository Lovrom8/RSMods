// Replacement for the game's phrase banner progress vertex shader, rebuilt from its shipped bytecode (fxc /dumpbin).
// Same two changes as phrase_bars_vtf.hlsl: phrase data comes from a vertex texture, and thin phrases are drawn edge to edge.
// This shader has no screen width constant of its own, so RSMods stores the banner's width in the last texel of the phrase texture.
//
// Build: fxc /nologo /T vs_3_0 /E main /O3 /Fh phrase_progress_vtf.h /Vn g_phraseProgressVtfVS phrase_progress_vtf.hlsl

sampler2D PhraseTex : register(s0);   // D3DVERTEXTEXTURESAMPLER0

#define PHRASE_TEX_WIDTH   4096.0f
#define THIN_PHRASE_PIXELS 4.0f
#define WIDTH_TEXEL        4095

float4 FetchPhrase(int id)
{
	return tex2Dlod(PhraseTex, float4((id + 0.5f) / PHRASE_TEX_WIDTH, 0.5f, 0.0f, 0.0f));
}

void main(float4 position : POSITION, float2 texCoord : TEXCOORD0,
	out float4 outPosition : POSITION, out float4 outTexCoord : TEXCOORD0,
	out float4 outScreen : TEXCOORD1, out float4 outExtra : TEXCOORD2)
{
	int vertex = position.z;
	int phrase = vertex / 4;
	int corner = vertex % 4;
	bool leftCorner = (corner == 0 || corner == 3);

	float4 info  = FetchPhrase(phrase);
	float  nextX = FetchPhrase(phrase + 1).x;

	float bannerWidth = FetchPhrase(WIDTH_TEXEL).x;   // Same width the game lays the banner out in
	float phraseWidth = nextX - info.x;
	bool thin = phraseWidth > 0.0f && phraseWidth < THIN_PHRASE_PIXELS && bannerWidth > 0.0f;

	float2 pos = position.xy;
	if (thin)
		pos.x = (leftCorner ? info.x : nextX) / bannerWidth;

	outPosition = float4(pos.x * 2.0f - 1.0f, pos.y * -2.0f + 1.0f, 0.9f, 1.0f);
	outTexCoord = float4(texCoord.x, texCoord.y, info.z, position.w);   // .w = section or bucket
	outScreen = float4(pos.x * 1280.0f, pos.y * 720.0f, info.x, nextX - 1.0f);
	outExtra = float4(0.0f, 0.0f, 0.0f, info.w);
}
