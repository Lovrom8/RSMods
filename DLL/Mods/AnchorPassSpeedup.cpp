#include "../stdafx.h"
#include "AnchorPassSpeedup.hpp"
#include "../MemUtil.hpp"

/// <summary>
/// When a song loads, Rocksmith runs a pass over every difficulty level (0x0055DDC0 on the September 2022 build) that
/// works out, per phrase iteration, which notes it holds, and cleans up the level's anchors (drops empty ones, pulls
/// an anchor back to an unpitched slide that ends on it). For every phrase iteration and for every anchor it walks
/// the whole note list, calling fabsf a few times per note. That is fine for a normal song, but a chart with
/// ~40,000 notes and ~9,000 anchors in one level takes minutes per level and looks like a soft lock after the tuner.
///
/// This replaces the three hot loops with lookups that give the exact same answers:
///  - Phrase note scan: which notes fall inside the phrase iteration, the last one, and the last "plain" one.
///  - Anchor note scan: does any note start inside the anchor or end on its start, and the first sustained note that
///    ends on its start (the loop stops there and checks it for an unpitched slide).
///  - Anchor lookahead: for an empty anchor, the first anchor in a later phrase iteration, to see if it matches.
/// The first two sort the notes once per level and use binary searches. Every tolerance check in them is monotonic in
/// the note time, so the matching notes are a contiguous run of the sorted list. The float maths is done in the same
/// order and precision as the game (the game runs x87 in single precision, so plain float maths rounds the same).
/// The lookahead keeps the game's order, just without the x87 round trips.
///
/// Everything that changes game data (removing anchors, moving anchor starts, the per phrase iteration info) is still
/// done by the game's own code. We only fill in the loop results and skip the loops.
///
/// On levels small enough for the original loops to be quick, the original still runs and our answers are compared
/// against it. Any difference turns this off for the rest of the session, so a mistake here can't stay hidden.
/// </summary>
namespace AnchorPassSpeedup {

	// Locals in the game function's stack frame, relative to its EBP.
	constexpr int frameSong = -0x10C;
	constexpr int frameLevel = -0xF8;
	constexpr int framePhraseIterationInfo = -0x168;
	constexpr int framePhraseIterationIndex = -0x14C;
	constexpr int framePhraseIterationCount = -0x150;
	constexpr int framePhraseStart = -0x15C;
	constexpr int framePhraseEnd = -0x11C;
	constexpr int framePhraseHasNotes = -0x154;
	constexpr int framePhraseLastNoteTime = -0xEC;
	constexpr int framePhraseLastPlainNoteTime = -0xE8;
	constexpr int frameAnchor = -0x160;
	constexpr int frameAnchorCount = -0x134;
	constexpr int frameAnchorStart = -0x110;
	constexpr int frameNextAnchorStart = -0x108;
	constexpr int frameAnchorHasNotes = -0xE2;
	constexpr int frameAnchorSlideIn = -0xE1;
	constexpr int frameRemoveAnchor = -0xE3;

	// Song
	constexpr int songPhraseIterations = 0x64;		// vector, 0x18 byte entries
	constexpr int songChords = 0x94;				// vector, 0x48 byte entries
	constexpr int songChordNotes = 0xAC;			// array, 0x948 byte entries
	constexpr int songLength = 0x148;				// float

	constexpr int phraseIterationSize = 0x18;
	constexpr int phraseIterationTime = 0x4;		// float

	constexpr int chordSize = 0x48;
	constexpr int chordFrets = 0x4;					// byte[6], 0xFF = unused string

	constexpr int chordNotesSize = 0x948;
	constexpr int chordNotesSlideTo = 0x930;			// byte[6]
	constexpr int chordNotesSlideUnpitchTo = 0x936;	// byte[6]

	// Difficulty level
	constexpr int levelAnchors = 0x0;				// vector, 0x1C byte entries
	constexpr int levelNotes = 0x30;				// vector, 0x1C8 byte entries
	constexpr int levelDifficulty = 0x60;			// int

	constexpr int anchorSize = 0x1C;
	constexpr int anchorStart = 0x0;				// float
	constexpr int anchorFret = 0x10;				// byte
	constexpr int anchorWidth = 0x14;				// int

	constexpr int noteSize = 0x1C8;
	constexpr int noteMask = 0x0;					// uint
	constexpr int noteTime = 0xC;					// float
	constexpr int noteChordId = 0x14;				// int, -1 = single note
	constexpr int noteChordNotesId = 0x18;			// int
	constexpr int noteSlideTo = 0x34;				// byte, 0xFF = none
	constexpr int noteSlideUnpitchTo = 0x35;		// byte, 0xFF = none
	constexpr int noteSustain = 0x3C;				// float

	// Constants the game compares against (0x0122485C, 0x012248A0, 0x012248A8).
	constexpr float tolerance = 0.002f;
	constexpr double anchorMargin = static_cast<double>(0.002f);
	constexpr double phraseMargin = 0.002;

	// Above this much original work (notes x (phrase iterations + anchors)) we skip the original loops.
	// Below it, the original runs and checks our answers. Summed over a song's levels, anything bigger adds seconds.
	constexpr uint64_t verifyWorkLimit = 2000000; // About 20-50 ms of the original loops

	template <typename T>
	T& At(uintptr_t base, int offset) {
		return *reinterpret_cast<T*>(base + offset);
	}

	uint32_t VectorCount(uintptr_t vector, uint32_t elementSize) {
		return static_cast<uint32_t>(static_cast<int>(At<uintptr_t>(vector, 4) - At<uintptr_t>(vector, 0)) / static_cast<int>(elementSize));
	}

	// The game's tolerance checks. x87 compares treat NaN as "unordered", which fails every one of these the same way.

	/// Is t at or after start, allowing t to be up to the tolerance early.
	bool NotBefore(float t, float start) {
		if (t >= start)
			return true;
		const float gap = start - t;
		return fabsf(gap) <= tolerance;
	}

	/// Is t before end, by more than margin.
	bool EndsAfter(float t, float end, double margin) {
		if (!(end > t))
			return false;
		const float remaining = end - t;
		return static_cast<double>(remaining) > margin;
	}

	/// Does a note that ends at noteEnd (time + sustain) end on the anchor start.
	bool EndsOn(float noteEnd, float start) {
		const float gap = noteEnd - start;
		return fabsf(gap) <= tolerance;
	}

	float NoteEnd(uintptr_t note) {
		const float time = At<float>(note, noteTime);
		const float sustain = At<float>(note, noteSustain);
		return time + sustain;
	}

	bool IsSustained(uintptr_t note) {
		return 0.0f < At<float>(note, noteSustain);
	}

	/// Plain note, for the "last plain note" time: no 0x40000 flag and no unpitched slide.
	bool IsPlain(uintptr_t note) {
		return (At<uint32_t>(note, noteMask) & 0x40000) == 0 && At<uint8_t>(note, noteSlideUnpitchTo) == 0xFF;
	}

	enum class Answer { No, Yes, GameThrows };

	/// Does this note (or one of its chord's notes) end in an unpitched slide. Mirrors 0x0055E33A.
	Answer EndsInUnpitchedSlide(uintptr_t note, uintptr_t song) {
		if (At<uint8_t>(note, noteSlideUnpitchTo) != 0xFF && At<uint8_t>(note, noteSlideTo) == 0xFF && (At<uint32_t>(note, noteMask) & 0x4000) == 0)
			return Answer::Yes;

		const int chordId = At<int>(note, noteChordId);
		if (chordId == -1)
			return Answer::No;

		const uintptr_t chords = song + songChords;
		if (static_cast<uint32_t>(chordId) >= VectorCount(chords, chordSize))
			return Answer::GameThrows; // Let the game's own code hit its bounds check

		const uintptr_t chord = At<uintptr_t>(chords, 0) + static_cast<uint32_t>(chordId) * chordSize;
		const uintptr_t chordNotes = At<uintptr_t>(song, songChordNotes) + static_cast<uint32_t>(At<int>(note, noteChordNotesId)) * chordNotesSize;
		for (int string = 0; string < 6; string++) {
			if (At<uint8_t>(chord, chordFrets + string) == 0xFF)
				continue;
			if (At<uint8_t>(chordNotes, chordNotesSlideUnpitchTo + string) != 0xFF && At<uint8_t>(chordNotes, chordNotesSlideTo + string) == 0xFF)
				return Answer::Yes;
		}
		return Answer::No;
	}

	/// Min or max over ranges of a fixed array.
	class RangeTree {
	public:
		void Build(const std::vector<int>& values, bool findMax) {
			isMax = findMax;
			size = values.size();
			tree.assign(size * 2, Identity());
			std::copy(values.begin(), values.end(), tree.begin() + size);
			for (size_t i = size - 1; i > 0 && i < size; i--)
				tree[i] = Pick(tree[i * 2], tree[i * 2 + 1]);
		}

		/// [first, last)
		int Query(size_t first, size_t last) const {
			int result = Identity();
			for (first += size, last += size; first < last; first /= 2, last /= 2) {
				if (first & 1)
					result = Pick(result, tree[first++]);
				if (last & 1)
					result = Pick(result, tree[--last]);
			}
			return result;
		}

		int Identity() const { return isMax ? -1 : INT_MAX; }

	private:
		int Pick(int a, int b) const { return isMax ? (a > b ? a : b) : (a < b ? a : b); }

		bool isMax = false;
		size_t size = 0;
		std::vector<int> tree;
	};

	/// The level's notes, sorted by start time and by end time (time + sustain), plus range lookups over them.
	struct NoteIndex {
		uintptr_t notes = 0;
		uint32_t count = 0;

		std::vector<float> startTimes;
		RangeTree lastNote;			// max note index, by start time
		RangeTree lastPlainNote;	// max plain note index, by start time

		std::vector<float> endTimes;
		RangeTree firstSustainedNote; // min sustained note index, by end time

		uintptr_t Note(int index) const { return notes + static_cast<uint32_t>(index) * noteSize; }

		void Build(uintptr_t levelNotesBegin, uint32_t noteCount) {
			notes = levelNotesBegin;
			count = noteCount;

			std::vector<std::pair<float, int>> sorted;
			sorted.reserve(count);
			std::vector<int> values;

			// Start times. NaN times fail every check, so they are left out.
			for (uint32_t i = 0; i < count; i++) {
				const float time = At<float>(Note(i), noteTime);
				if (time == time)
					sorted.emplace_back(time, i);
			}
			std::sort(sorted.begin(), sorted.end());
			startTimes.resize(sorted.size());
			values.resize(sorted.size());
			for (size_t i = 0; i < sorted.size(); i++) {
				startTimes[i] = sorted[i].first;
				values[i] = sorted[i].second;
			}
			lastNote.Build(values, true);
			for (size_t i = 0; i < sorted.size(); i++)
				values[i] = IsPlain(Note(sorted[i].second)) ? sorted[i].second : -1;
			lastPlainNote.Build(values, true);

			// End times
			sorted.clear();
			for (uint32_t i = 0; i < count; i++) {
				const float end = NoteEnd(Note(i));
				if (end == end)
					sorted.emplace_back(end, i);
			}
			std::sort(sorted.begin(), sorted.end());
			endTimes.resize(sorted.size());
			values.resize(sorted.size());
			for (size_t i = 0; i < sorted.size(); i++) {
				endTimes[i] = sorted[i].first;
				values[i] = IsSustained(Note(sorted[i].second)) ? sorted[i].second : INT_MAX;
			}
			firstSustainedNote.Build(values, false);
		}
	};

	struct PhraseScan {
		bool hasNotes = false;
		float lastNoteTime = 0.0f;
		bool hasPlainNote = false;
		float lastPlainNoteTime = 0.0f;
	};

	struct AnchorScan {
		bool hasNotes = false;
		Answer slideIn = Answer::No;
	};

	enum class Mode { Original, Verify, Fast };

	Mode mode = Mode::Original;
	bool disabled = false;
	NoteIndex noteIndex;

	// In fast mode, every Nth scan is also done the slow way and compared, so the index can't be quietly wrong on
	// charts too big to check against the game.
	constexpr uint32_t phraseSpotCheckEvery = 16;
	constexpr uint32_t anchorSpotCheckEvery = 64;
	uint32_t anchorScans = 0;

	struct {
		bool pending = false;
		PhraseScan result;
	} pendingPhrase;

	struct {
		bool pending = false;
		AnchorScan result;
	} pendingAnchor;

	struct {
		bool pending = false;
		bool remove = false;
	} pendingLookahead;

	uintptr_t Level(uintptr_t ebp) { return At<uintptr_t>(ebp, frameLevel); }
	uintptr_t Song(uintptr_t ebp) { return At<uintptr_t>(ebp, frameSong); }

	/// Make sure the note index is for this level's notes.
	void UseLevelNotes(uintptr_t ebp) {
		const uintptr_t notes = Level(ebp) + levelNotes;
		const uintptr_t begin = At<uintptr_t>(notes, 0);
		const uint32_t count = VectorCount(notes, noteSize);
		if (noteIndex.notes != begin || noteIndex.count != count)
			noteIndex.Build(begin, count);
	}

	/// The note loop at 0x0055DF50, one note at a time like the game.
	PhraseScan ScanPhraseFull(float phraseStart, float phraseEnd) {
		PhraseScan result;
		for (uint32_t i = 0; i < noteIndex.count; i++) {
			const uintptr_t note = noteIndex.Note(i);
			const float time = At<float>(note, noteTime);
			if (!NotBefore(time, phraseStart) || !EndsAfter(time, phraseEnd, phraseMargin))
				continue;
			result.hasNotes = true;
			result.lastNoteTime = time;
			if (IsPlain(note)) {
				result.hasPlainNote = true;
				result.lastPlainNoteTime = time;
			}
		}
		return result;
	}

	/// Same answer as ScanPhraseFull, from the note index.
	PhraseScan ScanPhrase(float phraseStart, float phraseEnd) {
		if (!std::isfinite(phraseStart) || !std::isfinite(phraseEnd))
			return ScanPhraseFull(phraseStart, phraseEnd);

		PhraseScan result;
		const auto& times = noteIndex.startTimes;
		const size_t first = std::partition_point(times.begin(), times.end(), [&](float time) { return !NotBefore(time, phraseStart); }) - times.begin();
		const size_t last = std::partition_point(times.begin(), times.end(), [&](float time) { return EndsAfter(time, phraseEnd, phraseMargin); }) - times.begin();
		if (first >= last)
			return result;

		const int lastNote = noteIndex.lastNote.Query(first, last);
		const int lastPlainNote = noteIndex.lastPlainNote.Query(first, last);
		result.hasNotes = true;
		result.lastNoteTime = At<float>(noteIndex.Note(lastNote), noteTime);
		if (lastPlainNote >= 0) {
			result.hasPlainNote = true;
			result.lastPlainNoteTime = At<float>(noteIndex.Note(lastPlainNote), noteTime);
		}
		return result;
	}

	/// The note loop at 0x0055E227, one note at a time like the game.
	AnchorScan ScanAnchorFull(uintptr_t song, float start, float nextStart) {
		AnchorScan result;
		for (uint32_t i = 0; i < noteIndex.count; i++) {
			const uintptr_t note = noteIndex.Note(i);
			const float time = At<float>(note, noteTime);
			const bool endsOnStart = EndsOn(NoteEnd(note), start);
			if ((NotBefore(time, start) && EndsAfter(time, nextStart, anchorMargin)) || endsOnStart)
				result.hasNotes = true;
			if (endsOnStart && IsSustained(note)) {
				result.slideIn = EndsInUnpitchedSlide(note, song);
				break;
			}
		}
		return result;
	}

	/// Same answer as ScanAnchorFull, from the note index.
	AnchorScan ScanAnchor(uintptr_t song, float start, float nextStart) {
		if (!std::isfinite(start) || !std::isfinite(nextStart))
			return ScanAnchorFull(song, start, nextStart);

		AnchorScan result;

		// Notes that start inside the anchor
		const auto& times = noteIndex.startTimes;
		const auto first = std::partition_point(times.begin(), times.end(), [&](float time) { return !NotBefore(time, start); });
		const auto last = std::partition_point(times.begin(), times.end(), [&](float time) { return EndsAfter(time, nextStart, anchorMargin); });
		const bool startsInside = first < last;

		// Notes that end on the anchor start. EndsOn is |end - start| <= tolerance, so split it into two monotonic halves.
		const auto& ends = noteIndex.endTimes;
		const size_t firstEnd = std::partition_point(ends.begin(), ends.end(), [&](float end) { const float gap = end - start; return gap < -tolerance; }) - ends.begin();
		const size_t lastEnd = std::partition_point(ends.begin(), ends.end(), [&](float end) { const float gap = end - start; return gap <= tolerance; }) - ends.begin();

		// The game's loop stops at the first sustained note that ends on the anchor start. That note itself counts as
		// a note in the anchor, so whatever came before it doesn't matter.
		const int stopNote = firstEnd < lastEnd ? noteIndex.firstSustainedNote.Query(firstEnd, lastEnd) : INT_MAX;
		if (stopNote != INT_MAX) {
			result.hasNotes = true;
			result.slideIn = EndsInUnpitchedSlide(noteIndex.Note(stopNote), song);
			return result;
		}

		result.hasNotes = startsInside || firstEnd < lastEnd;
		return result;
	}

	/// Mirrors 0x0055E400 to 0x0055E7DA. Yes = remove the anchor.
	Answer Lookahead(uintptr_t ebp) {
		const uintptr_t song = Song(ebp);
		const uintptr_t level = Level(ebp);
		const uintptr_t info = At<uintptr_t>(ebp, framePhraseIterationInfo);
		const int difficulty = At<int>(level, levelDifficulty);

		// Bit number <difficulty> of the phrase iteration's bit vector
		const int bit = difficulty + At<int>(info, 4);
		uintptr_t word = At<uintptr_t>(info, 0) + ((bit + ((bit >> 31) & 31)) >> 5) * 4;
		int shift = bit & 0x8000001F;
		if (shift < 0) {
			shift = ((shift - 1) | static_cast<int>(0xFFFFFFE0)) + 1;
			if (shift < 0) {
				shift += 32;
				word -= 4;
			}
		}
		if (At<uint32_t>(word, 0) & (1u << (shift & 31)))
			return Answer::Yes;
		if (difficulty != 0)
			return Answer::Yes;

		const uint32_t index = At<uint32_t>(ebp, framePhraseIterationIndex);
		const uint32_t count = At<uint32_t>(ebp, framePhraseIterationCount);
		if (index >= count - 1 || index + 1 >= count)
			return Answer::Yes;

		const uintptr_t phraseIterations = song + songPhraseIterations;
		const uint32_t phraseIterationCount = VectorCount(phraseIterations, phraseIterationSize);
		const uintptr_t anchors = level + levelAnchors;
		const uintptr_t anchor = At<uintptr_t>(ebp, frameAnchor);
		const uint32_t anchorCount = At<uint32_t>(ebp, frameAnchorCount);

		for (uint32_t phrase = index + 1; phrase < count; phrase++) {
			if (phrase >= phraseIterationCount)
				return Answer::GameThrows;
			const uintptr_t phraseIteration = At<uintptr_t>(phraseIterations, 0) + phrase * phraseIterationSize;
			const float phraseStart = At<float>(phraseIteration, phraseIterationTime);
			float phraseEnd = At<float>(song, songLength);
			if (phrase < phraseIterationCount - 1)
				phraseEnd = At<float>(phraseIteration + phraseIterationSize, phraseIterationTime);

			if (anchorCount == 0)
				continue;

			for (uint32_t other = 0; other < VectorCount(anchors, anchorSize); other++) {
				if (other >= anchorCount)
					return Answer::GameThrows;
				const uintptr_t otherAnchor = At<uintptr_t>(anchors, 0) + other * anchorSize;
				const float otherStart = At<float>(otherAnchor, anchorStart);

				// The game checks the gap to the phrase END here, not the start. Kept as is.
				if (!(otherStart >= phraseStart)) {
					const float gap = phraseEnd - otherStart;
					if (!(fabsf(gap) <= tolerance))
						continue;
				}
				if (!(otherStart < phraseEnd))
					continue;
				const float remaining = phraseEnd - otherStart;
				if (!(static_cast<double>(remaining) > phraseMargin))
					continue;

				const bool same = At<uint8_t>(otherAnchor, anchorFret) == At<uint8_t>(anchor, anchorFret) && At<int>(otherAnchor, anchorWidth) == At<int>(anchor, anchorWidth);
				return same ? Answer::No : Answer::Yes;
			}
		}
		return Answer::Yes;
	}

	void Mismatch(const std::string& what) {
		disabled = true;
		mode = Mode::Original;
		pendingPhrase.pending = false;
		pendingAnchor.pending = false;
		pendingLookahead.pending = false;
		LOG_ERROR("(ANCHOR PASS) Our " << what << " didn't match the game's. Turned the speedup off for this session." << std::endl);
	}

	bool SameFloat(float a, float b) {
		return memcmp(&a, &b, sizeof(float)) == 0;
	}

	bool SamePhraseScan(const PhraseScan& a, const PhraseScan& b) {
		return a.hasNotes == b.hasNotes && a.hasPlainNote == b.hasPlainNote &&
			(!a.hasNotes || SameFloat(a.lastNoteTime, b.lastNoteTime)) &&
			(!a.hasPlainNote || SameFloat(a.lastPlainNoteTime, b.lastPlainNoteTime));
	}

	/// First phrase iteration of a level: pick the mode and index the notes.
	void StartLevel(uintptr_t ebp) {
		pendingPhrase.pending = false;
		pendingAnchor.pending = false;
		pendingLookahead.pending = false;

		if (disabled) {
			mode = Mode::Original;
			return;
		}

		const uintptr_t level = Level(ebp);
		const uint64_t notes = VectorCount(level + levelNotes, noteSize);
		const uint64_t anchors = VectorCount(level + levelAnchors, anchorSize);
		const uint64_t phraseIterations = At<uint32_t>(ebp, framePhraseIterationCount);
		mode = notes * (phraseIterations + anchors) > verifyWorkLimit ? Mode::Fast : Mode::Verify;

		noteIndex.notes = 0;
		UseLevelNotes(ebp);

		if (mode == Mode::Fast)
			LOG_INFO("(ANCHOR PASS) Fast path for a level with " << notes << " notes, " << anchors << " anchors, " << phraseIterations << " phrase iterations" << std::endl);
	}

	// Where the hooks continue. Each "original" one runs the instruction the hook replaced and goes back.

	void __declspec(naked) phraseScanOriginal() {
		__asm {
			mov byte ptr [ebp - 0x154], 0
			push offset Offsets::ptr_anchorPassPhraseScanJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) phraseScanSkip() {
		__asm {
			fstp st(0) // Phrase start, left on the x87 stack for the loop
			push offset Offsets::ptr_anchorPassPhraseScanDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteScanOriginal() {
		__asm {
			mov eax, dword ptr [ebp - 0xF8]
			push offset Offsets::ptr_anchorPassNoteScanJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteScanSkip() {
		__asm {
			push offset Offsets::ptr_anchorPassNoteScanDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) lookaheadOriginal() {
		__asm {
			mov eax, dword ptr [ebp - 0x168]
			push offset Offsets::ptr_anchorPassLookaheadJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) lookaheadRemove() {
		__asm {
			push offset Offsets::ptr_anchorPassRemoveAnchor
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) lookaheadKeep() {
		__asm {
			push offset Offsets::ptr_anchorPassLookaheadDone
			jmp MemUtil::JumpToVersioned
		}
	}

	// Hook handlers. The ones that pick where to continue return the address to continue at.

	uintptr_t __stdcall OnPhraseScan(uintptr_t ebp) {
		if (At<uint32_t>(ebp, framePhraseIterationIndex) == 0)
			StartLevel(ebp);
		if (mode == Mode::Original)
			return reinterpret_cast<uintptr_t>(phraseScanOriginal);

		UseLevelNotes(ebp);
		const PhraseScan result = ScanPhrase(At<float>(ebp, framePhraseStart), At<float>(ebp, framePhraseEnd));
		if (mode == Mode::Verify) {
			pendingPhrase.pending = true;
			pendingPhrase.result = result;
			return reinterpret_cast<uintptr_t>(phraseScanOriginal);
		}

		if (At<uint32_t>(ebp, framePhraseIterationIndex) % phraseSpotCheckEvery == 0 && !SamePhraseScan(result, ScanPhraseFull(At<float>(ebp, framePhraseStart), At<float>(ebp, framePhraseEnd)))) {
			Mismatch("indexed phrase note scan");
			return reinterpret_cast<uintptr_t>(phraseScanOriginal);
		}

		// Both times already hold 0.0 from before the loop.
		At<uint8_t>(ebp, framePhraseHasNotes) = result.hasNotes ? 1 : 0;
		if (result.hasNotes)
			At<float>(ebp, framePhraseLastNoteTime) = result.lastNoteTime;
		if (result.hasPlainNote)
			At<float>(ebp, framePhraseLastPlainNoteTime) = result.lastPlainNoteTime;
		return reinterpret_cast<uintptr_t>(phraseScanSkip);
	}

	void __stdcall OnPhraseScanDone(uintptr_t ebp) {
		if (!pendingPhrase.pending)
			return;
		pendingPhrase.pending = false;

		const PhraseScan& ours = pendingPhrase.result;
		const bool hasNotes = At<uint8_t>(ebp, framePhraseHasNotes) != 0;
		if (hasNotes != ours.hasNotes ||
			!SameFloat(At<float>(ebp, framePhraseLastNoteTime), ours.hasNotes ? ours.lastNoteTime : 0.0f) ||
			!SameFloat(At<float>(ebp, framePhraseLastPlainNoteTime), ours.hasPlainNote ? ours.lastPlainNoteTime : 0.0f))
			Mismatch("phrase note scan");
	}

	uintptr_t __stdcall OnNoteScan(uintptr_t ebp) {
		if (mode == Mode::Original)
			return reinterpret_cast<uintptr_t>(noteScanOriginal);

		UseLevelNotes(ebp);
		const AnchorScan result = ScanAnchor(Song(ebp), At<float>(ebp, frameAnchorStart), At<float>(ebp, frameNextAnchorStart));
		if (result.slideIn == Answer::GameThrows)
			return reinterpret_cast<uintptr_t>(noteScanOriginal);

		if (mode == Mode::Verify) {
			pendingAnchor.pending = true;
			pendingAnchor.result = result;
			return reinterpret_cast<uintptr_t>(noteScanOriginal);
		}

		if (++anchorScans % anchorSpotCheckEvery == 0) {
			const AnchorScan full = ScanAnchorFull(Song(ebp), At<float>(ebp, frameAnchorStart), At<float>(ebp, frameNextAnchorStart));
			if (full.hasNotes != result.hasNotes || full.slideIn != result.slideIn) {
				Mismatch("indexed anchor note scan");
				return reinterpret_cast<uintptr_t>(noteScanOriginal);
			}
		}

		At<uint8_t>(ebp, frameAnchorHasNotes) = result.hasNotes ? 1 : 0;
		At<uint8_t>(ebp, frameAnchorSlideIn) = result.slideIn == Answer::Yes ? 1 : 0;
		return reinterpret_cast<uintptr_t>(noteScanSkip);
	}

	void __stdcall OnNoteScanDone(uintptr_t ebp) {
		if (!pendingAnchor.pending)
			return;
		pendingAnchor.pending = false;

		const bool hasNotes = At<uint8_t>(ebp, frameAnchorHasNotes) != 0;
		const bool slideIn = At<uint8_t>(ebp, frameAnchorSlideIn) != 0;
		if (hasNotes != pendingAnchor.result.hasNotes || slideIn != (pendingAnchor.result.slideIn == Answer::Yes))
			Mismatch("anchor note scan");
	}

	uintptr_t __stdcall OnLookahead(uintptr_t ebp) {
		if (mode == Mode::Original)
			return reinterpret_cast<uintptr_t>(lookaheadOriginal);

		const Answer remove = Lookahead(ebp);
		if (remove == Answer::GameThrows)
			return reinterpret_cast<uintptr_t>(lookaheadOriginal);

		if (mode == Mode::Verify) {
			pendingLookahead.pending = true;
			pendingLookahead.remove = remove == Answer::Yes;
			return reinterpret_cast<uintptr_t>(lookaheadOriginal);
		}

		return reinterpret_cast<uintptr_t>(remove == Answer::Yes ? lookaheadRemove : lookaheadKeep);
	}

	void __stdcall OnLookaheadDone(uintptr_t ebp) {
		if (!pendingLookahead.pending)
			return;
		pendingLookahead.pending = false;

		if ((At<uint8_t>(ebp, frameRemoveAnchor) != 0) != pendingLookahead.remove)
			Mismatch("anchor lookahead");
	}

	// The hooks. The routing ones leave a slot on the stack for the handler's answer and "return" into it.

	void __declspec(naked) phraseScanHook() {
		__asm {
			push 0
			pushfd
			pushad
			push ebp
			call OnPhraseScan
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	void __declspec(naked) phraseScanDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnPhraseScanDone
			popad
			popfd
			mov esi, dword ptr [ebp - 0x168]	// The code we are overwriting to place this hook
			push offset Offsets::ptr_anchorPassPhraseScanDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteScanHook() {
		__asm {
			push 0
			pushfd
			pushad
			push ebp
			call OnNoteScan
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	void __declspec(naked) noteScanDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnNoteScanDone
			popad
			popfd
			cmp byte ptr [ebp - 0xE2], 0		// The code we are overwriting to place this hook
			push offset Offsets::ptr_anchorPassNoteScanDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) lookaheadHook() {
		__asm {
			push 0
			pushfd
			pushad
			push ebp
			call OnLookahead
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	void __declspec(naked) lookaheadDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnLookaheadDone
			popad
			popfd
			cmp byte ptr [ebp - 0xE1], 0		// The code we are overwriting to place this hook
			push offset Offsets::ptr_anchorPassLookaheadDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	struct HookSite {
		VersioningStruct<uintptr_t>& address;
		int length; // Bytes of game code the hook replaces
		void* hook;
	};

	// ---------------------------------------------------------------------------------------------------------------
	// Anchor loop per phrase iteration (0x0055E0D2). The game walks the anchors with a cursor that only moves on when an
	// anchor is handled, but loops a fixed number of times: once per anchor in the level. When the anchor at the cursor
	// isn't in this phrase iteration (it belongs to a later one), the iteration skips it without moving the cursor, so
	// every remaining iteration tests the same anchor, skips it the same way, and changes nothing. That's ~10,000 wasted
	// iterations per phrase iteration per level, each calling fabsf.
	// The three "not in this phrase iteration" branches jump here instead, and we end the loop the same way the game
	// would once its counter runs out.

	void __declspec(naked) endAnchorLoop() {
		__asm {
			// Counter = anchor count - 1, so the game's own increment and compare end the loop.
			mov eax, [esi + 0x4]
			sub eax, [esi]
			xor edx, edx
			mov ecx, 0x1C // Anchor size
			div ecx
			dec eax
			mov [ebp - 0x148], eax // The loop counter
			push offset Offsets::ptr_anchorPassAnchorLoopNext
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) endAnchorLoopPopTwo() {
		__asm {
			fstp st(1) // What the game's 0x0055F380 does before the loop's next step
			fstp st(0)
			jmp endAnchorLoop
		}
	}

	/// Points the rel32 of a 6 byte Jcc at our code.
	void RedirectBranch(VersioningStruct<uintptr_t>& branch, void* target) {
		const uintptr_t at = branch.Get();
		const int32_t relative = static_cast<int32_t>(reinterpret_cast<uintptr_t>(target) - (at + 6));
		MemUtil::PatchAdr(reinterpret_cast<LPVOID>(at + 2), &relative, sizeof(relative));
	}

	void Install() {
		RedirectBranch(Offsets::ptr_anchorPassSkipBeforePhrase, endAnchorLoop);
		RedirectBranch(Offsets::ptr_anchorPassSkipAfterPhrase, endAnchorLoopPopTwo);
		RedirectBranch(Offsets::ptr_anchorPassSkipAtPhraseEnd, endAnchorLoop);

		HookSite sites[] = {
			{ Offsets::ptr_anchorPassPhraseScan,		7, phraseScanHook },
			{ Offsets::ptr_anchorPassPhraseScanDone,	6, phraseScanDoneHook },
			{ Offsets::ptr_anchorPassNoteScan,			6, noteScanHook },
			{ Offsets::ptr_anchorPassNoteScanDone,		7, noteScanDoneHook },
			{ Offsets::ptr_anchorPassLookahead,			6, lookaheadHook },
			{ Offsets::ptr_anchorPassLookaheadDone,		7, lookaheadDoneHook },
		};

		for (const HookSite& site : sites) {
			MemUtil::PlaceHook(site.address, site.hook, site.length);
			FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(site.address.Get()), site.length);
		}

		LOG_INFO("(ANCHOR PASS) Installed song load speedup for huge charts" << std::endl);
	}
}
