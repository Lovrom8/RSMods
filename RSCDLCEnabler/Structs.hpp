#pragma once
#include <cmath>
#include <map>
#include <vector>

enum string_state {
	Ambient = 0x350,
	Enabled = 0x368,
	Disabled = 0x380,
	Glow = 0x398,
	PegsTuning = 0x3b0,
	PegsReset = 0x3c8,
	PegsInactive = 0x3e0,
	PegsInTune = 0x3f8,
	PegsNotInTune = 0x410,
	OddHighlightedFretNumber = 0x428,
	EvenHighlightedFretNumber = 0x440, //Particles?
	FretWire = 0x458,
	Text = 0x488,
	BodyNorm = 0x4b8,
	BodyAcc = 0x4d0
	/*Ambient = 0x350, //name = "GuitarStringsAmbientColorBlind" id = "3175458924
	Enabled = 0x368, //name="GuitarStringsEnabledColorBlind" id="237528906"
	Disabled = 0x380,  //name="GuitarStringsDisabledColorBlind" id="4184626561"
	Glow = 0x398, //name="GuitarStringsGlowBlind" id="3217126622
	PegsTuning = 0x3b0, //name = "GuitarPegsTuningBlind"id = "1806691030"
	PegsSuccess = 0x3c8, //name = "GuitarPegsSuccessBlind" id = "2074129191
	PegsReset = 0x3e0, //name = "GuitarPegsResetBlind"id = "2670600760"
	PegInTune = 0x3f8,  //name="GuitarPegsInTuneBlind" id="2547441015"
	PegNotInTune = 0x410, //name = "GuitarPegsOutTuneBlind" id = "1163525281"
	Text = 0x470, //name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
	Particles = 0x488, //name="GuitarRegistrarForkParticlesBlind" id="3239612871"
	BodyAcc = 0x4a0, // = name = "NotewayBodypartsAccentBlind" id = "47948252"
	BodyNorm = 0x4b8 //= name = "NotewayBodypartsPreviewBlind" id = "338656387"
	//BodyPrev == ?? - doesn't seem to be used while in a song*/

	// GC stuff not used, becuase can't use GC in non E standard anyway, 
	//but including theoretical values for the "replace all and self defined CB mode options.

	/*
	in theroy based on order within gamecolormanager.flat and adding 18 hex to each address;
	350 - start point may be off however....
	368 - name="GuitarStringsAmbientColorBlind"id="3175458924"
	380 - name="GuitarStringsEnabledColorBlind"id="237528906"
	398 - name="GuitarStringsDisabledColorBlind"id="4184626561"
	3b0 - name="GuitarStringsGlowBlind"id="3217126622"

	3c8 - name="GuitarPegsTuningBlind"id="1806691030"
	3e0 - name="GuitarPegsResetBlind"id="2670600760"
	3f8 - name="GuitarPegsSuccessBlind"id="2074129191"

	410 - name="GuitarPegsInTuneBlind"id="2547441015"
	428 - name="GuitarPegsOutTuneBlind"id="1163525281"
	440 - name="GuitarRegistrarTextIndicatorBlind"id="3186002004"
	458 - name="GuitarRegistrarForkParticlesBlind"id="3239612871"
	470 - name="NotewayBodypartsNormalBlind"id="3629363565"
	488 - name="NotewayBodypartsAccentBlind"id="47948252"
	4a0 - name="NotewayBodypartsPreviewBlind"id="338656387"

	4b8 - name="GuitarcadeMainBlind"id="2332417892"
	4d0 - name="GuitarcadeAdditiveBlind"id="1537455274"
	4e8 - name="GuitarcadeUIBlind"id="2107771575"
	*/
};

const double M_PI = 3.14159265359;
struct Color {
	float r;
	float g;
	float b;

	Color() {
		r = 1.0f;
		g = 0.0f;
		b = 0.0f;
	}

	Color(int R, int G, int B) {
		r = (float)R / 255;
		g = (float)G / 255;
		b = (float)B / 255;
	}

	Color(float R, float G, float B) {
		r = R;
		g = G;
		b = B;
	}

	void setH(float H) {
		double U = cos(H * M_PI / 180);
		double W = sin(H * M_PI / 180);

		float _r = 1.f;
		float _g = 0.f;
		float _b = 0.f;

		r = (float)(.299 + .701 * U + .168 * W) * _r
			+ (float)(.587 - .587 * U + .330 * W) * _g
			+ (float)(.114 - .114 * U - .497 * W) * _b;

		g = (float)(.299 - .299 * U - .328 * W) * _r
			+ (float)(.587 + .413 * U + .035 * W) * _g
			+ (float)(.114 - .114 * U + .292 * W) * _b;

		b = (float)(.299 - .3 * U + 1.25 * W) * _r
			+ (float)(.587 - .588 * U - 1.05 * W) * _g
			+ (float)(.114 + .886 * U - .203 * W) * _b;
	}
};

struct xyzPosition {
	double x;
	double y;
	double z;

	xyzPosition() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	xyzPosition(double X, double Y, double Z) {
		x = X;
		y = Y;
		z = Z;
	}
};

typedef Color RSColor;
typedef std::map<std::string, RSColor> ColorMap;
typedef std::vector<RSColor> ColorList;

/*------------------D3D | FOR REFERENCE---------------------------------*/

class D3DInfo
{
public:
	static const int Queryinterface_Index = 0;
	static const int AddRef_Index = 1;
	static const int Release_Index = 2;
	static const int TestCooperativeLevel_Index = 3;
	static const int GetAvailableTextureMem_Index = 4;
	static const int EvictManagedResources_Index = 5;
	static const int GetDirect3D_Index = 6;
	static const int GetDeviceCaps_Index = 7;
	static const int GetDisplayMode_Index = 8;
	static const int GetCreationParameters_Index = 9;
	static const int SetCursorProperties_Index = 10;
	static const int SetCursorPosition_Index = 11;
	static const int ShowCursor_Index = 12;
	static const int CreateAdditionalSwapChain_Index = 13;
	static const int GetSwapChain_Index = 14;
	static const int GetNumberOfSwapChains_Index = 15;
	static const int Reset_Index = 16;
	static const int Present_Index = 17;
	static const int GetBackBuffer_Index = 18;
	static const int GetRasterStatus_Index = 19;
	static const int SetDialogBoxMode_Index = 20;
	static const int SetGammaRamp_Index = 21;
	static const int GetGammaRamp_Index = 22;
	static const int CreateTexture_Index = 23;
	static const int CreateVolumeTexture_Index = 24;
	static const int CreateCubeTexture_Index = 25;
	static const int CreateVertexBuffer_Index = 26;
	static const int CreateIndexBuffer_Index = 27;
	static const int CreateRenderTarget_Index = 28;
	static const int CreateDepthStencilSurface_Index = 29;
	static const int UpdateSurface_Index = 30;
	static const int UpdateTexture_Index = 31;
	static const int GetRenderTargetData_Index = 32;
	static const int GetFrontBufferData_Index = 33;
	static const int StretchRect_Index = 34;
	static const int ColorFill_Index = 35;
	static const int CreateOffscreenPlainSurface_Index = 36;
	static const int SetRenderTarget_Index = 37;
	static const int GetRenderTarget_Index = 38;
	static const int SetDepthStencilSurface_Index = 39;
	static const int GetDepthStencilSurface_Index = 40;
	static const int BeginScene_Index = 41;
	static const int EndScene_Index = 42;
	static const int Clear_Index = 43;
	static const int SetTransform_Index = 44;
	static const int GetTransform_Index = 45;
	static const int MultiplyTransform_Index = 46;
	static const int SetViewport_Index = 47;
	static const int GetViewport_Index = 48;
	static const int SetMaterial_Index = 49;
	static const int GetMaterial_Index = 50;
	static const int SetLight_Index = 51;
	static const int GetLight_Index = 52;
	static const int LightEnable_Index = 53;
	static const int GetLightEnable_Index = 54;
	static const int SetClipPlane_Index = 55;
	static const int GetClipPlane_Index = 56;
	static const int SetRenderState_Index = 57;
	static const int GetRenderState_Index = 58;
	static const int CreateStateBlock_Index = 59;
	static const int BeginStateBlock_Index = 60;
	static const int EndStateBlock_Index = 61;
	static const int SetClipStatus_Index = 62;
	static const int GetClipStatus_Index = 63;
	static const int GetTexture_Index = 64;
	static const int SetTexture_Index = 65;
	static const int GetTextureStageState_Index = 66;
	static const int SetTextureStageState_Index = 67;
	static const int GetSamplerState_Index = 68;
	static const int SetSamplerState_Index = 69;
	static const int ValidateDevice_Index = 70;
	static const int SetPaletteEntries_Index = 71;
	static const int GetPaletteEntries_Index = 72;
	static const int SetCurrentTexturePalette_Index = 73;
	static const int GetCurrentTexturePalette_Index = 74;
	static const int SetScissorRect_Index = 75;
	static const int GetScissorRect_Index = 76;
	static const int SetSoftwareVertexProcessing_Index = 77;
	static const int GetSoftwareVertexProcessing_Index = 78;
	static const int SetNPatchMode_Index = 79;
	static const int GetNPatchMode_Index = 80;
	static const int DrawPrimitive_Index = 81;
	static const int DrawIndexedPrimitive_Index = 82;
	static const int DrawPrimitiveUP_Index = 83;
	static const int DrawIndexedPrimitiveUP_Index = 84;
	static const int ProcessVertices_Index = 85;
	static const int CreateVertexDeclaration_Index = 86;
	static const int SetVertexDeclaration_Index = 87;
	static const int GetVertexDeclaration_Index = 88;
	static const int SetFVF_Index = 89;
	static const int GetFVF_Index = 90;
	static const int CreateVertexShader_Index = 91;
	static const int SetVertexShader_Index = 92;
	static const int GetVertexShader_Index = 93;
	static const int SetVertexShaderConstantF_Index = 94;
	static const int GetVertexShaderConstantF_Index = 95;
	static const int SetVertexShaderConstantI_Index = 96;
	static const int GetVertexShaderConstantI_Index = 97;
	static const int SetVertexShaderConstantB_Index = 98;
	static const int GetVertexShaderConstantB_Index = 99;
	static const int SetStreamSource_Index = 100;
	static const int GetStreamSource_Index = 101;
	static const int SetStreamSourceFreq_Index = 102;
	static const int GetStreamSourceFreq_Index = 103;
	static const int SetIndices_Index = 104;
	static const int GetIndices_Index = 105;
	static const int CreatePixelShader_Index = 106;
	static const int SetPixelShader_Index = 107;
	static const int GetPixelShader_Index = 108;
	static const int SetPixelShaderConstantF_Index = 109;
	static const int GetPixelShaderConstantF_Index = 110;
	static const int SetPixelShaderConstantI_Index = 111;
	static const int GetPixelShaderConstantI_Index = 112;
	static const int SetPixelShaderConstantB_Index = 113;
	static const int GetPixelShaderConstantB_Index = 114;
	static const int DrawRectPatch_Index = 115;
	static const int DrawTriPatch_Index = 116;
	static const int DeletePatch_Index = 117;
	static const int CreateQuery_Index = 118;
	static const int NumberOfFunctions = 118;
};