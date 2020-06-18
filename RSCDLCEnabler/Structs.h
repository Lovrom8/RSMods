#pragma once
enum string_state {
	normal = 0x368,
	disabled = 0x380,
	highlight = 0x398,
	meme2 = 0x428,
	meme = 0x440,
	meme3 = 0x458
};

const float M_PI = 3.14159265359;
struct Color {
	float r;
	float g;
	float b;

	void setH(float H) {
		float U = cos(H * M_PI / 180);
		float W = sin(H * M_PI / 180);

		float _r = 1.f;
		float _g = 0.f;
		float _b = 0.f;

		r = (.299 + .701 * U + .168 * W) * _r
			+ (.587 - .587 * U + .330 * W) * _g
			+ (.114 - .114 * U - .497 * W) * _b;

		g = (.299 - .299 * U - .328 * W) * _r
			+ (.587 + .413 * U + .035 * W) * _g
			+ (.114 - .114 * U + .292 * W) * _b;

		b = (.299 - .3 * U + 1.25 * W) * _r
			+ (.587 - .588 * U - 1.05 * W) * _g
			+ (.114 + .886 * U - .203 * W) * _b;
	}
};

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