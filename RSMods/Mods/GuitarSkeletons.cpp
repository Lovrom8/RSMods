#include "GuitarSkeletons.hpp"

/*
From Rocksmith 2014's Cache.psarc\cache7.7z\manifests\ folder.
Initial values are here: https://cdn.discordapp.com/attachments/711633334983196756/734706213517787226/guitar_skeletons.database.json
*/


/*
X is the distance from the fretboard, to the object. This will either be 0, or extremely close to it. This dimension is from the camera to the loft (depth)
Y is the distance from around the 11th fret. Normally - means right handed (moved to the left) and + means left handed (moved to the right). This dimension is from the left side of your screen to the right (length).
Z is the distance from the middle of the headstock. Normally - means down, and + means up. This dimension is from the bottom of your screen, to the top (width)
*/

xyzPosition GuitarSkeletons::GetSkeletonPosition() {
	return xyzPosition(X, Y, Z);
}

// Where should the string be in relationship to the headstock? Z+ up, Z- down, Z0 center of fretboard.
//WARNING: !Moves tuning camera with strings!
xyzPosition GuitarSkeletons::StringAttachPosition(bool leftHanded, bool bass, int stringNumber, bool setToDefaults, double inputZ) {
	X = 1.33226762955E-15;
	Y = -53.6527178626;

	if (leftHanded)
		Y = -Y; // Move to the opposite end of the fret board

	if (setToDefaults) {
		if (stringNumber == 0 || stringNumber == 3) // Shared across guitar & bass
			Z = 4.01962971687;

		if (!bass) { // If this is a guitar
			if (stringNumber == 1 || stringNumber == 4)
				Z = 2.41177797318;
			else if (stringNumber == 2 || stringNumber == 5)
				Z = 0.803926229477;

			if ((stringNumber - 2) > 0) // If string is 3, 4, or 5
				Z = -Z; // Moves string attach points down
		}
		else { // If this is a bass
			if (stringNumber == 1 || stringNumber == 2)
				Z = 1.33987665176;

			if ((stringNumber - 1) > 0) // If string is 2, 3
				Z = -Z; // Moves string attach points down
		}
	}
	else {
		Z = inputZ;
	}

	return GetSkeletonPosition();
}

// bool topOfFretBar -> 1 = A (top of fretbar), 0 = B (bottom of fretbar).
// int fretNumber starts at 1 (for first fret)
xyzPosition GuitarSkeletons::FretBarPosition(bool leftHanded, bool topOfFretBar, bool setToDefaults, int fretNumber, double inputZ)
{

	X = 0.0;
	
	// WARNING: !Do not edit these values!

	switch (fretNumber) {
		case 1:
			Y = -53.6527178626;
			break;
		case 2:
			Y = -47.2859269312;
			break;
		case 3:
			Y = -41.571296608;
			break;
		case 4:
			Y = -35.9061810256;
			break;
		case 5:
			Y = -30.4193875755;
			break;
		case 6:
			Y = -25.0898652107;
			break;
		case 7:
			Y = -20.0143908249;
			break;
		case 8:
			Y = -14.9411685956;
			break;
		case 9:
			Y = -10.083622901;
			break;
		case 10:
			Y = -5.36222444569;
			break;
		case 11:
			Y = -0.759047133141;
			break;
		case 12:
			Y = 3.75125107804;
			break;
		case 13:
			Y = 8.11236881306;
			break;
		case 14:
			Y = 12.3725997238;
			break;
		case 15:
			Y = 16.5238361907;
			break;
		case 16:
			Y = 20.6624282726;
			break;
		case 17:
			Y = 24.5914074241;
			break;
		case 18:
			Y = 28.5391059999;
			break;
		case 19:
			Y = 32.3164597843;
			break;
		case 20:
			Y = 36.1167300113;
			break;
		case 21:
			Y = 39.7790145179;
			break;
		case 22:
			Y = 43.3392100451;
			break;
		case 23:
			Y = 46.7799529547;
			break;
		case 24:
			Y = 50.2136309284;
			break;
		case 25:
			Y = 53.6527176054;
			break;
	}

	if (setToDefaults) {
		if (topOfFretBar)
			Z = 6.532;
		else
			Z = -5.99;
	}
	else
	{
		Z = inputZ;
	}
		

	if (leftHanded)
		Y = -Y; // Move to the opposite end of the fret board

	return GetSkeletonPosition();
}

// bool tipOfCameraMarker -> 1 = Tip, 0 = Tail.
// bool topOfCameraMarker -> 1 = Top, 0 = Bottom
xyzPosition GuitarSkeletons::CameraMarkerPosition(bool leftHanded, bool tipOfCameraMarker, bool topOfCameraMarker, bool setToDefaults, double inputZ)
{
	if (tipOfCameraMarker) { // Tip
		X = 1.33226762955e-15;
		Y = -53.847;
	}
	else { // Tail
		X = -1.86517468137e-14;
		Y = 53.8473158316;
	}

	if (setToDefaults) {
		if (topOfCameraMarker) // Top
			Z = 6.361;
		else // Bottom
			Z = -5.821;
	}
	else {
		Z = inputZ;
	}

	if (leftHanded)
		Y = -Y; // Move to the opposite end of the fret board

	return GetSkeletonPosition();
}