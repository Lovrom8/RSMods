#pragma once
#pragma warning(disable: 4244 4305) // Double -> Float may lose data. | initializing truncated double -> float | uninitialized value
#include <cmath>

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
	
//= 0x4b8 //name = "GuitarcadeMainBlind"id = "2332417892"
	//= 0x4d0 //name = "GuitarcadeAdditiveBlind"id = "1537455274"
	//= 0x4e8 //name = "GuitarcadeUIBlind"id = "2107771575"


	//0x350 = unknown
	//0x3b0 = tuning peg glow color (not realtime)
	//0x3c8 = unknown
	//0x3e0 = inactive tuning peg glow (only string index 0, applies to all pegs, not realtime)
	//0x3f8 = tuner note letter color (not realtime)
	//0x410 = lit text at bottom of note way - maybe
	//0x428 = unknown
	//0x440 = unknown
	//0x458 = unknown
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

const float M_PI = 3.14159265359;
struct Color {
	float r;
	float g;
	float b;

	Color() {

	}

	Color(int R, int G, int B) {
		r = (float)R/255;
		g = (float)G/255;
		b = (float)B/255;
	}

	Color(float R, float G, float B) {
		r = R;
		g = G;
		b = B;
	}

	void setH(float H) {
		float U = cos(H * M_PI / 180);
		float W = sin(H * M_PI / 180);

		//float _r = 1.f;
		//float _g = 0.f;
		//float _b = 0.f;

		r = (.299 + .701 * U + .168 * W) * r
			+ (.587 - .587 * U + .330 * W) * g
			+ (.114 - .114 * U - .497 * W) * b;

		g = (.299 - .299 * U - .328 * W) * r
			+ (.587 + .413 * U + .035 * W) * g
			+ (.114 - .114 * U + .292 * W) * b;

		b = (.299 - .3 * U + 1.25 * W) * r
			+ (.587 - .588 * U - 1.05 * W) * g
			+ (.114 + .886 * U - .203 * W) * b;
	}
};

struct xyzPosition {
	double x;
	double y;
	double z;

	xyzPosition() {

	}

	xyzPosition(double X, double Y, double Z) {
		x = X;
		y = Y;
		z = Z;
	}
};

typedef Color RSColor;

/*------------------AUDIO KINETIC STUFF---------------------------------*/

typedef unsigned char		AkUInt8;			///< Unsigned 8-bit integer
typedef unsigned short		AkUInt16;			///< Unsigned 16-bit integer
typedef unsigned long		AkUInt32;			///< Unsigned 32-bit integer
typedef char			AkInt8;					///< Signed 8-bit integer
typedef short			AkInt16;				///< Signed 16-bit integer
typedef long   			AkInt32;				///< Signed 32-bit integer
typedef int 		AkInt;					///< Signed integer
typedef int AkIntPtr;						///< Integer type for pointers
typedef  unsigned int AkUIntPtr;			///< Integer (unsigned) type for pointers
typedef float			AkReal32;				///< 32-bit floating point
typedef double          AkReal64;				///< 64-bit floating point

typedef AkUInt32		AkUniqueID;			 		///< Unique 32-bit ID
typedef AkUInt32		AkStateID;			 		///< State ID
typedef AkUInt32		AkStateGroupID;		 		///< State group ID
typedef AkUInt32		AkPlayingID;		 		///< Playing ID
typedef	AkInt32			AkTimeMs;			 		///< Time in ms
typedef AkReal32		AkPitchValue;		 		///< Pitch value
typedef AkReal32		AkVolumeValue;		 		///< Volume value( also apply to LFE )
typedef AkUIntPtr		AkGameObjectID;		 		///< Game object ID
typedef AkReal32		AkLPFType;			 		///< Low-pass filter type
typedef AkInt32			AkMemPoolId;		 		///< Memory pool ID
typedef AkUInt32		AkPluginID;			 		///< Source or effect plug-in ID
typedef AkUInt32		AkCodecID;			 		///< Codec plug-in ID
typedef AkUInt32		AkEnvID;			 		///< Environmental ID
typedef AkInt16			AkPluginParamID;	 		///< Source or effect plug-in parameter ID
typedef AkInt8			AkPriority;			 		///< Priority
typedef AkUInt16        AkDataCompID;		 		///< Data compression format ID
typedef AkUInt16        AkDataTypeID;		 		///< Data sample type ID
typedef AkUInt8			AkDataInterleaveID;	 		///< Data interleaved state ID
typedef AkUInt32        AkSwitchGroupID;	 		///< Switch group ID
typedef AkUInt32        AkSwitchStateID;	 		///< Switch ID
typedef AkUInt32        AkRtpcID;			 		///< Real time parameter control ID
typedef AkReal32        AkRtpcValue;		 		///< Real time parameter control value
typedef AkUInt32        AkBankID;			 		///< Run time bank ID
typedef AkUInt32        AkFileID;			 		///< Integer-type file identifier
typedef AkUInt32        AkDeviceID;			 		///< I/O device ID
typedef AkUInt32		AkTriggerID;		 		///< Trigger ID
typedef AkUInt32		AkArgumentValueID;			///< Argument value ID
typedef AkUInt32		AkChannelMask;				///< Channel mask (similar to WAVE_FORMAT_EXTENSIBLE). Bit values are defined in AkCommonDefs.h.

static const AkGameObjectID	AK_INVALID_GAME_OBJECT = (AkGameObjectID)-1;	///< Invalid game object (may also mean all game objects)

enum AKRESULT
{
	AK_NotImplemented = 0,	///< This feature is not implemented.
	AK_Success = 1,	///< The operation was successful.
	AK_Fail = 2,	///< The operation failed.
	AK_PartialSuccess = 3,	///< The operation succeeded partially.
	AK_NotCompatible = 4,	///< Incompatible formats
	AK_AlreadyConnected = 5,	///< The stream is already connected to another node.
	AK_NameNotSet = 6,	///< Trying to open a file when its name was not set
	AK_InvalidFile = 7,	///< An unexpected value causes the file to be invalid.
	AK_CorruptedFile = 8,	///< The file is missing some exprected data.
	AK_MaxReached = 9,	///< The maximum was reached.
	AK_InputsInUsed = 10,	///< Inputs are currently used.
	AK_OutputsInUsed = 11,	///< Outputs are currently used.
	AK_InvalidName = 12,	///< The name is invalid.
	AK_NameAlreadyInUse = 13,	///< The name is already in use.
	AK_InvalidID = 14,	///< The ID is invalid.
	AK_IDNotFound = 15,	///< The ID was not found.
	AK_InvalidInstanceID = 16,	///< The InstanceID is invalid.
	AK_NoMoreData = 17,	///< No more data is available from the source.
	AK_NoSourceAvailable = 18,	///< There is no child (source) associated with the node.
	AK_StateGroupAlreadyExists = 19,	///< The StateGroup already exists.
	AK_InvalidStateGroup = 20,	///< The StateGroup is not a valid channel.
	AK_ChildAlreadyHasAParent = 21,	///< The child already has a parent.
	AK_InvalidLanguage = 22,	///< The language is invalid (applies to the Low-Level I/O).
	AK_CannotAddItseflAsAChild = 23,	///< It is not possible to add itself as its own child.
	AK_TransitionNotFound = 24,	///< The transition is not in the list.
	AK_TransitionNotStartable = 25,	///< Start allowed in the Running and Done states.
	AK_TransitionNotRemovable = 26,	///< Must not be in the Computing state.
	AK_UsersListFull = 27,	///< No one can be added any more, could be AK_MaxReached.
	AK_UserAlreadyInList = 28,	///< This user is already there.
	AK_UserNotInList = 29,	///< This user is not there.
	AK_NoTransitionPoint = 30,	///< Not in use.
	AK_InvalidParameter = 31,	///< Something is not within bounds.
	AK_ParameterAdjusted = 32,	///< Something was not within bounds and was relocated to the nearest OK value.
	AK_IsA3DSound = 33,	///< The sound has 3D parameters.
	AK_NotA3DSound = 34,	///< The sound does not have 3D parameters.
	AK_ElementAlreadyInList = 35,	///< The item could not be added because it was already in the list.
	AK_PathNotFound = 36,	///< This path is not known.
	AK_PathNoVertices = 37,	///< Stuff in vertices before trying to start it
	AK_PathNotRunning = 38,	///< Only a running path can be paused.
	AK_PathNotPaused = 39,	///< Only a paused path can be resumed.
	AK_PathNodeAlreadyInList = 40,	///< This path is already there.
	AK_PathNodeNotInList = 41,	///< This path is not there.
	AK_VoiceNotFound = 42,	///< Unknown in our voices list
	AK_DataNeeded = 43,	///< The consumer needs more.
	AK_NoDataNeeded = 44,	///< The consumer does not need more.
	AK_DataReady = 45,	///< The provider has available data.
	AK_NoDataReady = 46,	///< The provider does not have available data.
	AK_NoMoreSlotAvailable = 47,	///< Not enough space to load bank.
	AK_SlotNotFound = 48,	///< Bank error.
	AK_ProcessingOnly = 49,	///< No need to fetch new data.
	AK_MemoryLeak = 50,	///< Debug mode only.
	AK_CorruptedBlockList = 51,	///< The memory manager's block list has been corrupted.
	AK_InsufficientMemory = 52,	///< Memory error.
	AK_Cancelled = 53,	///< The requested action was cancelled (not an error).
	AK_UnknownBankID = 54,	///< Trying to load a bank using an ID which is not defined.
	AK_IsProcessing = 55,   ///< Asynchronous pipeline component is processing.
	AK_BankReadError = 56,	///< Error while reading a bank.
	AK_InvalidSwitchType = 57,	///< Invalid switch type (used with the switch container)
	AK_VoiceDone = 58,	///< Internal use only.
	AK_UnknownEnvironment = 59,	///< This environment is not defined.
	AK_EnvironmentInUse = 60,	///< This environment is used by an object.
	AK_UnknownObject = 61,	///< This object is not defined.
	AK_NoConversionNeeded = 62,	///< Audio data already in target format, no conversion to perform.
	AK_FormatNotReady = 63,   ///< Source format not known yet.
	AK_WrongBankVersion = 64,	///< The bank version is not compatible with the current bank reader.
	AK_DataReadyNoProcess = 65,	///< The provider has some data but does not process it (virtual voices).
	AK_FileNotFound = 66,   ///< File not found.
	AK_DeviceNotReady = 67,   ///< IO device not ready (may be because the tray is open)
	AK_CouldNotCreateSecBuffer = 68,   ///< The direct sound secondary buffer creation failed.
	AK_BankAlreadyLoaded = 69,	///< The bank load failed because the bank is already loaded.
	AK_RenderedFX = 71,	///< The effect on the node is rendered.
	AK_ProcessNeeded = 72,	///< A routine needs to be executed on some CPU.
	AK_ProcessDone = 73,	///< The executed routine has finished its execution.
	AK_MemManagerNotInitialized = 74,	///< The memory manager should have been initialized at this point.
	AK_StreamMgrNotInitialized = 75,	///< The stream manager should have been initialized at this point.
	AK_SSEInstructionsNotSupported = 76,///< The machine does not support SSE instructions (required on PC).
	AK_Busy = 77,	///< The system is busy and could not process the request.
	AK_UnsupportedChannelConfig = 78,	///< Channel configuration is not supported in the current execution context.
	AK_PluginMediaNotAvailable = 79,	///< Plugin media is not available for effect.
};


/// Curve interpolation types
enum AkCurveInterpolation
{
	//DONT GO BEYOND 15! (see below for details)
	//Curves from 0 to LastFadeCurve NEED TO BE A MIRROR IMAGE AROUND LINEAR (eg. Log3 is the inverse of Exp3)
	AkCurveInterpolation_Log3 = 0, ///< Log3
	AkCurveInterpolation_Sine = 1, ///< Sine
	AkCurveInterpolation_Log1 = 2, ///< Log1
	AkCurveInterpolation_InvSCurve = 3, ///< Inversed S Curve
	AkCurveInterpolation_Linear = 4, ///< Linear (Default)
	AkCurveInterpolation_SCurve = 5, ///< S Curve
	AkCurveInterpolation_Exp1 = 6, ///< Exp1
	AkCurveInterpolation_SineRecip = 7, ///< Reciprocal of sine curve
	AkCurveInterpolation_Exp3 = 8, ///< Exp3
	AkCurveInterpolation_LastFadeCurve = 8, ///< Update this value to reflect last curve available for fades
	AkCurveInterpolation_Constant = 9  ///< Constant ( not valid for fading values )
//DONT GO BEYOND 15! The value is stored on 5 bits,
//but we can use only 4 bits for the actual values, keeping
//the 5th bit at 0 to void problems when the value is
//expanded to 32 bits.
};

enum RTPCValue_type
{
	RTPCValue_Default,      ///< The value is the Default RTPC.
	RTPCValue_Global,       ///< The value is the Global RTPC.
	RTPCValue_GameObject,   ///< The value is the game object specific RTPC.
	RTPCValue_PlayingID,    ///< The value is the playing ID specific RTPC.
	RTPCValue_Unavailable   ///< The value is not available for the RTPC specified.
};


/*------------------AUDIO KINETIC STUFF---------------------------------*/

/*------------------FOR REFERENCE---------------------------------------*/

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