#pragma once

#include "Types.hpp"
#include "Exports.hpp"

namespace Wwise::MotionEngine {
	AKRESULT AddPlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID, void* in_pDevice);
	void RegisterMotionDevice(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc);
	void RemovePlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID);
	void SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener);
	void SetPlayerVolume(AkUInt8 in_iPlayerID, AkReal32 in_fVolume);
}

typedef AKRESULT(__cdecl* tMotion_AddPlayerMotionDevice)(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID, void* in_pDevice);
typedef void(__cdecl* tMotion_RegisterMotionDevice)(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc);
typedef void(__cdecl* tMotion_RemovePlayerMotionDevice)(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID);
typedef void(__cdecl* tMotion_SetPlayerListener)(AkUInt8 in_iPlayerID, AkUInt8 in_iListener);
typedef void(__cdecl* tMotion_SetPlayerVolume)(AkUInt8 in_iPlayerID, AkReal32 in_fVolume);