#pragma once

// Lets Non-Stop Play use the extra song lists the GUI adds to a profile (Profile Edits > Add Song List).
//
// The profile keeps its song lists in one array, and most of the game sizes itself from that array: the Learn
// a Song and Song Arcade filters, the song list editor (including its next / previous list buttons),
// membership checks, and adding or removing songs all work for any number of lists. Non-Stop Play is the
// exception. It has two hardcoded limits:
//
//  - The Non-Stop Play menu builds its "song list" choice from exactly 8 options (All Songs, Favorites, Song List 1-6).
//    Two loops in the same function stop at 8: one pushes the option ids, the other builds their labels.
//    A saved choice that is not among those ids is reset to All Songs, and the reset is saved.
//  - The Non-Stop Play pool builder switches on the chosen id with "cmp eax, 7" and an 8 entry jump table.
//    A higher id falls out of the switch and the pool is empty. Every song list entry of the table goes to
//    the same code, which fetches song list id - 2 for any id.
//
// Install() makes both loops stop at 2 + the number of song lists (never fewer than the stock 8, at most 22,
// the GUI's 20 lists), and sends ids above 7 to the table's song list entry when the list exists. With the
// stock 6 lists nothing changes.
namespace ExtraSongLists {
	// Patch the three sites once. Later calls are no-ops.
	void Install();
}
