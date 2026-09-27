#pragma once

namespace RiffRepeater {
	// Real song speed limits, in percent. 6.25% = 1/16x, 400% = 4x.
	inline constexpr float MinRealSpeed = 6.25f;
	inline constexpr float MaxRealSpeed = 400.f;

	float GetSpeed(bool realSpeed = false);
	void SetSpeed(float newSpeed, bool isRealSpeed = false);
	float ConvertSpeed(float speed);
	void EnableTimeStretch();
	void RemoveTimeStretch();
	void DisableTimeStretch();
	void SyncTimeStretch();
	void EnableLinearSpeeds();
	void DisableLinearSpeeds();
	bool LogSongID(const std::string& songKey);
	void HandleSongChange(const std::string& previousSongKey);
	void SaveSpeedToFileOnChange();

	inline std::map<std::string, AkUInt32> SongObjectIDs;
	inline AkUInt32 currentSongID;
	inline AkUInt32 timeStretchSongID; // Object the Time Stretch effect is currently attached to.
	inline bool readyToLogSongID;
	inline bool loggedCurrentSongID = false;

	inline bool currentlyEnabled_Above100 = false;
	inline bool currentlyEnabled_LinearRR = false;

	inline bool saveNewRRSpeedToFile = false;
}