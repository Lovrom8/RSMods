// Replacement for the game's phrase banner bar vertex shader, rebuilt from its shipped bytecode (fxc /dumpbin).
//
// Changes from the game's shader:
//  - Phrase data comes from a vertex texture (one texel per phrase, 4096 wide) instead of the
//    109-entry constant array, so the banner isn't limited to 108 phrases.
//  - Phrases thinner than THIN_PHRASE_PIXELS are drawn edge to edge over their exact
//    range. The game's mesh starts every phrase one pixel after floor(start) and this
//    shader pulls colored bars in by another pixel per side, so with hundreds of phrases
//    most of them would have no width left and the banner would be full of gaps.
//    Wider phrases keep the game's look.
// The constants the game sets keep their original registers, so its values still land in the right place.
//
// Build: fxc /nologo /T vs_3_0 /E main /O3 /Fh phrase_bars_vtf.h /Vn g_phraseBarsVtfVS phrase_bars_vtf.hlsl

float  barColors    : register(c109);   // > 0 when drawing the colored bars
float  screenWidth  : register(c110);
float  screenHeight : register(c111);
sampler2D PhraseTex : register(s0);     // D3DVERTEXTEXTURESAMPLER0

#define PHRASE_TEX_WIDTH   4096.0f
#define THIN_PHRASE_PIXELS 4.0f

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

	// info.x is in the same pixels as position.x * screenWidth. nextX <= info.x means there is no next phrase.
	float phraseWidth = nextX - info.x;
	bool thin = phraseWidth > 0.0f && phraseWidth < THIN_PHRASE_PIXELS;

	float2 pos = position.xy;
	if (thin)
		pos.x = (leftCorner ? info.x : nextX) / screenWidth;

	if (barColors > 0.0f)
	{
		float pixelWidth = 1.0f / screenWidth;
		float pixelHeight = 1.0f / screenHeight;

		float heightBias;
		if (corner < 2)
			heightBias = (info.y < 0.001f) ? pixelHeight : info.y;
		else
			heightBias = -pixelHeight;

		float widthBias = thin ? 0.0f : (leftCorner ? pixelWidth : -pixelWidth);

		outPosition.xy = (pos + float2(widthBias, heightBias)) * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	}
	else
	{
		outPosition.xy = pos * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	}
	outPosition.zw = float2(0.9f, 1.0f);

	outTexCoord = float4(texCoord.x, texCoord.y, info.z, 0.0f);
	outScreen = float4(pos.x * screenWidth, pos.y * screenHeight, info.x, nextX - 1.0f);
	outExtra = float4(0.0f, 0.0f, 0.0f, info.w);
}
