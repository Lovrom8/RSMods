#include "MotionEngine.hpp"

// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_motion_engine.html

namespace Wwise::MotionEngine {

	/// <summary>
	/// Connects a motion device to a player. Call this function from your game to tell the motion engine that a player is using the specified device.
	/// </summary>
	/// <param name="in_iPlayerID"> - Player number, must be between 0 and 3. See platform-specific documentation for more details.</param>
	/// <param name="in_iCompanyID"> - Company ID providing support for the device</param>
	/// <param name="in_iDeviceID"> - Device ID, must be one of the currently supported devices.</param>
	/// <param name="in_pDevice"> - Windows Direct Input Device reference for DirectInput. NULL to use XInput.</param>
	/// <returns>AK_Success if the initialization was successful. AK_Fail if the device could not be initialized. Usually this means the drivers are not installed.</returns>
	AKRESULT AddPlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID, void* in_pDevice) {
		tMotion_AddPlayerMotionDevice func = (tMotion_AddPlayerMotionDevice)Wwise::Exports::func_Wwise_Motion_AddPlayerMotionDevice;
		return func(in_iPlayerID, in_iCompanyID, in_iDeviceID, in_pDevice);
	}

	/// <summary>
	/// Registers a motion device for use in the game.
	/// </summary>
	/// <param name="in_ulCompanyID"> - Company ID providing support for the device</param>
	/// <param name="in_ulPluginID"> - Device ID, must be one of the currently supported devices.</param>
	/// <param name="in_pCreateFunc"> - Creation function.</param>
	void RegisterMotionDevice(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc) {
		tMotion_RegisterMotionDevice func = (tMotion_RegisterMotionDevice)Wwise::Exports::func_Wwise_Motion_RegisterMotionDevice;
		return func(in_ulCompanyID, in_ulPluginID, in_pCreateFunc);
	}

	/// <summary>
	/// Disconnects a motion device from a player port. Call this function from your game to tell the motion engine that a player is not using the specified device anymore.
	/// </summary>
	/// <param name="in_iPlayerID"> - Player number, must be between 0 and 3. See platform-specific documentation for more details.</param>
	/// <param name="in_iCompanyID"> - Company ID providing support for the device</param>
	/// <param name="in_iDeviceID"> - Device ID, must be one of the currently supported devices.</param>
	void RemovePlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID) {
		tMotion_RemovePlayerMotionDevice func = (tMotion_RemovePlayerMotionDevice)Wwise::Exports::func_Wwise_Motion_RemovePlayerMotionDevice;
		return func(in_iPlayerID, in_iCompanyID, in_iDeviceID);
	}

	/// <summary>
	/// Attaches a player to a listener. This is necessary for the player to receive motion through the connected devices.
	/// </summary>
	/// <param name="in_iPlayerID"> - Player ID, between 0 and 3</param>
	/// <param name="in_iListener"> - Listener ID, between 0 and 7</param>
	void SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener) {
		tMotion_SetPlayerListener func = (tMotion_SetPlayerListener)Wwise::Exports::func_Wwise_Motion_SetPlayerListener;
		return func(in_iPlayerID, in_iListener);
	}

	/// <summary>
	/// Set the master volume for a player. All devices assigned to this player will be affected by this volume.
	/// </summary>
	/// <param name="in_iPlayerID"> - Player ID, between 0 and 3</param>
	/// <param name="in_fVolume"> - Master volume for the given player, in decibels (-96.3 to 96.3).</param>
	void SetPlayerVolume(AkUInt8 in_iPlayerID, AkReal32 in_fVolume) {
		tMotion_SetPlayerVolume func = (tMotion_SetPlayerVolume)Wwise::Exports::func_Wwise_Motion_SetPlayerVolume;
		return func(in_iPlayerID, in_fVolume);
	}
}