#include "MotionEngine.hpp"

namespace Wwise::MotionEngine {
	AKRESULT AddPlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID, void* in_pDevice) {
		tMotion_AddPlayerMotionDevice func = (tMotion_AddPlayerMotionDevice)Wwise::Exports::func_Wwise_Motion_AddPlayerMotionDevice;
		return func(in_iPlayerID, in_iCompanyID, in_iDeviceID, in_pDevice);
	}

	void RegisterMotionDevice(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc) {
		tMotion_RegisterMotionDevice func = (tMotion_RegisterMotionDevice)Wwise::Exports::func_Wwise_Motion_RegisterMotionDevice;
		return func(in_ulCompanyID, in_ulPluginID, in_pCreateFunc);
	}

	void RemovePlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID) {
		tMotion_RemovePlayerMotionDevice func = (tMotion_RemovePlayerMotionDevice)Wwise::Exports::func_Wwise_Motion_RemovePlayerMotionDevice;
		return func(in_iPlayerID, in_iCompanyID, in_iDeviceID);
	}

	void SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener) {
		tMotion_SetPlayerListener func = (tMotion_SetPlayerListener)Wwise::Exports::func_Wwise_Motion_SetPlayerListener;
		return func(in_iPlayerID, in_iListener);
	}

	void SetPlayerVolume(AkUInt8 in_iPlayerID, AkReal32 in_fVolume) {
		tMotion_SetPlayerVolume func = (tMotion_SetPlayerVolume)Wwise::Exports::func_Wwise_Motion_SetPlayerVolume;
		return func(in_iPlayerID, in_fVolume);
	}
}