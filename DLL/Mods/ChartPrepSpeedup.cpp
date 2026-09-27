#include "../stdafx.h"
#include "ChartPrepSpeedup.hpp"
#include "../MemUtil.hpp"
#include <array>
#include <functional>
#include <limits>

/// <summary>
/// After the anchor pass, song load runs one more pass over the whole chart (0x0055B430 on the September 2022
/// build). It fills in, for every difficulty level:
///  - each anchor's end (the next anchor start), first / last note time and phrase iteration,
///  - each note's anchor fret / width, hand shape and arpeggio, and its neighbours in the phrase iteration,
///  - each hand shape's and arpeggio's first note and last note end,
///  - each section's per level string mask.
/// Every one of those is a "for each X, walk every note" (or every anchor) loop. With ~50,000 notes, ~10,000 anchors
/// and ~1,000 phrase iterations per level that is billions of steps, which is minutes of loading.
///
/// This replaces those loops with ones that sort once and use binary searches, and give the same results:
/// the same comparisons (x87 in single precision rounds like plain float maths), the same "first match" / "last
/// match" choices, and the anchor note times are still widened one note at a time in the game's order.
/// When the data isn't laid out the way the fast version needs (anchors or hand shapes out of order or overlapping),
/// that level uses a straight copy of the game's loop instead.
///
/// Each block is hooked at its start and end. On charts small enough for the game's loops to be quick, the game's
/// loop still runs and our results are compared with what it wrote. Any difference turns this off for the session.
/// </summary>
namespace ChartPrepSpeedup {

	// Locals in the game function's stack frame, relative to its EBP.
	constexpr int frameSong = -0x2C;
	constexpr int frameDefaultAnchorEnd = -0x44;

	// Song
	constexpr int songLevels = 0x40;				// vector, 100 byte entries
	constexpr int songPhraseIterations = 0x64;		// vector, 0x18 byte entries
	constexpr int songChords = 0x94;				// vector, 0x48 byte entries
	constexpr int songSections = 0xF4;				// vector, 0x58 byte entries

	constexpr int levelSize = 100;
	constexpr int levelAnchors = 0x0;				// vector, 0x1C byte entries
	constexpr int levelHandShapes = 0x18;			// vector, 0x14 byte entries
	constexpr int levelArpeggios = 0x24;			// vector, 0x14 byte entries
	constexpr int levelNotes = 0x30;				// vector, 0x1C8 byte entries

	constexpr int phraseIterationSize = 0x18;
	constexpr int phraseIterationStart = 0x4;		// float
	constexpr int phraseIterationEnd = 0x8;			// float

	constexpr int chordSize = 0x48;
	constexpr int chordFrets = 0x4;					// byte[6], 0xFF = unused string

	constexpr int sectionSize = 0x58;
	constexpr int sectionStart = 0x24;				// float
	constexpr int sectionEnd = 0x28;				// float
	constexpr int sectionStrings = 0x34;			// byte per level, bit per string

	constexpr int anchorSize = 0x1C;
	constexpr int anchorStart = 0x0;				// float
	constexpr int anchorEnd = 0x4;					// float
	constexpr int anchorFirstNote = 0x8;			// float
	constexpr int anchorLastNote = 0xC;				// float
	constexpr int anchorFret = 0x10;				// byte
	constexpr int anchorWidth = 0x14;				// int
	constexpr int anchorPhraseIteration = 0x18;		// int

	constexpr int spanSize = 0x14;					// Hand shapes and arpeggios
	constexpr int spanStart = 0x4;					// float
	constexpr int spanEnd = 0x8;					// float
	constexpr int spanFirstNote = 0xC;				// float
	constexpr int spanLastNoteEnd = 0x10;			// float

	constexpr int noteSize = 0x1C8;
	constexpr int noteMask = 0x0;					// uint
	constexpr int noteTime = 0xC;					// float
	constexpr int noteString = 0x10;				// byte
	constexpr int noteAnchorFret = 0x12;			// byte
	constexpr int noteAnchorWidth = 0x13;			// byte
	constexpr int noteChordId = 0x14;				// int, -1 = single note
	constexpr int noteHandShape = 0x24;				// ushort, 0xFFFF = none
	constexpr int noteArpeggio = 0x26;				// ushort, 0xFFFF = none
	constexpr int noteNextInPhrase = 0x2A;			// ushort
	constexpr int notePrevInPhrase = 0x2C;			// ushort
	constexpr int noteSustain = 0x3C;				// float

	constexpr uint32_t noteMaskArpeggio = 0x20000000;
	constexpr uint32_t noteMaskBendTail = 0x400800;	// Notes whose end gets the extra 0.1 below

	// The game looks for each anchor's end (the next anchor start) below 10000 seconds (0x0113D5C0), and uses 10000 as
	// the last phrase's end. Past 2h46m no next anchor is ever found, so every later anchor runs to the end of the song,
	// and every later note belongs to the first of them (wrong fret / zone). Install() points both of those loads at
	// this instead. 0x0113D5C0 itself is shared with unrelated code, so it stays 10000.
	float noNextAnchor = std::numeric_limits<float>::max();

	// Constant the game uses (0x01224390).
	const double bendTail = 0.10000000149011612;	// 0.1f as a double

	// Above this much original work we skip the original loops. Below it, the original runs and checks our results.
	constexpr uint64_t verifyWorkLimit = 2000000; // About 20-50 ms of the original loops

	template <typename T>
	T& At(uintptr_t base, int offset) {
		return *reinterpret_cast<T*>(base + offset);
	}

	uint32_t Count(uintptr_t vector, int elementSize) {
		return static_cast<uint32_t>(static_cast<int>(At<uintptr_t>(vector, 4) - At<uintptr_t>(vector, 0)) / elementSize);
	}

	uintptr_t Element(uintptr_t vector, int elementSize, uint32_t index) {
		return At<uintptr_t>(vector, 0) + index * elementSize;
	}

	uintptr_t Song(uintptr_t ebp) { return At<uintptr_t>(ebp, frameSong); }
	uint32_t LevelCount(uintptr_t song) { return Count(song + songLevels, levelSize); }
	uintptr_t Level(uintptr_t song, uint32_t index) { return Element(song + songLevels, levelSize, index); }

	/// e + 0.1 the way the game does it: x87 add of a double, rounded to float by the single precision control word.
	float AddBendTail(float value) {
		float result;
		__asm {
			fld value
			fadd bendTail
			fstp result
		}
		return result;
	}

	/// Values sorted with their original index. NaN values are left out (they fail every compare in these loops).
	struct Sorted {
		std::vector<float> values;
		std::vector<int> indices;

		template <typename Get>
		void Build(uint32_t count, Get get) {
			std::vector<std::pair<float, int>> pairs;
			pairs.reserve(count);
			for (uint32_t i = 0; i < count; i++) {
				float value;
				if (get(i, value) && value == value)
					pairs.emplace_back(value, static_cast<int>(i));
			}
			std::sort(pairs.begin(), pairs.end());
			values.resize(pairs.size());
			indices.resize(pairs.size());
			for (size_t i = 0; i < pairs.size(); i++) {
				values[i] = pairs[i].first;
				indices[i] = pairs[i].second;
			}
		}

		/// First position where pred(value) is false. pred must be true then false along the sorted values.
		template <typename Pred>
		size_t Until(Pred pred) const {
			return std::partition_point(values.begin(), values.end(), pred) - values.begin();
		}
	};

	/// Min over ranges of a fixed array.
	class MinTree {
	public:
		void Build(const std::vector<int>& values) {
			size = values.size();
			tree.assign(size * 2, INT_MAX);
			std::copy(values.begin(), values.end(), tree.begin() + size);
			for (size_t i = size - 1; i > 0 && i < size; i--)
				tree[i] = tree[i * 2] < tree[i * 2 + 1] ? tree[i * 2] : tree[i * 2 + 1];
		}

		int Query(size_t first, size_t last) const {
			int result = INT_MAX;
			for (first += size, last += size; first < last; first /= 2, last /= 2) {
				if (first & 1) { if (tree[first] < result) result = tree[first]; first++; }
				if (last & 1) { last--; if (tree[last] < result) result = tree[last]; }
			}
			return result;
		}

	private:
		size_t size = 0;
		std::vector<int> tree;
	};

	/// Min and max over ranges of a fixed array of floats (no NaNs).
	class FloatRange {
	public:
		void Build(const std::vector<float>& values) {
			size = values.size();
			lows.assign(size * 2, std::numeric_limits<float>::infinity());
			highs.assign(size * 2, -std::numeric_limits<float>::infinity());
			for (size_t i = 0; i < size; i++)
				lows[size + i] = highs[size + i] = values[i];
			for (size_t i = size - 1; i > 0 && i < size; i--) {
				lows[i] = lows[i * 2] < lows[i * 2 + 1] ? lows[i * 2] : lows[i * 2 + 1];
				highs[i] = highs[i * 2] > highs[i * 2 + 1] ? highs[i * 2] : highs[i * 2 + 1];
			}
		}

		/// [first, last), must not be empty
		std::pair<float, float> Query(size_t first, size_t last) const {
			float low = std::numeric_limits<float>::infinity();
			float high = -std::numeric_limits<float>::infinity();
			for (first += size, last += size; first < last; first /= 2, last /= 2) {
				if (first & 1) { low = lows[first] < low ? lows[first] : low; high = highs[first] > high ? highs[first] : high; first++; }
				if (last & 1) { last--; low = lows[last] < low ? lows[last] : low; high = highs[last] > high ? highs[last] : high; }
			}
			return { low, high };
		}

	private:
		size_t size = 0;
		std::vector<float> lows, highs;
	};

	/// "First span (in list order) with start <= time < end" for a list of spans.
	/// When the starts are in order (the spans may overlap), the spans starting at or before a time are a prefix of
	/// the list, and the answer is the leftmost one in it whose end is after the time. A max tree over the ends finds
	/// that in log time. Lists that aren't in order, or have NaN times, use the game's straight walk.
	class SpanLookup {
	public:
		void Build(uintptr_t vector, int elementSize, int startOffset, int endOffset) {
			list = vector;
			stride = elementSize;
			startAt = startOffset;
			endAt = endOffset;
			count = Count(vector, elementSize);
			starts.resize(count);
			ordered = true;
			std::vector<float> ends(count);
			for (uint32_t i = 0; i < count; i++) {
				const uintptr_t span = Element(vector, elementSize, i);
				starts[i] = At<float>(span, startOffset);
				ends[i] = At<float>(span, endOffset);
				if (starts[i] != starts[i] || ends[i] != ends[i] || (i > 0 && !(starts[i - 1] <= starts[i])))
					ordered = false;
			}
			if (!ordered)
				return;
			leaves = 1;
			while (leaves < count)
				leaves *= 2;
			highestEnd.assign(leaves * 2, -std::numeric_limits<float>::infinity());
			for (uint32_t i = 0; i < count; i++)
				highestEnd[leaves + i] = ends[i];
			for (size_t i = leaves - 1; i > 0; i--)
				highestEnd[i] = highestEnd[i * 2] > highestEnd[i * 2 + 1] ? highestEnd[i * 2] : highestEnd[i * 2 + 1];
		}

		int Find(float time) const {
			if (!ordered) {
				for (uint32_t i = 0; i < count; i++) {
					const uintptr_t span = Element(list, stride, i);
					if (At<float>(span, startAt) <= time && At<float>(span, endAt) > time)
						return static_cast<int>(i);
				}
				return -1;
			}
			const size_t prefix = std::partition_point(starts.begin(), starts.end(), [&](float start) { return start <= time; }) - starts.begin();
			return Leftmost(1, 0, leaves, prefix, time);
		}

	private:
		/// Leftmost leaf below node (covering [low, high)) with index < limit and end > time, or -1.
		int Leftmost(size_t node, size_t low, size_t high, size_t limit, float time) const {
			if (low >= limit || !(highestEnd[node] > time))
				return -1;
			if (high - low == 1)
				return static_cast<int>(low);
			const size_t middle = (low + high) / 2;
			const int left = Leftmost(node * 2, low, middle, limit, time);
			return left >= 0 ? left : Leftmost(node * 2 + 1, middle, high, limit, time);
		}

		uintptr_t list = 0;
		int stride = 0, startAt = 0, endAt = 0;
		uint32_t count = 0;
		bool ordered = false;
		size_t leaves = 1;
		std::vector<float> starts;
		std::vector<float> highestEnd;
	};

	enum class Mode { Original, Verify, Fast };
	Mode mode = Mode::Original;
	bool disabled = false;

	void Mismatch(const char* block) {
		disabled = true;
		mode = Mode::Original;
		LOG_ERROR("(CHART PREP) Our " << block << " didn't match the game's. Turned the speedup off for this session." << std::endl);
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Block 1 (0x0055B607): per anchor end, first / last note time, and phrase iteration.

	struct AnchorResult {
		std::vector<std::vector<uint8_t>> levels; // Copy of every level's anchors after the block
	};

	AnchorResult ComputeAnchors(uintptr_t ebp) {
		AnchorResult result;
		const uintptr_t song = Song(ebp);
		const float defaultEnd = At<float>(ebp, frameDefaultAnchorEnd);

		const uintptr_t phraseIterations = song + songPhraseIterations;
		const uint32_t phraseIterationCount = Count(phraseIterations, phraseIterationSize);
		const auto phraseStart = [&](uint32_t i) { return At<float>(Element(phraseIterations, phraseIterationSize, i), phraseIterationStart); };
		bool phrasesIncreasing = true;
		for (uint32_t i = 0; i < phraseIterationCount; i++) {
			const float start = phraseStart(i);
			if (start != start || (i + 1 < phraseIterationCount && !(start < phraseStart(i + 1))))
				phrasesIncreasing = false;
		}

		const uint32_t levelCount = LevelCount(song);
		result.levels.resize(levelCount);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t levelData = Level(song, level);
			const uintptr_t anchorVector = levelData + levelAnchors;
			const uint32_t anchorCount = Count(anchorVector, anchorSize);
			const uintptr_t noteVector = levelData + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);

			std::vector<uint8_t>& anchors = result.levels[level];
			anchors.resize(anchorCount * anchorSize);
			if (anchorCount)
				memcpy(anchors.data(), reinterpret_cast<void*>(At<uintptr_t>(anchorVector, 0)), anchors.size());
			const auto anchor = [&](uint32_t i) { return reinterpret_cast<uintptr_t>(anchors.data()) + i * anchorSize; };
			const auto note = [&](uint32_t i) { return Element(noteVector, noteSize, i); };

			Sorted starts;
			starts.Build(anchorCount, [&](uint32_t i, float& value) { value = At<float>(anchor(i), anchorStart); return true; });
			Sorted noteTimes;
			noteTimes.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(note(i), noteTime); return true; });

			// Note ends, for sustained notes only
			std::vector<float> noteEnds(noteCount);
			for (uint32_t i = 0; i < noteCount; i++) {
				const uintptr_t n = note(i);
				const float sustain = At<float>(n, noteSustain);
				noteEnds[i] = std::numeric_limits<float>::quiet_NaN();
				if (0.0f < sustain) {
					float end = At<float>(n, noteTime) + sustain;
					if (At<uint32_t>(n, noteMask) & noteMaskBendTail)
						end = AddBendTail(end);
					noteEnds[i] = end;
				}
			}
			Sorted sortedEnds;
			sortedEnds.Build(noteCount, [&](uint32_t i, float& value) { value = noteEnds[i]; return true; });
			std::vector<float> endNoteTimes(sortedEnds.indices.size());
			for (size_t k = 0; k < endNoteTimes.size(); k++)
				endNoteTimes[k] = At<float>(note(sortedEnds.indices[k]), noteTime);
			FloatRange endNoteTimeRange;
			endNoteTimeRange.Build(endNoteTimes);

			for (uint32_t i = 0; i < anchorCount; i++) {
				const float start = At<float>(anchor(i), anchorStart);

				// End: the smallest start after this one (first such anchor on ties), below noNextAnchor.
				// If nothing qualifies (the last anchor), it runs to the default end.
				const size_t next = starts.Until([&](float value) { return !(value > start); });
				float end = defaultEnd;
				if (next < starts.values.size() && starts.values[next] < noNextAnchor)
					end = At<float>(anchor(starts.indices[next]), anchorStart);
				At<float>(anchor(i), anchorEnd) = end;

				// The game widens the anchor's first / last note time one note at a time (0x005581F0) with:
				//  - the time of each note starting inside,
				//  - the end of each sustained note ending inside, and that note's time, clamped into the anchor.
				// One at a time that is just a min and a max, so take them over the ranges instead.
				// The clamp keeps order, so the clamped min / max are the min / max clamped.
				const auto clamp = [&](float time) {
					const float low = start < time ? time : start;
					return end > low ? low : end;
				};
				bool any = false;
				float lowest = std::numeric_limits<float>::infinity();
				float highest = -std::numeric_limits<float>::infinity();
				const auto take = [&](float low, float high) {
					any = true;
					lowest = low < lowest ? low : lowest;
					highest = high > highest ? high : highest;
				};
				const size_t firstTime = noteTimes.Until([&](float value) { return !(value >= start); });
				const size_t lastTime = noteTimes.Until([&](float value) { return end > value; });
				if (firstTime < lastTime)
					take(noteTimes.values[firstTime], noteTimes.values[lastTime - 1]); // Already inside, so the clamp keeps them
				const size_t firstEnd = sortedEnds.Until([&](float value) { return !(value >= start); });
				const size_t lastEnd = sortedEnds.Until([&](float value) { return end > value; });
				if (firstEnd < lastEnd) {
					take(sortedEnds.values[firstEnd], sortedEnds.values[lastEnd - 1]);
					const auto times = endNoteTimeRange.Query(firstEnd, lastEnd);
					take(clamp(times.first), clamp(times.second));
				}
				if (any) {
					float& first = At<float>(anchor(i), anchorFirstNote);
					float& last = At<float>(anchor(i), anchorLastNote);
					first = first < lowest ? first : lowest;
					last = last > highest ? last : highest;
				}

				// Phrase iteration: the last one with start <= anchor start < next phrase start (noNextAnchor for the last).
				const auto holds = [&](uint32_t p) {
					const float phraseEnd = p + 1 < phraseIterationCount ? phraseStart(p + 1) : noNextAnchor;
					return start >= phraseStart(p) && phraseEnd > start;
				};
				if (phrasesIncreasing) {
					uint32_t low = 0, high = phraseIterationCount;
					while (low < high) {
						const uint32_t middle = (low + high) / 2;
						if (phraseStart(middle) <= start)
							low = middle + 1;
						else
							high = middle;
					}
					if (low > 0 && holds(low - 1))
						At<int>(anchor(i), anchorPhraseIteration) = static_cast<int>(low - 1);
				}
				else {
					for (uint32_t p = 0; p < phraseIterationCount; p++) {
						if (holds(p))
							At<int>(anchor(i), anchorPhraseIteration) = static_cast<int>(p);
					}
				}
			}
		}
		return result;
	}

	void ApplyAnchors(uintptr_t ebp, const AnchorResult& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			if (!result.levels[level].empty())
				memcpy(reinterpret_cast<void*>(At<uintptr_t>(Level(song, level) + levelAnchors, 0)), result.levels[level].data(), result.levels[level].size());
		}
	}

	bool SameAnchors(uintptr_t ebp, const AnchorResult& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			if (!result.levels[level].empty() && memcmp(reinterpret_cast<void*>(At<uintptr_t>(Level(song, level) + levelAnchors, 0)), result.levels[level].data(), result.levels[level].size()) != 0)
				return false;
		}
		return true;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Note fields, for blocks 2, 4 and 5. Each keeps the final value of the fields it writes, for every note.

	struct NoteFields {
		std::vector<std::vector<uint32_t>> levels;
	};

	// Block 2 (0x0055BDCD): each note's anchor fret and width, from the first anchor holding it.
	NoteFields ComputeNoteAnchors(uintptr_t ebp) {
		NoteFields result;
		const uintptr_t song = Song(ebp);
		const uint32_t levelCount = LevelCount(song);
		result.levels.resize(levelCount);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t levelData = Level(song, level);
			const uintptr_t anchorVector = levelData + levelAnchors;
			SpanLookup anchors;
			anchors.Build(anchorVector, anchorSize, anchorStart, anchorEnd);
			const uintptr_t noteVector = levelData + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);
			auto& fields = result.levels[level];
			fields.resize(noteCount);
			for (uint32_t i = 0; i < noteCount; i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				uint8_t fret = At<uint8_t>(note, noteAnchorFret);
				uint8_t width = At<uint8_t>(note, noteAnchorWidth);
				const int anchor = anchors.Find(At<float>(note, noteTime));
				if (anchor >= 0) {
					const uintptr_t anchorData = Element(anchorVector, anchorSize, anchor);
					fret = At<uint8_t>(anchorData, anchorFret);
					width = At<uint8_t>(anchorData, anchorWidth);
				}
				fields[i] = fret | (width << 8);
			}
		}
		return result;
	}

	void ApplyNoteAnchors(uintptr_t ebp, const NoteFields& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.levels[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				At<uint8_t>(note, noteAnchorFret) = static_cast<uint8_t>(result.levels[level][i]);
				At<uint8_t>(note, noteAnchorWidth) = static_cast<uint8_t>(result.levels[level][i] >> 8);
			}
		}
	}

	bool SameNoteAnchors(uintptr_t ebp, const NoteFields& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.levels[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				if (static_cast<uint32_t>(At<uint8_t>(note, noteAnchorFret) | (At<uint8_t>(note, noteAnchorWidth) << 8)) != result.levels[level][i])
					return false;
			}
		}
		return true;
	}

	// Block 4 (0x0055C333): each note's arpeggio (and arpeggio flag) and hand shape.
	struct NoteShapes {
		std::vector<std::vector<uint32_t>> masks;
		std::vector<std::vector<uint32_t>> ids; // arpeggio | hand shape << 16
	};

	NoteShapes ComputeNoteShapes(uintptr_t ebp) {
		NoteShapes result;
		const uintptr_t song = Song(ebp);
		const uint32_t levelCount = LevelCount(song);
		result.masks.resize(levelCount);
		result.ids.resize(levelCount);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t levelData = Level(song, level);
			const uintptr_t arpeggios = levelData + levelArpeggios;
			const uintptr_t handShapes = levelData + levelHandShapes;
			SpanLookup arpeggioLookup, handShapeLookup;
			arpeggioLookup.Build(arpeggios, spanSize, spanStart, spanEnd);
			handShapeLookup.Build(handShapes, spanSize, spanStart, spanEnd);
			const uintptr_t noteVector = levelData + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);
			result.masks[level].resize(noteCount);
			result.ids[level].resize(noteCount);
			for (uint32_t i = 0; i < noteCount; i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				const float time = At<float>(note, noteTime);
				const int arpeggio = arpeggioLookup.Find(time);
				const int handShape = handShapeLookup.Find(time);
				const uint16_t arpeggioId = arpeggio >= 0 ? static_cast<uint16_t>(arpeggio) : 0xFFFF;
				const uint16_t handShapeId = handShape >= 0 ? static_cast<uint16_t>(handShape) : 0xFFFF;
				uint32_t mask = At<uint32_t>(note, noteMask);
				mask = arpeggioId != 0xFFFF ? (mask | noteMaskArpeggio) : (mask & ~noteMaskArpeggio);
				result.masks[level][i] = mask;
				result.ids[level][i] = arpeggioId | (handShapeId << 16);
			}
		}
		return result;
	}

	void ApplyNoteShapes(uintptr_t ebp, const NoteShapes& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.ids.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.ids[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				At<uint32_t>(note, noteMask) = result.masks[level][i];
				At<uint16_t>(note, noteArpeggio) = static_cast<uint16_t>(result.ids[level][i]);
				At<uint16_t>(note, noteHandShape) = static_cast<uint16_t>(result.ids[level][i] >> 16);
			}
		}
	}

	bool SameNoteShapes(uintptr_t ebp, const NoteShapes& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.ids.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.ids[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				if (At<uint32_t>(note, noteMask) != result.masks[level][i] ||
					static_cast<uint32_t>(At<uint16_t>(note, noteArpeggio) | (At<uint16_t>(note, noteHandShape) << 16)) != result.ids[level][i])
					return false;
			}
		}
		return true;
	}

	// Block 5 (0x0055C5F7): link notes that sit next to each other inside a phrase iteration.
	NoteFields ComputeNoteLinks(uintptr_t ebp) {
		NoteFields result;
		const uintptr_t song = Song(ebp);
		const uintptr_t phraseIterations = song + songPhraseIterations;
		const uint32_t phraseIterationCount = Count(phraseIterations, phraseIterationSize);
		const uint32_t levelCount = LevelCount(song);
		result.levels.resize(levelCount);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);
			const auto note = [&](uint32_t i) { return Element(noteVector, noteSize, i); };
			auto& fields = result.levels[level];
			fields.resize(noteCount);
			for (uint32_t i = 0; i < noteCount; i++)
				fields[i] = At<uint16_t>(note(i), noteNextInPhrase) | (At<uint16_t>(note(i), notePrevInPhrase) << 16);

			Sorted times;
			times.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(note(i), noteTime); return true; });

			// The game walks the notes in order and links each one to the one before it when both are inside. The
			// values written don't depend on the phrase iteration, so only which pairs get linked matters.
			for (uint32_t p = 0; p < phraseIterationCount; p++) {
				const uintptr_t phrase = Element(phraseIterations, phraseIterationSize, p);
				const float start = At<float>(phrase, phraseIterationStart);
				const float end = At<float>(phrase, phraseIterationEnd);
				const auto inside = [&](float time) { return start <= time && end > time; };
				const size_t first = times.Until([&](float value) { return !(start <= value); });
				const size_t last = times.Until([&](float value) { return end > value; });
				for (size_t k = first; k < last; k++) {
					const int j = times.indices[k];
					if (j == 0 || !inside(At<float>(note(j - 1), noteTime)))
						continue;
					fields[j - 1] = (fields[j - 1] & 0xFFFF0000) | static_cast<uint16_t>(j);
					fields[j] = (fields[j] & 0x0000FFFF) | (static_cast<uint32_t>(static_cast<uint16_t>(j - 1)) << 16);
				}
			}
		}
		return result;
	}

	void ApplyNoteLinks(uintptr_t ebp, const NoteFields& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.levels[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				At<uint16_t>(note, noteNextInPhrase) = static_cast<uint16_t>(result.levels[level][i]);
				At<uint16_t>(note, notePrevInPhrase) = static_cast<uint16_t>(result.levels[level][i] >> 16);
			}
		}
	}

	bool SameNoteLinks(uintptr_t ebp, const NoteFields& result) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.levels.size(); level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			for (uint32_t i = 0; i < result.levels[level].size(); i++) {
				const uintptr_t note = Element(noteVector, noteSize, i);
				if (static_cast<uint32_t>(At<uint16_t>(note, noteNextInPhrase) | (At<uint16_t>(note, notePrevInPhrase) << 16)) != result.levels[level][i])
					return false;
			}
		}
		return true;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Block 6 (0x0055C7AC): each hand shape's and arpeggio's first note and last note end.

	struct SpanResult {
		std::vector<std::vector<std::pair<float, float>>> handShapes; // first note, last note end
		std::vector<std::vector<std::pair<float, float>>> arpeggios;
	};

	void ComputeSpans(uintptr_t levelData, int spansOffset, std::vector<std::pair<float, float>>& out, const Sorted& times, const MinTree& firstNote, const Sorted& ends) {
		const uintptr_t spans = levelData + spansOffset;
		const uint32_t count = Count(spans, spanSize);
		const uintptr_t noteVector = levelData + levelNotes;
		out.resize(count);
		for (uint32_t s = 0; s < count; s++) {
			const uintptr_t span = Element(spans, spanSize, s);
			const float start = At<float>(span, spanStart);
			const float end = At<float>(span, spanEnd);
			float first = At<float>(span, spanFirstNote);
			float last = At<float>(span, spanLastNoteEnd);

			// First note (in order) starting inside
			const size_t firstTime = times.Until([&](float value) { return !(start <= value); });
			const size_t lastTime = times.Until([&](float value) { return end > value; });
			if (firstTime < lastTime)
				first = At<float>(Element(noteVector, noteSize, firstNote.Query(firstTime, lastTime)), noteTime);

			// Latest note end inside (first such note on ties)
			const size_t firstEnd = ends.Until([&](float value) { return !(start <= value); });
			const size_t lastEnd = ends.Until([&](float value) { return end > value; });
			if (firstEnd < lastEnd) {
				size_t best = lastEnd - 1;
				while (best > firstEnd && ends.values[best - 1] == ends.values[lastEnd - 1])
					best--;
				const float latest = ends.values[best];
				if (last != last || last < latest)
					last = latest;
			}

			if (0.0f <= first) {
				if (!(0.0f <= last))
					last = end;
			}
			else if (0.0f <= last) {
				first = start;
			}
			out[s] = { first, last };
		}
	}

	SpanResult ComputeAllSpans(uintptr_t ebp) {
		SpanResult result;
		const uintptr_t song = Song(ebp);
		const uint32_t levelCount = LevelCount(song);
		result.handShapes.resize(levelCount);
		result.arpeggios.resize(levelCount);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t levelData = Level(song, level);
			const uintptr_t noteVector = levelData + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);
			const auto note = [&](uint32_t i) { return Element(noteVector, noteSize, i); };

			Sorted times;
			times.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(note(i), noteTime); return true; });
			MinTree firstNote;
			firstNote.Build(times.indices);
			Sorted ends;
			ends.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(note(i), noteSustain) + At<float>(note(i), noteTime); return true; });

			ComputeSpans(levelData, levelHandShapes, result.handShapes[level], times, firstNote, ends);
			ComputeSpans(levelData, levelArpeggios, result.arpeggios[level], times, firstNote, ends);
		}
		return result;
	}

	void ForEachSpan(uintptr_t ebp, const SpanResult& result, const std::function<void(uintptr_t, const std::pair<float, float>&)>& action) {
		const uintptr_t song = Song(ebp);
		for (uint32_t level = 0; level < result.handShapes.size(); level++) {
			const uintptr_t levelData = Level(song, level);
			for (uint32_t s = 0; s < result.handShapes[level].size(); s++)
				action(Element(levelData + levelHandShapes, spanSize, s), result.handShapes[level][s]);
			for (uint32_t s = 0; s < result.arpeggios[level].size(); s++)
				action(Element(levelData + levelArpeggios, spanSize, s), result.arpeggios[level][s]);
		}
	}

	bool SameFloat(float a, float b) {
		return memcmp(&a, &b, sizeof(float)) == 0;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Block 7 (0x0055CB14): each section's per level mask of strings played.

	struct SectionResult {
		bool gameThrows = false;
		std::vector<std::array<uint8_t, 32>> strings;
	};

	SectionResult ComputeSections(uintptr_t ebp) {
		SectionResult result;
		const uintptr_t song = Song(ebp);
		const uintptr_t sections = song + songSections;
		const uint32_t sectionCount = Count(sections, sectionSize);
		const uintptr_t chords = song + songChords;
		const uint32_t chordCount = Count(chords, chordSize);
		result.strings.resize(sectionCount);
		for (uint32_t s = 0; s < sectionCount; s++)
			memcpy(result.strings[s].data(), reinterpret_cast<void*>(Element(sections, sectionSize, s) + sectionStrings), 32);

		const uint32_t levelCount = LevelCount(song);
		for (uint32_t level = 0; level < levelCount; level++) {
			const uintptr_t noteVector = Level(song, level) + levelNotes;
			const uint32_t noteCount = Count(noteVector, noteSize);
			const auto note = [&](uint32_t i) { return Element(noteVector, noteSize, i); };
			Sorted times;
			times.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(note(i), noteTime); return true; });

			for (uint32_t s = 0; s < sectionCount; s++) {
				const uintptr_t section = Element(sections, sectionSize, s);
				const float start = At<float>(section, sectionStart);
				const float end = At<float>(section, sectionEnd);
				const size_t first = times.Until([&](float value) { return !(value >= start); });
				const size_t last = times.Until([&](float value) { return end > value; });
				for (size_t k = first; k < last; k++) {
					const uintptr_t n = note(times.indices[k]);
					const int chordId = At<int>(n, noteChordId);
					uint8_t strings = 0;
					if (chordId == -1) {
						const uint8_t string = At<uint8_t>(n, noteString);
						if (string != 0xFF && string < 6)
							strings = static_cast<uint8_t>(1 << string);
					}
					else {
						if (static_cast<uint32_t>(chordId) >= chordCount) {
							result.gameThrows = true; // Let the game's own loop hit its bounds check
							return result;
						}
						const uintptr_t chord = Element(chords, chordSize, chordId);
						for (int string = 0; string < 6; string++) {
							if (At<uint8_t>(chord, chordFrets + string) != 0xFF)
								strings |= static_cast<uint8_t>(1 << string);
						}
					}
					if (level < 32) // The game only keeps 32 levels here
						result.strings[s][level] |= strings;
				}
			}
		}
		return result;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Hooks

	struct {
		bool anchors = false;
		bool noteAnchors = false;
		bool noteShapes = false;
		bool noteLinks = false;
		bool spans = false;
		bool sections = false;
	} pending;

	AnchorResult anchorResult;
	NoteFields noteAnchorResult;
	NoteShapes noteShapeResult;
	NoteFields noteLinkResult;
	SpanResult spanResult;
	SectionResult sectionResult;

	/// First block of the function: pick the mode for this song.
	void StartChart(uintptr_t ebp) {
		pending = {};
		if (disabled) {
			mode = Mode::Original;
			return;
		}

		const uintptr_t song = Song(ebp);
		const uint64_t phraseIterations = Count(song + songPhraseIterations, phraseIterationSize);
		const uint64_t sections = Count(song + songSections, sectionSize);
		uint64_t work = 0;
		uint64_t notes = 0;
		for (uint32_t level = 0; level < LevelCount(song); level++) {
			const uintptr_t levelData = Level(song, level);
			const uint64_t levelNoteCount = Count(levelData + levelNotes, noteSize);
			const uint64_t anchors = Count(levelData + levelAnchors, anchorSize);
			const uint64_t spans = Count(levelData + levelHandShapes, spanSize) + Count(levelData + levelArpeggios, spanSize);
			work += anchors * anchors + levelNoteCount * (anchors + spans + phraseIterations + sections);
			notes += levelNoteCount;
		}
		mode = work > verifyWorkLimit ? Mode::Fast : Mode::Verify;
		if (mode == Mode::Fast)
			LOG_INFO("(CHART PREP) Fast path for a chart with " << notes << " notes over all levels" << std::endl);
	}

	// Trampolines. "Original" runs the code the hook replaced and goes back. "Skip" goes past the game's loop.

	void __declspec(naked) anchorsOriginal() {
		__asm {
			mov ecx, dword ptr [esi + 0x44]
			fldz
			push offset Offsets::ptr_chartPrepAnchorsJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) anchorsSkip() {
		__asm {
			fldz // Left on the x87 stack by the game's loop
			push offset Offsets::ptr_chartPrepAnchorsDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteAnchorsOriginal() {
		__asm {
			mov edi, dword ptr [ebx + 0x40]
			cmp edi, dword ptr [ebx + 0x44]
			push offset Offsets::ptr_chartPrepNoteAnchorsJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteAnchorsSkip() {
		__asm {
			push offset Offsets::ptr_chartPrepNoteAnchorsDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) emptyLoopOriginal() {
		__asm {
			mov ecx, dword ptr [ebx + 0x44]
			sub ecx, dword ptr [ebx + 0x40]
			push offset Offsets::ptr_chartPrepEmptyLoopJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) emptyLoopSkip() {
		__asm {
			push offset Offsets::ptr_chartPrepEmptyLoopDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteShapesOriginal() {
		__asm {
			mov edi, dword ptr [ebp - 0x2C]
			mov eax, dword ptr [edi + 0x40]
			push offset Offsets::ptr_chartPrepNoteShapesJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteShapesSkip() {
		__asm {
			mov edi, dword ptr [ebp - 0x2C] // The code after the loop reads the song from EDI
			push offset Offsets::ptr_chartPrepNoteShapesDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteLinksOriginal() {
		__asm {
			mov eax, dword ptr [edi + 0x64]
			mov dword ptr [ebp - 0x24], eax
			push offset Offsets::ptr_chartPrepNoteLinksJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteLinksSkip() {
		__asm {
			push offset Offsets::ptr_chartPrepNoteLinksDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) spansOriginal() {
		__asm {
			mov eax, dword ptr [edi + 0x40]
			mov dword ptr [ebp - 0x14], eax
			push offset Offsets::ptr_chartPrepSpansJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) spansSkip() {
		__asm {
			push offset Offsets::ptr_chartPrepSpansDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) sectionsOriginal() {
		__asm {
			mov ecx, dword ptr [ebp - 0x2C]
			mov dword ptr [ebp - 0x24], eax
			push offset Offsets::ptr_chartPrepSectionsJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) sectionsSkip() {
		__asm {
			mov ecx, dword ptr [ebp - 0x2C] // The code after the loop reads the song from ECX
			push offset Offsets::ptr_chartPrepSectionsDone
			jmp MemUtil::JumpToVersioned
		}
	}

	template <auto original>
	uintptr_t Continue() { return reinterpret_cast<uintptr_t>(original); }

	// Block start handlers: work out the block's results, then either skip the game's loop or let it run and check.

	void ForgetPrerollLevels();

	uintptr_t __stdcall OnAnchors(uintptr_t ebp) {
		ForgetPrerollLevels(); // A new song is loading, so the gameplay indexes are stale
		StartChart(ebp);
		if (mode == Mode::Original)
			return Continue<anchorsOriginal>();
		anchorResult = ComputeAnchors(ebp);
		if (mode == Mode::Verify) {
			pending.anchors = true;
			return Continue<anchorsOriginal>();
		}
		ApplyAnchors(ebp, anchorResult);
		return Continue<anchorsSkip>();
	}

	uintptr_t __stdcall OnNoteAnchors(uintptr_t ebp) {
		if (mode == Mode::Original)
			return Continue<noteAnchorsOriginal>();
		noteAnchorResult = ComputeNoteAnchors(ebp);
		if (mode == Mode::Verify) {
			pending.noteAnchors = true;
			return Continue<noteAnchorsOriginal>();
		}
		ApplyNoteAnchors(ebp, noteAnchorResult);
		return Continue<noteAnchorsSkip>();
	}

	uintptr_t __stdcall OnEmptyLoop(uintptr_t) {
		// Counts up to the note count for every level and phrase iteration, and throws it away.
		return mode == Mode::Fast ? Continue<emptyLoopSkip>() : Continue<emptyLoopOriginal>();
	}

	uintptr_t __stdcall OnNoteShapes(uintptr_t ebp) {
		if (mode == Mode::Original)
			return Continue<noteShapesOriginal>();
		noteShapeResult = ComputeNoteShapes(ebp);
		if (mode == Mode::Verify) {
			pending.noteShapes = true;
			return Continue<noteShapesOriginal>();
		}
		ApplyNoteShapes(ebp, noteShapeResult);
		return Continue<noteShapesSkip>();
	}

	uintptr_t __stdcall OnNoteLinks(uintptr_t ebp) {
		if (mode == Mode::Original)
			return Continue<noteLinksOriginal>();
		noteLinkResult = ComputeNoteLinks(ebp);
		if (mode == Mode::Verify) {
			pending.noteLinks = true;
			return Continue<noteLinksOriginal>();
		}
		ApplyNoteLinks(ebp, noteLinkResult);
		return Continue<noteLinksSkip>();
	}

	uintptr_t __stdcall OnSpans(uintptr_t ebp) {
		if (mode == Mode::Original)
			return Continue<spansOriginal>();
		spanResult = ComputeAllSpans(ebp);
		if (mode == Mode::Verify) {
			pending.spans = true;
			return Continue<spansOriginal>();
		}
		ForEachSpan(ebp, spanResult, [](uintptr_t span, const std::pair<float, float>& values) {
			At<float>(span, spanFirstNote) = values.first;
			At<float>(span, spanLastNoteEnd) = values.second;
		});
		return Continue<spansSkip>();
	}

	uintptr_t __stdcall OnSections(uintptr_t ebp) {
		if (mode == Mode::Original)
			return Continue<sectionsOriginal>();
		sectionResult = ComputeSections(ebp);
		if (sectionResult.gameThrows)
			return Continue<sectionsOriginal>();
		if (mode == Mode::Verify) {
			pending.sections = true;
			return Continue<sectionsOriginal>();
		}
		const uintptr_t sections = Song(ebp) + songSections;
		for (uint32_t s = 0; s < sectionResult.strings.size(); s++)
			memcpy(reinterpret_cast<void*>(Element(sections, sectionSize, s) + sectionStrings), sectionResult.strings[s].data(), 32);
		return Continue<sectionsSkip>();
	}

	// Block end handlers: in verify mode, check the game wrote what we worked out.

	void __stdcall OnAnchorsDone(uintptr_t ebp) {
		if (pending.anchors && (pending.anchors = false, !SameAnchors(ebp, anchorResult)))
			Mismatch("anchor spans");
	}

	void __stdcall OnNoteAnchorsDone(uintptr_t ebp) {
		if (pending.noteAnchors && (pending.noteAnchors = false, !SameNoteAnchors(ebp, noteAnchorResult)))
			Mismatch("note anchors");
	}

	void __stdcall OnNoteShapesDone(uintptr_t ebp) {
		if (pending.noteShapes && (pending.noteShapes = false, !SameNoteShapes(ebp, noteShapeResult)))
			Mismatch("note hand shapes / arpeggios");
	}

	void __stdcall OnNoteLinksDone(uintptr_t ebp) {
		if (pending.noteLinks && (pending.noteLinks = false, !SameNoteLinks(ebp, noteLinkResult)))
			Mismatch("note links");
	}

	void __stdcall OnSpansDone(uintptr_t ebp) {
		if (!pending.spans)
			return;
		pending.spans = false;
		bool same = true;
		ForEachSpan(ebp, spanResult, [&](uintptr_t span, const std::pair<float, float>& values) {
			if (!SameFloat(At<float>(span, spanFirstNote), values.first) || !SameFloat(At<float>(span, spanLastNoteEnd), values.second))
				same = false;
		});
		if (!same)
			Mismatch("hand shape / arpeggio spans");
	}

	void __stdcall OnSectionsDone(uintptr_t ebp) {
		if (!pending.sections)
			return;
		pending.sections = false;
		const uintptr_t sections = Song(ebp) + songSections;
		for (uint32_t s = 0; s < sectionResult.strings.size(); s++) {
			if (memcmp(reinterpret_cast<void*>(Element(sections, sectionSize, s) + sectionStrings), sectionResult.strings[s].data(), 32) != 0) {
				Mismatch("section strings");
				return;
			}
		}
	}

	// Hook stubs. Block starts leave a slot on the stack for the handler's answer and "return" into it.
	// Block ends run the check, then the instructions the hook replaced.

#define CHART_PREP_START_HOOK(name, handler) \
	void __declspec(naked) name() { \
		__asm push 0 \
		__asm pushfd \
		__asm pushad \
		__asm push ebp \
		__asm call handler \
		__asm mov [esp + 36], eax \
		__asm popad \
		__asm popfd \
		__asm ret \
	}

	CHART_PREP_START_HOOK(anchorsHook, OnAnchors)
	CHART_PREP_START_HOOK(noteAnchorsHook, OnNoteAnchors)
	CHART_PREP_START_HOOK(emptyLoopHook, OnEmptyLoop)
	CHART_PREP_START_HOOK(noteShapesHook, OnNoteShapes)
	CHART_PREP_START_HOOK(noteLinksHook, OnNoteLinks)
	CHART_PREP_START_HOOK(spansHook, OnSpans)
	CHART_PREP_START_HOOK(sectionsHook, OnSections)

	void __declspec(naked) anchorsDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnAnchorsDone
			popad
			popfd
			mov ebx, dword ptr [ebp - 0x2C]		// The code we are overwriting to place this hook
			mov eax, dword ptr [ebx + 0x40]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepAnchorsDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteAnchorsDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnNoteAnchorsDone
			popad
			popfd
			mov ecx, dword ptr [ebx + 0x68]		// The code we are overwriting to place this hook
			sub ecx, dword ptr [ebx + 0x64]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepNoteAnchorsDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteShapesDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnNoteShapesDone
			popad
			popfd
			mov ebx, dword ptr [edi + 0x40]		// The code we are overwriting to place this hook
			mov dword ptr [ebp - 0x14], ebx		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepNoteShapesDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) noteLinksDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnNoteLinksDone
			popad
			popfd
			mov ebx, dword ptr [edi + 0x38]		// The code we are overwriting to place this hook
			sub ebx, dword ptr [edi + 0x34]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepNoteLinksDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) spansDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnSpansDone
			popad
			popfd
			mov ecx, dword ptr [edi + 0xF8]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepSpansDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) sectionsDoneHook() {
		__asm {
			pushfd
			pushad
			push ebp
			call OnSectionsDone
			popad
			popfd
			mov edx, dword ptr [ecx + 0x44]		// The code we are overwriting to place this hook
			sub edx, dword ptr [ecx + 0x40]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_chartPrepSectionsDoneJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Hand shape chord repeats (0x0055D490, once per level before the anchor pass). For each hand shape the game
	// looks for the first note starting on it (within 0.002), then walks on from there to the first later note with
	// the same chord that doesn't have flag 8, and marks that one as a repeat. Both walks start from the first note
	// and the second one never stops at the hand shape's end, so each hand shape walks the whole level.
	// We find both notes with lookups, then carry on in the game's own loop at the repeat note, so the game still
	// does its own checks and all of the marking.

	// Locals in 0x0055D490's frame, relative to its EBP.
	constexpr int frameHandShapeStart = -0x9B4;
	constexpr int frameHandShapeEnd = -0x9C4;
	constexpr int frameHandShapeNotes = -0x9B8;
	constexpr int frameHandShapes = -0x9A8;			// Becomes the repeat note once one is found
	constexpr int frameHandShapeOffset = -0x9AC;
	constexpr int frameHandShapeIndex = -0x9BC;
	constexpr int frameHandShapeFirstNote = -0x9A4;
	constexpr int frameHandShapeFirstTime = -0x9C0;

	constexpr int handShapeChordId = 0x0;			// int, first field of a hand shape

	constexpr uint32_t handShapeCheckEvery = 64;

	struct PushadRegisters {
		uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	};

	struct HandShapeIndex {
		uintptr_t notes = 0;
		uint32_t count = 0;
		Sorted times;
		MinTree firstNote;
		std::unordered_map<int, std::vector<int>> notesByChord; // Notes without flag 8, by chord
	} handShapeIndex;

	bool handShapesDisabled = false;
	uint32_t handShapeScans = 0;

	void BuildHandShapeIndex(uintptr_t notes, uint32_t count) {
		handShapeIndex.notes = notes;
		handShapeIndex.count = count;
		const auto note = [&](uint32_t i) { return notes + i * noteSize; };
		handShapeIndex.times.Build(count, [&](uint32_t i, float& value) { value = At<float>(note(i), noteTime); return true; });
		handShapeIndex.firstNote.Build(handShapeIndex.times.indices);
		handShapeIndex.notesByChord.clear();
		for (uint32_t i = 0; i < count; i++) {
			if ((At<uint8_t>(note(i), noteMask) & 8) == 0)
				handShapeIndex.notesByChord[At<int>(note(i), noteChordId)].push_back(static_cast<int>(i));
		}
	}

	bool StartsOn(float time, float start) {
		const float gap = time - start;
		return fabsf(gap) <= 0.002f;
	}

	/// First note (in order) starting within 0.002 of start, or -1.
	int FirstNoteOn(float start, bool full) {
		const auto note = [&](uint32_t i) { return handShapeIndex.notes + i * noteSize; };
		if (full || !std::isfinite(start)) {
			for (uint32_t i = 0; i < handShapeIndex.count; i++) {
				if (StartsOn(At<float>(note(i), noteTime), start))
					return static_cast<int>(i);
			}
			return -1;
		}
		const Sorted& times = handShapeIndex.times;
		const size_t first = times.Until([&](float value) { const float gap = value - start; return gap < -0.002f; });
		const size_t last = times.Until([&](float value) { const float gap = value - start; return gap <= 0.002f; });
		return first < last ? handShapeIndex.firstNote.Query(first, last) : -1;
	}

	/// First note from index `from` on with this chord, no flag 8, and a time after `after`, or -1.
	int NextRepeat(int from, int chord, float after, bool full) {
		const auto note = [&](uint32_t i) { return handShapeIndex.notes + i * noteSize; };
		if (full) {
			for (uint32_t i = from; i < handShapeIndex.count; i++) {
				const uintptr_t n = note(i);
				if (At<float>(n, noteTime) > after && At<int>(n, noteChordId) == chord && (At<uint8_t>(n, noteMask) & 8) == 0)
					return static_cast<int>(i);
			}
			return -1;
		}
		const auto found = handShapeIndex.notesByChord.find(chord);
		if (found == handShapeIndex.notesByChord.end())
			return -1;
		const std::vector<int>& list = found->second;
		for (auto it = std::lower_bound(list.begin(), list.end(), from); it != list.end(); ++it) {
			if (At<float>(note(*it), noteTime) > after)
				return *it;
		}
		return -1;
	}

	void __declspec(naked) handShapeOriginal() {
		__asm {
			mov ecx, dword ptr [ebp - 0x9B8]
			push offset Offsets::ptr_handShapeScanJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) handShapeNext() {
		__asm {
			push offset Offsets::ptr_handShapeNext
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) handShapeRepeat() {
		__asm {
			fld dword ptr [ebp - 0x9C4]		// Hand shape end, like the game's loop has on the x87 stack
			fld dword ptr [ebp - 0x9C0]		// First note's time
			push offset Offsets::ptr_handShapeRepeatLoop
			jmp MemUtil::JumpToVersioned
		}
	}

	uintptr_t __stdcall OnHandShapeScan(uintptr_t ebp, PushadRegisters* registers) {
		if (handShapesDisabled)
			return reinterpret_cast<uintptr_t>(handShapeOriginal);

		const uintptr_t notes = At<uintptr_t>(ebp, frameHandShapeNotes);
		const uint32_t count = registers->esi;
		if (At<uint32_t>(ebp, frameHandShapeIndex) == 0 || handShapeIndex.notes != notes || handShapeIndex.count != count)
			BuildHandShapeIndex(notes, count);

		const float start = At<float>(ebp, frameHandShapeStart);
		const float end = At<float>(ebp, frameHandShapeEnd);
		const int chord = At<int>(At<uintptr_t>(ebp, frameHandShapes) + At<int>(ebp, frameHandShapeOffset), handShapeChordId);

		const auto find = [&](bool full, int& first, int& repeat) {
			repeat = -1;
			first = FirstNoteOn(start, full);
			if (first < 0)
				return;
			const uintptr_t firstNote = notes + first * noteSize;
			const float firstTime = At<float>(firstNote, noteTime);
			if (At<int>(firstNote, noteChordId) != chord || (At<uint8_t>(firstNote, noteMask) & 8) == 0 || !(firstTime < end))
				return;
			repeat = NextRepeat(first, chord, firstTime, full);
		};

		int first, repeat;
		find(false, first, repeat);
		if (++handShapeScans % handShapeCheckEvery == 0) {
			int fullFirst, fullRepeat;
			find(true, fullFirst, fullRepeat);
			if (fullFirst != first || fullRepeat != repeat) {
				handShapesDisabled = true;
				LOG_ERROR("(CHART PREP) Our hand shape repeat lookup didn't match the game's. Turned it off for this session." << std::endl);
				return reinterpret_cast<uintptr_t>(handShapeOriginal);
			}
		}

		if (first < 0 || repeat < 0)
			return reinterpret_cast<uintptr_t>(handShapeNext);

		// State as the game has it on reaching the repeat note in its second loop.
		const uintptr_t firstNote = notes + first * noteSize;
		At<uintptr_t>(ebp, frameHandShapeFirstNote) = firstNote;
		At<float>(ebp, frameHandShapeFirstTime) = At<float>(firstNote, noteTime);
		const uintptr_t repeatNote = notes + repeat * noteSize;
		At<uintptr_t>(ebp, frameHandShapes) = repeatNote;
		registers->edi = repeatNote;
		registers->ecx = repeat;
		registers->edx = chord;
		return reinterpret_cast<uintptr_t>(handShapeRepeat);
	}

	void __declspec(naked) handShapeScanHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnHandShapeScan
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Splitting hand shapes at anchor starts (0x0055F3F0).
	// For each anchor the game walks the level's hand shapes from the first one, stopping at the first that starts at
	// or after the anchor. Only a hand shape starting more than 0.002 before the anchor and ending more than 0.002
	// after it does anything (it gets split there); every other step is a no-op. With ~10,000 anchors and ~10,000
	// hand shapes per level that is tens of millions of no-op steps per level.
	// We move the game's loop index straight to the next hand shape that gets split, or to where the walk stops.
	// The game's own code still does every split.

	constexpr int frameSplitAnchorStart = -0x10;	// float
	constexpr int frameSplitSpans = -0x4;			// vector of hand shapes
	constexpr int frameSplitLevel = -0x8;
	constexpr int frameSplitIndex = -0x1C;			// hand shape index
	constexpr int frameSplitOffset = -0xC;			// hand shape index * 0x14
	constexpr int frameSplitAnchorIndex = -0x18;
	const double splitMargin = static_cast<double>(0.002f); // 0x012248A0

	struct SplitIndex {
		uintptr_t spans = 0;
		uint32_t count = 0;
		bool ordered = false;
		std::vector<float> starts;
		size_t leaves = 1;
		std::vector<float> highestEnd; // Upper bounds: ends only go down (when split), fixed up when we look
	} splitIndex;

	bool splitDisabled = false;
	uint32_t splitLookups = 0;

	void BuildSplitIndex(uintptr_t spans, uint32_t count) {
		splitIndex.spans = spans;
		splitIndex.count = count;
		splitIndex.starts.resize(count);
		splitIndex.ordered = true;
		splitIndex.leaves = 1;
		while (splitIndex.leaves < count)
			splitIndex.leaves *= 2;
		splitIndex.highestEnd.assign(splitIndex.leaves * 2, -std::numeric_limits<float>::infinity());
		for (uint32_t i = 0; i < count; i++) {
			const uintptr_t span = spans + i * spanSize;
			splitIndex.starts[i] = At<float>(span, spanStart);
			const float end = At<float>(span, spanEnd);
			splitIndex.highestEnd[splitIndex.leaves + i] = end == end ? end : -std::numeric_limits<float>::infinity();
			if (splitIndex.starts[i] != splitIndex.starts[i] || (i > 0 && !(splitIndex.starts[i - 1] <= splitIndex.starts[i])))
				splitIndex.ordered = false;
		}
		for (size_t i = splitIndex.leaves - 1; i > 0; i--)
			splitIndex.highestEnd[i] = splitIndex.highestEnd[i * 2] > splitIndex.highestEnd[i * 2 + 1] ? splitIndex.highestEnd[i * 2] : splitIndex.highestEnd[i * 2 + 1];
	}

	/// The game's split test on a hand shape's end: more than 0.002 past the anchor start.
	bool EndsPast(float end, float anchor) {
		const float past = end - anchor;
		return static_cast<double>(past) > splitMargin;
	}

	int LeftmostEndPast(size_t node, size_t low, size_t high, size_t first, size_t limit, float anchor) {
		if (high <= first || low >= limit || !EndsPast(splitIndex.highestEnd[node], anchor))
			return -1;
		if (high - low == 1)
			return static_cast<int>(low);
		const size_t middle = (low + high) / 2;
		const int left = LeftmostEndPast(node * 2, low, middle, first, limit, anchor);
		return left >= 0 ? left : LeftmostEndPast(node * 2 + 1, middle, high, first, limit, anchor);
	}

	void SetSplitEnd(size_t index, float end) {
		size_t node = splitIndex.leaves + index;
		splitIndex.highestEnd[node] = end == end ? end : -std::numeric_limits<float>::infinity();
		for (node /= 2; node > 0; node /= 2)
			splitIndex.highestEnd[node] = splitIndex.highestEnd[node * 2] > splitIndex.highestEnd[node * 2 + 1] ? splitIndex.highestEnd[node * 2] : splitIndex.highestEnd[node * 2 + 1];
	}

	/// The game's walk from `from`: the first hand shape that either stops the walk (starts at or after the anchor)
	/// or gets split. Returns count if the walk runs off the end.
	uint32_t NextSplitStep(uint32_t from, float anchor, bool full) {
		const auto interesting = [&](uint32_t i) {
			const uintptr_t span = splitIndex.spans + i * spanSize;
			const float start = At<float>(span, spanStart);
			if (!(start < anchor) && start == start)
				return true; // Stops the walk
			const float before = anchor - start;
			return static_cast<double>(before) > splitMargin && EndsPast(At<float>(span, spanEnd), anchor);
		};
		if (full || !splitIndex.ordered) {
			for (uint32_t i = from; i < splitIndex.count; i++) {
				if (interesting(i))
					return i;
			}
			return splitIndex.count;
		}

		const auto& starts = splitIndex.starts;
		// Where the walk stops, and where the starts are no longer more than 0.002 before the anchor.
		const uint32_t stop = static_cast<uint32_t>(std::partition_point(starts.begin(), starts.end(), [&](float start) { return start < anchor; }) - starts.begin());
		const uint32_t farEnough = static_cast<uint32_t>(std::partition_point(starts.begin(), starts.end(), [&](float start) { const float before = anchor - start; return static_cast<double>(before) > splitMargin; }) - starts.begin());
		const uint32_t limit = farEnough < stop ? farEnough : stop;
		while (true) {
			const int candidate = LeftmostEndPast(1, 0, splitIndex.leaves, from, limit, anchor);
			if (candidate < 0)
				return from > stop ? from : stop;
			const float end = At<float>(splitIndex.spans + candidate * spanSize, spanEnd);
			if (EndsPast(end, anchor))
				return static_cast<uint32_t>(candidate);
			SetSplitEnd(candidate, end); // Split earlier, so its end went down
		}
	}

	void __declspec(naked) splitStepContinue() {
		__asm {
			fld dword ptr [ecx + esi + 0x4]
			fstp dword ptr [ebp - 0x20]
			push offset Offsets::ptr_splitSpanLoopJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) splitStepDone() {
		__asm {
			push offset Offsets::ptr_splitSpanLoopDone
			jmp MemUtil::JumpToVersioned
		}
	}

	uintptr_t __stdcall OnSplitSpanStep(uintptr_t ebp, PushadRegisters* registers) {
		if (splitDisabled)
			return reinterpret_cast<uintptr_t>(splitStepContinue);

		const uintptr_t spansVector = At<uintptr_t>(ebp, frameSplitSpans);
		const uintptr_t spans = At<uintptr_t>(spansVector, 0);
		const uint32_t count = Count(spansVector, spanSize);
		const uint32_t from = At<uint32_t>(ebp, frameSplitIndex);
		if ((At<uint32_t>(ebp, frameSplitAnchorIndex) == 0 && from == 0) || splitIndex.spans != spans || splitIndex.count != count)
			BuildSplitIndex(spans, count);

		const float anchor = At<float>(ebp, frameSplitAnchorStart);
		uint32_t next = NextSplitStep(from, anchor, false);
		if (++splitLookups % handShapeCheckEvery == 0) {
			const uint32_t fullNext = NextSplitStep(from, anchor, true);
			if (fullNext != next) {
				splitDisabled = true;
				LOG_ERROR("(CHART PREP) Our hand shape split walk didn't match the game's. Turned it off for this session." << std::endl);
				return reinterpret_cast<uintptr_t>(splitStepContinue);
			}
		}

		At<uint32_t>(ebp, frameSplitIndex) = next;
		At<uint32_t>(ebp, frameSplitOffset) = next * spanSize;
		registers->ecx = next * spanSize;
		if (next >= count) {
			registers->edi = At<uint32_t>(ebp, frameSplitLevel); // As the game has it leaving the loop
			return reinterpret_cast<uintptr_t>(splitStepDone);
		}
		return reinterpret_cast<uintptr_t>(splitStepContinue);
	}

	void __declspec(naked) splitSpanLoopHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnSplitSpanStep
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// During play (not load): the preroll update (0x00462170) runs every frame. For each anchor coming
	// up it looks, in another difficulty level, for the first note near the anchor's phrase start, then for the first
	// anchor holding that note, to see if the anchor changes. Both walks start at the first note / anchor of the
	// level, so the deeper into the song, the longer every frame takes (160 FPS at the start, 5 FPS 3 hours in).
	// Both searches are intervals over sorted times, so an index per level answers them directly. We then carry on at
	// the game's own fret / width comparison.

	constexpr int framePrerollPhraseStart = -0x18;	// float
	constexpr int framePrerollAnchorEnd = -0x2C;	// float
	constexpr int framePrerollNoteTime = -0x1C;		// float, the note found

	constexpr uint32_t prerollCheckEvery = 64;
	constexpr double prerollMargin = 0.002;			// 0x012248A8

	struct PrerollLevel {
		uintptr_t notes = 0;
		uint32_t noteCount = 0;
		uintptr_t anchors = 0;
		uint32_t anchorCount = 0;
		Sorted noteTimes;
		MinTree firstNote;
		bool anchorsOrdered = false;
		std::vector<float> anchorStarts;
		size_t leaves = 1;
		std::vector<float> highestAnchorEnd;
	};

	std::unordered_map<uintptr_t, PrerollLevel> prerollLevels;
	bool prerollDisabled = false;
	uint32_t prerollLookups = 0;

	void ForgetPrerollLevels() {
		prerollLevels.clear();
	}

	PrerollLevel& PrerollIndex(uintptr_t level) {
		PrerollLevel& index = prerollLevels[level];
		const uintptr_t notes = At<uintptr_t>(level + levelNotes, 0);
		const uint32_t noteCount = Count(level + levelNotes, noteSize);
		const uintptr_t anchors = At<uintptr_t>(level + levelAnchors, 0);
		const uint32_t anchorCount = Count(level + levelAnchors, anchorSize);
		if (index.notes == notes && index.noteCount == noteCount && index.anchors == anchors && index.anchorCount == anchorCount)
			return index;

		index.notes = notes;
		index.noteCount = noteCount;
		index.anchors = anchors;
		index.anchorCount = anchorCount;
		index.noteTimes.Build(noteCount, [&](uint32_t i, float& value) { value = At<float>(notes + i * noteSize, noteTime); return true; });
		index.firstNote.Build(index.noteTimes.indices);

		index.anchorStarts.resize(anchorCount);
		std::vector<float> ends(anchorCount);
		index.anchorsOrdered = true;
		for (uint32_t i = 0; i < anchorCount; i++) {
			index.anchorStarts[i] = At<float>(anchors + i * anchorSize, anchorStart);
			ends[i] = At<float>(anchors + i * anchorSize, anchorEnd);
			if (index.anchorStarts[i] != index.anchorStarts[i] || ends[i] != ends[i] || (i > 0 && !(index.anchorStarts[i - 1] <= index.anchorStarts[i])))
				index.anchorsOrdered = false;
		}
		index.leaves = 1;
		while (index.leaves < anchorCount)
			index.leaves *= 2;
		index.highestAnchorEnd.assign(index.leaves * 2, -std::numeric_limits<float>::infinity());
		for (uint32_t i = 0; i < anchorCount; i++)
			index.highestAnchorEnd[index.leaves + i] = ends[i];
		for (size_t i = index.leaves - 1; i > 0; i--)
			index.highestAnchorEnd[i] = index.highestAnchorEnd[i * 2] > index.highestAnchorEnd[i * 2 + 1] ? index.highestAnchorEnd[i * 2] : index.highestAnchorEnd[i * 2 + 1];
		return index;
	}

	/// The game's note test at 0x004624D3: inside [start, end) by more than 0.002, or within 0.002 of start.
	bool PrerollNoteMatches(float time, float start, float end) {
		if (time >= start && end > time) {
			const float remaining = end - time;
			if (static_cast<double>(remaining) > prerollMargin)
				return true;
		}
		const float gap = time - start;
		return fabsf(gap) <= 0.002f;
	}

	/// The game's anchor test at 0x00462553: holds time (and not within 0.002 of its end), or starts within 0.002 of it.
	bool PrerollAnchorMatches(float start, float end, float time) {
		if (start <= time && end > time) {
			const float remaining = end - time;
			if (static_cast<double>(remaining) > static_cast<double>(0.002f))
				return true;
		}
		const float gap = start - time;
		return fabsf(gap) <= 0.002f;
	}

	int FirstPrerollNote(const PrerollLevel& index, float start, float end, bool full) {
		if (full) {
			for (uint32_t i = 0; i < index.noteCount; i++) {
				if (PrerollNoteMatches(At<float>(index.notes + i * noteSize, noteTime), start, end))
					return static_cast<int>(i);
			}
			return -1;
		}
		const Sorted& times = index.noteTimes;
		int best = INT_MAX;
		// Inside [start, end) by more than 0.002
		const size_t first = times.Until([&](float value) { return !(value >= start); });
		const size_t last = times.Until([&](float value) { if (!(end > value)) return false; const float remaining = end - value; return static_cast<double>(remaining) > prerollMargin; });
		if (first < last)
			best = index.firstNote.Query(first, last);
		// Within 0.002 of start
		const size_t closeFirst = times.Until([&](float value) { const float gap = value - start; return gap < -0.002f; });
		const size_t closeLast = times.Until([&](float value) { const float gap = value - start; return gap <= 0.002f; });
		if (closeFirst < closeLast) {
			const int candidate = index.firstNote.Query(closeFirst, closeLast);
			best = candidate < best ? candidate : best;
		}
		return best == INT_MAX ? -1 : best;
	}

	int LeftmostAnchorEnd(const PrerollLevel& index, size_t node, size_t low, size_t high, size_t limit, float time) {
		const float highest = index.highestAnchorEnd[node];
		if (low >= limit || !(highest > time))
			return -1;
		const float remaining = highest - time;
		if (!(static_cast<double>(remaining) > static_cast<double>(0.002f)))
			return -1;
		if (high - low == 1)
			return static_cast<int>(low);
		const size_t middle = (low + high) / 2;
		const int left = LeftmostAnchorEnd(index, node * 2, low, middle, limit, time);
		return left >= 0 ? left : LeftmostAnchorEnd(index, node * 2 + 1, middle, high, limit, time);
	}

	int FirstPrerollAnchor(const PrerollLevel& index, float time, bool full) {
		if (full || !index.anchorsOrdered) {
			for (uint32_t i = 0; i < index.anchorCount; i++) {
				const uintptr_t anchor = index.anchors + i * anchorSize;
				if (PrerollAnchorMatches(At<float>(anchor, anchorStart), At<float>(anchor, anchorEnd), time))
					return static_cast<int>(i);
			}
			return -1;
		}
		const auto& starts = index.anchorStarts;
		// Holding the time: among the anchors starting at or before it, the leftmost whose end is far enough past it.
		const size_t prefix = std::partition_point(starts.begin(), starts.end(), [&](float start) { return start <= time; }) - starts.begin();
		int best = LeftmostAnchorEnd(index, 1, 0, index.leaves, prefix, time);
		// Starting within 0.002 of it: the starts are in order, so the leftmost is the first in range.
		const size_t closeFirst = std::partition_point(starts.begin(), starts.end(), [&](float start) { const float gap = start - time; return gap < -0.002f; }) - starts.begin();
		const size_t closeLast = std::partition_point(starts.begin(), starts.end(), [&](float start) { const float gap = start - time; return gap <= 0.002f; }) - starts.begin();
		if (closeFirst < closeLast && (best < 0 || static_cast<int>(closeFirst) < best))
			best = static_cast<int>(closeFirst);
		return best;
	}

	void __declspec(naked) prerollOriginal() {
		__asm {
			mov esi, dword ptr [edx + 0x30]
			fstp st(0)
			push offset Offsets::ptr_prerollNoteSearchJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) prerollNotFound() {
		__asm {
			fstp st(0)
			push offset Offsets::ptr_prerollNextAnchor
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) prerollFound() {
		__asm {
			fstp st(0)
			push offset Offsets::ptr_prerollAnchorFound
			jmp MemUtil::JumpToVersioned
		}
	}

	uintptr_t __stdcall OnPrerollNoteSearch(uintptr_t ebp, PushadRegisters* registers) {
		if (prerollDisabled)
			return reinterpret_cast<uintptr_t>(prerollOriginal);

		const PrerollLevel& index = PrerollIndex(registers->edx);
		const float start = At<float>(ebp, framePrerollPhraseStart);
		const float end = At<float>(ebp, framePrerollAnchorEnd);

		const auto find = [&](bool full, int& note, int& anchor) {
			anchor = -1;
			note = FirstPrerollNote(index, start, end, full);
			if (note >= 0)
				anchor = FirstPrerollAnchor(index, At<float>(index.notes + note * noteSize, noteTime), full);
		};

		int note, anchor;
		find(false, note, anchor);
		if (++prerollLookups % prerollCheckEvery == 0) {
			int fullNote, fullAnchor;
			find(true, fullNote, fullAnchor);
			if (fullNote != note || fullAnchor != anchor) {
				prerollDisabled = true;
				LOG_ERROR("(CHART PREP) Our preroll anchor lookup didn't match the game's. Turned it off for this session." << std::endl);
				return reinterpret_cast<uintptr_t>(prerollOriginal);
			}
		}

		if (note < 0 || anchor < 0)
			return reinterpret_cast<uintptr_t>(prerollNotFound);

		At<float>(ebp, framePrerollNoteTime) = At<float>(index.notes + note * noteSize, noteTime);
		registers->esi = index.anchors + anchor * anchorSize;
		return reinterpret_cast<uintptr_t>(prerollFound);
	}

	void __declspec(naked) prerollNoteSearchHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnPrerollNoteSearch
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// Anchor phrase starts, on song load (loop at 0x0055AECD in the song load finishing step, before 0x0055B430).
	// Per level the game fills a table with one { float phraseStart = -1, byte alone = 0 } per anchor. For every anchor
	// it walks every phrase iteration; for each one holding the anchor's start (starts at or before it, or within
	// 0.002 after it, and ends more than 0.002 after it) it:
	//  - stores that phrase iteration's start in the anchor's entry, then
	//  - walks every anchor's entry, and if none within 0.002 of that start belongs to an anchor with a different
	//    start, sets the anchor's "alone" byte.
	// That's anchors x phrase iterations (with fabsf) plus anchors x anchors, per level.
	// With the phrase iterations in order, the ones holding an anchor are a run found with two binary searches, and
	// "any entry near this start with a different anchor start" is a lookup in a map of stored starts.

	constexpr int songLength = 0x148;				// float
	constexpr int phraseStartEntrySize = 8;			// { float start, byte alone }
	constexpr float phraseStartTolerance = 0.002f;	// 0x0122485C
	constexpr uint32_t phraseStartCheckEvery = 256;

	bool phraseStartsDisabled = false;
	uint32_t phraseStartAnchors = 0;

	/// Starts at or before the anchor, or within 0.002 after it (0x0055AF69).
	bool PhraseStartsInTime(float phraseStart, float anchor) {
		if (phraseStart <= anchor)
			return true;
		const float gap = anchor - phraseStart;
		return fabsf(gap) <= phraseStartTolerance;
	}

	/// Ends more than 0.002 after the anchor (0x0055AFA5).
	bool PhraseEndsAfter(float phraseEnd, float anchor) {
		if (!(anchor < phraseEnd))
			return false;
		const float remaining = phraseEnd - anchor;
		return static_cast<double>(remaining) > static_cast<double>(0.002f);
	}

	/// Entry start within 0.002 of the phrase start (0x0055B024).
	bool NearPhraseStart(float entry, float phraseStart) {
		const float gap = entry - phraseStart;
		return fabsf(gap) <= phraseStartTolerance;
	}

	/// Stored phrase start -> anchor start -> how many entries.
	using PhraseStartMap = std::map<float, std::map<float, uint32_t>>;

	bool DifferentAnchorNear(const PhraseStartMap& stored, float phraseStart, float anchor) {
		for (auto it = stored.lower_bound(phraseStart - 1.0f); it != stored.end() && it->first <= phraseStart + 1.0f; ++it) {
			if (!NearPhraseStart(it->first, phraseStart))
				continue;
			const auto& starts = it->second;
			if (starts.size() > 1 || !(starts.begin()->first == anchor))
				return true;
		}
		return false;
	}

	void __declspec(naked) phraseStartsOriginal() {
		__asm {
			mov dword ptr [esp + 0x38], 0
			push offset Offsets::ptr_anchorPhraseStartsJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) phraseStartsDone() {
		__asm {
			push offset Offsets::ptr_anchorPhraseStartsDone
			jmp MemUtil::JumpToVersioned
		}
	}

	uintptr_t __stdcall OnAnchorPhraseStarts(uintptr_t, PushadRegisters* registers) {
		if (phraseStartsDisabled)
			return reinterpret_cast<uintptr_t>(phraseStartsOriginal);

		const uintptr_t esp = registers->esp + 8; // Before our push 0 / pushfd
		const uintptr_t song = registers->ebx;
		const uintptr_t anchors = registers->esi;
		const uint32_t anchorCount = registers->eax;
		const uintptr_t entryVector = At<uintptr_t>(esp, 0x3C);
		const uintptr_t entries = At<uintptr_t>(entryVector, 0);
		if (Count(entryVector, phraseStartEntrySize) < anchorCount)
			return reinterpret_cast<uintptr_t>(phraseStartsOriginal); // The game's bounds check throws

		const uintptr_t phraseVector = song + songPhraseIterations;
		const uint32_t phraseCount = Count(phraseVector, phraseIterationSize);
		const float length = At<float>(song, songLength);
		const auto startOfAnchor = [&](uint32_t i) { return At<float>(anchors + i * anchorSize, anchorStart); };
		const auto entryStart = [&](uint32_t i) -> float& { return At<float>(entries + i * phraseStartEntrySize, 0); };
		const auto entryAlone = [&](uint32_t i) -> uint8_t& { return At<uint8_t>(entries + i * phraseStartEntrySize, 4); };

		// Phrase starts and ends in order, no NaNs; anchors without NaNs
		std::vector<float> starts(phraseCount), ends(phraseCount);
		for (uint32_t j = 0; j < phraseCount; j++) {
			starts[j] = At<float>(Element(phraseVector, phraseIterationSize, j), phraseIterationStart);
			if (!std::isfinite(starts[j]) || (j > 0 && !(starts[j - 1] <= starts[j])))
				return reinterpret_cast<uintptr_t>(phraseStartsOriginal);
		}
		for (uint32_t j = 0; j < phraseCount; j++)
			ends[j] = j + 1 < phraseCount ? starts[j + 1] : length;
		if (phraseCount > 0 && !(std::isfinite(length) && ends[phraseCount - 1] >= starts[phraseCount - 1] && (phraseCount < 2 || ends[phraseCount - 1] >= ends[phraseCount - 2])))
			return reinterpret_cast<uintptr_t>(phraseStartsOriginal);
		PhraseStartMap stored;
		for (uint32_t i = 0; i < anchorCount; i++) {
			if (!std::isfinite(startOfAnchor(i)) || !std::isfinite(entryStart(i)))
				return reinterpret_cast<uintptr_t>(phraseStartsOriginal);
			stored[entryStart(i)][startOfAnchor(i)]++;
		}

		// The game's way for one anchor, straight from the table
		const auto gameWay = [&](uint32_t i) {
			const float anchor = startOfAnchor(i);
			for (uint32_t j = 0; j < phraseCount; j++) {
				if (!PhraseStartsInTime(starts[j], anchor) || !PhraseEndsAfter(ends[j], anchor))
					continue;
				entryStart(i) = starts[j];
				bool alone = true;
				for (uint32_t k = 0; k < anchorCount && alone; k++) {
					if (NearPhraseStart(entryStart(k), starts[j]) && !(anchor == startOfAnchor(k)))
						alone = false;
				}
				if (alone)
					entryAlone(i) = 1;
			}
		};

		bool useGameWay = false;
		for (uint32_t i = 0; i < anchorCount; i++) {
			if (useGameWay) {
				gameWay(i);
				continue;
			}

			const float anchor = startOfAnchor(i);
			const uint32_t inTime = static_cast<uint32_t>(std::partition_point(starts.begin(), starts.end(), [&](float start) { return PhraseStartsInTime(start, anchor); }) - starts.begin());
			const uint32_t endsAfter = static_cast<uint32_t>(std::partition_point(ends.begin(), ends.end(), [&](float end) { return !PhraseEndsAfter(end, anchor); }) - ends.begin());
			const float oldStart = entryStart(i);
			const uint8_t oldAlone = entryAlone(i);

			for (uint32_t j = endsAfter; j < inTime; j++) {
				float& entry = entryStart(i);
				auto old = stored.find(entry);
				if (--old->second[anchor] == 0) {
					old->second.erase(anchor);
					if (old->second.empty())
						stored.erase(old);
				}
				entry = starts[j];
				stored[entry][anchor]++;
				if (!DifferentAnchorNear(stored, starts[j], anchor))
					entryAlone(i) = 1;
			}

			if (++phraseStartAnchors % phraseStartCheckEvery == 0) {
				const float ourStart = entryStart(i);
				const uint8_t ourAlone = entryAlone(i);
				entryStart(i) = oldStart;
				entryAlone(i) = oldAlone;
				gameWay(i);
				if (!SameFloat(ourStart, entryStart(i)) || ourAlone != entryAlone(i)) {
					// Everything before this anchor was right, and the game's result for it is in place now.
					// Finish the level the game's way, and don't use this again this session.
					phraseStartsDisabled = true;
					useGameWay = true;
					LOG_ERROR("(CHART PREP) Our anchor phrase start lookup didn't match the game's. Turned it off for this session." << std::endl);
				}
			}
		}
		return reinterpret_cast<uintptr_t>(phraseStartsDone);
	}

	void __declspec(naked) anchorPhraseStartsHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnAnchorPhraseStarts
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// During play (not load): the anchor zone update (0x007E5260), every frame.
	// For anchors near the screen, the game looks up which fret / width the phrase's harder level uses there, with a
	// walk over every anchor of that level (two copies of it, 0x007E5780 and 0x007E5A10). The walk keeps the last
	// anchor that:
	//  - starts at or after the phrase start, or the phrase starts within 0.002 of the anchor being looked at, and
	//  - starts before the phrase end by more than 0.002.
	// That is ~10,000 anchors (each one before the phrase calls fabsf) per lookup, many lookups per frame, and it gets
	// slower the further into the song you are. With the starts in order the matching anchors are one run of the
	// list, so the last one is found with two binary searches. We do the lookup and skip the walk.

	constexpr uint32_t zoneCheckEvery = 128;
	constexpr float zoneNearPhrase = 0.002f;			// 0x0122485C
	const double zoneMargin = static_cast<double>(0.002f); // 0x012248A0

	/// Where each copy of the walk keeps its values, relative to ESP at the hook.
	struct ZoneWalk {
		int phraseStart;	// float
		int phraseEnd;		// float
		int anchorStart;	// float, the anchor being looked at (outside the walk)
		int width;			// dword, the result (the fret is BL)
	};

	struct ZoneAnchors {
		uint32_t count = 0;
		bool ordered = false;
		std::vector<float> starts;
	};

	std::unordered_map<uintptr_t, ZoneAnchors> zoneAnchors;
	bool zoneDisabled = false;
	uint32_t zoneLookups = 0;

	const ZoneAnchors& ZoneIndex(uintptr_t anchors, uint32_t count) {
		ZoneAnchors& index = zoneAnchors[anchors];
		// Cheap check that the list is still the one we indexed (lists get freed and reused between songs)
		if (index.count == count && (count == 0 || (index.starts[0] == At<float>(anchors, anchorStart)
			&& index.starts[count / 2] == At<float>(anchors + (count / 2) * anchorSize, anchorStart)
			&& index.starts[count - 1] == At<float>(anchors + (count - 1) * anchorSize, anchorStart))))
			return index;

		index.count = count;
		index.starts.resize(count);
		index.ordered = true;
		for (uint32_t i = 0; i < count; i++) {
			index.starts[i] = At<float>(anchors + i * anchorSize, anchorStart);
			if (index.starts[i] != index.starts[i] || (i > 0 && !(index.starts[i - 1] <= index.starts[i])))
				index.ordered = false;
		}
		return index;
	}

	/// The game's test for one anchor start, `closeToPhrase` being the phrase-start-within-0.002 part.
	bool ZoneAnchorMatches(float start, float phraseStart, float phraseEnd, bool closeToPhrase) {
		if (!(start >= phraseStart) && !closeToPhrase)
			return false;
		if (!(phraseEnd > start))
			return false;
		const float before = phraseEnd - start;
		return static_cast<double>(before) > zoneMargin;
	}

	int LastZoneAnchor(uintptr_t anchors, uint32_t count, float phraseStart, float phraseEnd, bool closeToPhrase, bool full) {
		const ZoneAnchors* index = full ? nullptr : &ZoneIndex(anchors, count);
		if (full || !index->ordered) {
			int last = -1;
			for (uint32_t i = 0; i < count; i++) {
				if (ZoneAnchorMatches(At<float>(anchors + i * anchorSize, anchorStart), phraseStart, phraseEnd, closeToPhrase))
					last = static_cast<int>(i);
			}
			return last;
		}
		const auto& starts = index->starts;
		const size_t first = closeToPhrase ? 0 : std::partition_point(starts.begin(), starts.end(), [&](float start) { return !(start >= phraseStart); }) - starts.begin();
		const size_t end = std::partition_point(starts.begin(), starts.end(), [&](float start) {
			if (!(phraseEnd > start))
				return false;
			const float before = phraseEnd - start;
			return static_cast<double>(before) > zoneMargin;
		}) - starts.begin();
		return first < end ? static_cast<int>(end - 1) : -1;
	}

	void __declspec(naked) zoneWalkADone() {
		__asm {
			push offset Offsets::ptr_anchorZoneWalkADone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) zoneWalkBDone() {
		__asm {
			push offset Offsets::ptr_anchorZoneWalkBDone
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) zoneWalkAOriginal() {
		__asm {
			cmp esi, edi
			jz empty
			push offset Offsets::ptr_anchorZoneWalkAJmpBck
			jmp MemUtil::JumpToVersioned
		empty:
			push offset Offsets::ptr_anchorZoneWalkAEmpty
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) zoneWalkBOriginal() {
		__asm {
			cmp esi, edi
			jz empty
			push offset Offsets::ptr_anchorZoneWalkBJmpBck
			jmp MemUtil::JumpToVersioned
		empty:
			push offset Offsets::ptr_anchorZoneWalkBEmpty
			jmp MemUtil::JumpToVersioned
		}
	}

	/// Returns false if we left it to the game.
	bool OnZoneWalk(PushadRegisters* registers, const ZoneWalk& walk) {
		if (zoneDisabled)
			return false;

		const uintptr_t esp = registers->esp + 8; // Before our push 0 / pushfd
		const uintptr_t anchors = registers->esi;
		const uint32_t count = (registers->edi - registers->esi) / anchorSize;
		const float phraseStart = At<float>(esp, walk.phraseStart);
		const float phraseEnd = At<float>(esp, walk.phraseEnd);
		const float gap = phraseStart - At<float>(esp, walk.anchorStart);
		const bool closeToPhrase = fabsf(gap) <= zoneNearPhrase;

		const int last = LastZoneAnchor(anchors, count, phraseStart, phraseEnd, closeToPhrase, false);
		if (++zoneLookups % zoneCheckEvery == 0 && LastZoneAnchor(anchors, count, phraseStart, phraseEnd, closeToPhrase, true) != last) {
			zoneDisabled = true;
			LOG_ERROR("(CHART PREP) Our anchor zone lookup didn't match the game's. Turned it off for this session." << std::endl);
			return false;
		}

		if (last >= 0) {
			const uintptr_t anchor = anchors + last * anchorSize;
			registers->ebx = (registers->ebx & ~0xFFu) | At<uint8_t>(anchor, anchorFret);
			At<uint32_t>(esp, walk.width) = At<uint32_t>(anchor, anchorWidth);
		}
		registers->esi = registers->edi; // As the game leaves the walk
		return true;
	}

	uintptr_t __stdcall OnZoneWalkA(uintptr_t, PushadRegisters* registers) {
		static const ZoneWalk walk = { 0x28, 0x58, 0x24, 0x3C };
		return OnZoneWalk(registers, walk) ? reinterpret_cast<uintptr_t>(zoneWalkADone) : reinterpret_cast<uintptr_t>(zoneWalkAOriginal);
	}

	uintptr_t __stdcall OnZoneWalkB(uintptr_t, PushadRegisters* registers) {
		static const ZoneWalk walk = { 0x3C, 0x54, 0x24, 0x28 };
		return OnZoneWalk(registers, walk) ? reinterpret_cast<uintptr_t>(zoneWalkBDone) : reinterpret_cast<uintptr_t>(zoneWalkBOriginal);
	}

	void __declspec(naked) zoneWalkAHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnZoneWalkA
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	void __declspec(naked) zoneWalkBHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnZoneWalkB
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	struct HookSite {
		VersioningStruct<uintptr_t>& address;
		int length; // Bytes of game code the hook replaces
		void* hook;
	};

	void Install() {
		HookSite sites[] = {
			{ Offsets::ptr_chartPrepAnchors,			5, anchorsHook },
			{ Offsets::ptr_chartPrepAnchorsDone,		6, anchorsDoneHook },
			{ Offsets::ptr_chartPrepNoteAnchors,		6, noteAnchorsHook },
			{ Offsets::ptr_chartPrepNoteAnchorsDone,	6, noteAnchorsDoneHook },
			{ Offsets::ptr_chartPrepEmptyLoop,			6, emptyLoopHook },
			{ Offsets::ptr_chartPrepNoteShapes,			6, noteShapesHook },
			{ Offsets::ptr_chartPrepNoteShapesDone,		6, noteShapesDoneHook },
			{ Offsets::ptr_chartPrepNoteLinks,			6, noteLinksHook },
			{ Offsets::ptr_chartPrepNoteLinksDone,		6, noteLinksDoneHook },
			{ Offsets::ptr_chartPrepSpans,				6, spansHook },
			{ Offsets::ptr_chartPrepSpansDone,			6, spansDoneHook },
			{ Offsets::ptr_chartPrepSections,			6, sectionsHook },
			{ Offsets::ptr_chartPrepSectionsDone,		6, sectionsDoneHook },
			{ Offsets::ptr_handShapeScan,				6, handShapeScanHook },
			{ Offsets::ptr_prerollNoteSearch,			5, prerollNoteSearchHook },
			{ Offsets::ptr_splitSpanLoop,				7, splitSpanLoopHook },
			{ Offsets::ptr_anchorPhraseStarts,			8, anchorPhraseStartsHook },
			{ Offsets::ptr_anchorZoneWalkA,				8, zoneWalkAHook },
			{ Offsets::ptr_anchorZoneWalkB,				8, zoneWalkBHook },
		};

		for (const HookSite& site : sites) {
			MemUtil::PlaceHook(site.address, site.hook, site.length);
			FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(site.address.Get()), site.length);
		}

		// FLD float ptr [0x0113D5C0] -> FLD float ptr [noNextAnchor]
		const float* limit = &noNextAnchor;
		for (VersioningStruct<uintptr_t>* load : { &Offsets::ptr_anchorEndSearchLimit, &Offsets::ptr_lastPhraseEndLimit })
			MemUtil::PatchAdr(reinterpret_cast<LPVOID>(load->Get() + 2), &limit, sizeof(limit));

		LOG_INFO("(CHART PREP) Installed song load speedup for huge charts" << std::endl);
	}
}
