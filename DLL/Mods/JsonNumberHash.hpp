#pragma once

// Faster hash for the numbers the game's JSON parser interns.
//
// The game interns every JSON number in a hash set keyed by value, and its hash truncates the double to an integer first, so every
// value in [n, n+1) shares a bucket that is walked one entry at a time. Two parses hit that hard:
//   - A big profile has ~39,000 distinct numbers in [0, 1) (accuracies and such), which makes parsing it billions of comparisons.
//   - The song manifest database the game builds after the DLC scan is full of fractional times, tunings and difficulties.
//     ffio's private tree measured ~2 s off that build for 844 packages.
//
// Installed with FastProfileLoadAndSave or FastEnumeration. Install can run more than once; only the first call does anything.
namespace JsonNumberHash {
	void Install();
}
