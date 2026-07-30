// Standalone measurement harness for DelayLinePitchShifter.
// Feeds synthetic guitar-like signals through the shifter and reports:
//  - pop/click events: sample-to-sample discontinuities far above the local envelope
//  - artifact energy: output power outside the expected shifted harmonic series
//  - pitch accuracy: dominant output frequency vs expected
//
// WAV mode: harness.exe <input.wav> <semitones> [output.wav]
// Runs a recorded DI take through the shifter and optionally writes the result.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <windows.h>

// After the system headers: the project headers assume stdafx.h has already
// provided <cstdint> and <string>, which a standalone compile does not.
#include "DelayLinePitchShifter.hpp"

namespace
{
	constexpr double SAMPLE_RATE = 48000.0;
	constexpr double PI = 3.14159265358979323846;

	// Callback size used when feeding the shifter, in frames. The shifter is a
	// per-sample loop with no per-callback state, so results are identical at any
	// chunk size; this exists to demonstrate that against real ASIO buffer sizes
	// (96, 128, 256...) when users suspect their buffer setting.
	int chunkFrames = 96;

	std::mt19937 rng(1337);

	bool EndsWithWavExtension(const char* path)
	{
		const size_t length = std::strlen(path);
		if (length < 4) return false;

		const char* extension = path + length - 4;
		return (extension[0] == '.') &&
			(extension[1] == 'w' || extension[1] == 'W') &&
			(extension[2] == 'a' || extension[2] == 'A') &&
			(extension[3] == 'v' || extension[3] == 'V');
	}

	std::string DefaultShiftedPath(const char* inputPath)
	{
		const char* fileName = inputPath;
		for (const char* cursor = inputPath; *cursor; ++cursor)
		{
			if (*cursor == '\\' || *cursor == '/') fileName = cursor + 1;
		}

		std::string outputPath(inputPath, fileName - inputPath);
		outputPath += "shifted_";
		outputPath += fileName;
		return outputPath;
	}

	void WaitIfLaunchedByExplorer()
	{
		DWORD processes[2];
		if (GetConsoleProcessList(processes, 2) == 1)
		{
			std::printf("\nPress Enter to close.\n");
			std::getchar();
		}
	}

	// Karplus-Strong pluck: noise burst through the classic averaging loop.
	// Has a genuine attack transient and natural harmonic decay.
	std::vector<float> Pluck(double frequency, double seconds, float level)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		const int period = (int)(SAMPLE_RATE / frequency + 0.5);

		std::vector<float> line(period);
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		for (auto& s : line) s = dist(rng);

		std::vector<float> out(total);
		int pos = 0;

		for (int n = 0; n < total; ++n)
		{
			const float current = line[pos];
			const float next = line[(pos + 1) % period];
			line[pos] = 0.498f * (current + next);
			out[n] = current * level;
			pos = (pos + 1) % period;
		}

		return out;
	}

	// Harmonic sum with exponential decay and mild vibrato - a controlled "held note".
	std::vector<float> HeldNote(double frequency, double seconds, float level, double vibratoHz, double vibratoCents)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		std::vector<float> out(total);

		const double amps[6] = { 1.0, 0.55, 0.32, 0.2, 0.12, 0.07 };
		double phases[6] = {};

		for (int n = 0; n < total; ++n)
		{
			const double t = n / SAMPLE_RATE;
			const double vibrato = std::pow(2.0, (vibratoCents / 1200.0) * std::sin(2.0 * PI * vibratoHz * t));
			const double f = frequency * vibrato;
			const double envelope = std::exp(-t * 1.2);

			double sample = 0.0;
			for (int h = 0; h < 6; ++h)
			{
				phases[h] += 2.0 * PI * f * (h + 1) / SAMPLE_RATE;
				sample += amps[h] * std::exp(-t * 0.8 * h) * std::sin(phases[h]);
			}

			out[n] = (float)(sample * envelope) * level * 0.4f;
		}

		return out;
	}

	std::vector<float> Bend(double startFrequency, double cents, double seconds, float level)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		std::vector<float> out(total);

		const double amps[6] = { 1.0, 0.55, 0.32, 0.2, 0.12, 0.07 };
		double phases[6] = {};

		for (int n = 0; n < total; ++n)
		{
			const double t = n / SAMPLE_RATE;
			// Bend rises over the middle third, holds after.
			double progress = (t - seconds / 3.0) / (seconds / 3.0);
			if (progress < 0.0) progress = 0.0;
			if (progress > 1.0) progress = 1.0;
			const double f = startFrequency * std::pow(2.0, cents * progress / 1200.0);
			const double envelope = std::exp(-t * 0.9);

			double sample = 0.0;
			for (int h = 0; h < 6; ++h)
			{
				phases[h] += 2.0 * PI * f * (h + 1) / SAMPLE_RATE;
				sample += amps[h] * std::sin(phases[h]);
			}

			out[n] = (float)(sample * envelope) * level * 0.4f;
		}

		return out;
	}

	std::vector<float> DoubleStop(double f1, double f2, double seconds, float level)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		std::vector<float> out(total);

		double phases1[4] = {}, phases2[4] = {};
		const double amps[4] = { 1.0, 0.5, 0.28, 0.15 };

		for (int n = 0; n < total; ++n)
		{
			const double t = n / SAMPLE_RATE;
			const double envelope = std::exp(-t * 1.0);

			double sample = 0.0;
			for (int h = 0; h < 4; ++h)
			{
				phases1[h] += 2.0 * PI * f1 * (h + 1) / SAMPLE_RATE;
				phases2[h] += 2.0 * PI * f2 * (h + 1) / SAMPLE_RATE;
				sample += amps[h] * (std::sin(phases1[h]) + std::sin(phases2[h]));
			}

			out[n] = (float)(sample * envelope) * level * 0.25f;
		}

		return out;
	}

	std::vector<float> Staccato(double frequency, int repeats, double noteSeconds, double gapSeconds, float level)
	{
		std::vector<float> out;

		for (int r = 0; r < repeats; ++r)
		{
			const int noteSamples = (int)(noteSeconds * SAMPLE_RATE);
			double phases[5] = {};
			const double amps[5] = { 1.0, 0.5, 0.3, 0.18, 0.1 };

			for (int n = 0; n < noteSamples; ++n)
			{
				const double t = n / SAMPLE_RATE;
				// Fast attack, fast release - palm-mute-ish. The release ramp matters:
				// hard-truncating the envelope mid-decay puts a genuine click in the
				// input at every note end, which both sounds wrong and blinds the pop
				// diff to shifter artifacts near note boundaries.
				double envelope = std::exp(-t * 18.0);
				if (n < 48) envelope *= n / 48.0;
				const int remaining = noteSamples - n;
				if (remaining < 240) envelope *= remaining / 240.0;

				double sample = 0.0;
				for (int h = 0; h < 5; ++h)
				{
					phases[h] += 2.0 * PI * frequency * (h + 1) / SAMPLE_RATE;
					sample += amps[h] * std::sin(phases[h]);
				}

				out.push_back((float)(sample * envelope) * level * 0.4f);
			}

			const int gapSamples = (int)(gapSeconds * SAMPLE_RATE);
			for (int n = 0; n < gapSamples; ++n) out.push_back(0.0f);
		}

		return out;
	}

	std::vector<float> AddNoise(std::vector<float> signal, float noiseLevel)
	{
		std::normal_distribution<float> dist(0.0f, noiseLevel);
		for (auto& s : signal) s += dist(rng);
		return signal;
	}

	struct PopEvent
	{
		int sample;
		float magnitude;
		float localEnvelope;
	};

	// A pop is a first-difference outlier: |x'| far beyond the running envelope of |x'|.
	// The envelope adapts fast enough to accept attacks but a splice discontinuity is a
	// single-sample spike the envelope has not seen coming.
	std::vector<PopEvent> DetectPops(const std::vector<float>& signal, float ratioThreshold, float absoluteFloor)
	{
		std::vector<PopEvent> events;

		float envelope = 0.0f;
		int lastEvent = -100000;

		for (size_t n = 1; n < signal.size(); ++n)
		{
			const float diff = std::fabs(signal[n] - signal[n - 1]);

			if (envelope > 1e-6f && diff > envelope * ratioThreshold && diff > absoluteFloor && (int)n - lastEvent > 256)
			{
				events.push_back({ (int)n, diff, envelope });
				lastEvent = (int)n;
			}

			const float attack = diff > envelope ? 0.02f : 0.0005f;
			envelope += (diff - envelope) * attack;
		}

		return events;
	}

	double Goertzel(const std::vector<float>& signal, int start, int length, double frequency)
	{
		const double w = 2.0 * PI * frequency / SAMPLE_RATE;
		const double coefficient = 2.0 * std::cos(w);
		double s0 = 0.0, s1 = 0.0, s2 = 0.0;

		for (int n = 0; n < length; ++n)
		{
			const double windowed = signal[start + n] * (0.5 - 0.5 * std::cos(2.0 * PI * n / length));
			s0 = windowed + coefficient * s1 - s2;
			s2 = s1;
			s1 = s0;
		}

		return s1 * s1 + s2 * s2 - coefficient * s1 * s2;
	}

	// Output energy at expected shifted harmonics vs a dense comb of off-harmonic probe
	// bins, in dB. Higher is cleaner; the gap is artifact energy.
	double HarmonicPurityDb(const std::vector<float>& output, int start, int length, double expectedFrequency)
	{
		double harmonicPower = 0.0;
		for (int h = 1; h <= 10; ++h)
		{
			const double f = expectedFrequency * h;
			if (f > 20000.0) break;
			// Sum a few bins around each harmonic to absorb vibrato and window leakage.
			for (double offset = -1.5; offset <= 1.5; offset += 0.75)
			{
				harmonicPower += Goertzel(output, start, length, f + offset * SAMPLE_RATE / length);
			}
		}

		double offHarmonicPower = 1e-12;
		int probes = 0;
		for (double f = 60.0; f < 8000.0; f *= 1.06)
		{
			// Skip probes near expected harmonics.
			const double nearest = std::round(f / expectedFrequency) * expectedFrequency;
			if (std::fabs(f - nearest) < expectedFrequency * 0.12) continue;
			offHarmonicPower += Goertzel(output, start, length, f);
			++probes;
		}
		offHarmonicPower /= probes > 0 ? probes : 1;
		const double perHarmonic = harmonicPower / 50.0;

		return 10.0 * std::log10(perHarmonic / offHarmonicPower);
	}

	double DominantFrequency(const std::vector<float>& output, int start, int length, double searchLow, double searchHigh)
	{
		double bestPower = 0.0;
		double bestFrequency = 0.0;

		for (double f = searchLow; f <= searchHigh; f *= 1.001)
		{
			const double p = Goertzel(output, start, length, f);
			if (p > bestPower)
			{
				bestPower = p;
				bestFrequency = f;
			}
		}

		return bestFrequency;
	}

	// Minimal RIFF WAV support: reads 16-bit PCM and 32-bit float, first channel only;
	// writes 32-bit float mono. Enough to run a recorded DI take through the shifter
	// and listen to or measure the result.
	bool ReadWav(const char* path, std::vector<float>& samples, uint32_t& sampleRate)
	{
		FILE* file = std::fopen(path, "rb");
		if (!file) return false;

		char riff[4];
		uint32_t riffSize;
		char wave[4];
		if (std::fread(riff, 1, 4, file) != 4 || std::memcmp(riff, "RIFF", 4) != 0 ||
			std::fread(&riffSize, 4, 1, file) != 1 ||
			std::fread(wave, 1, 4, file) != 4 || std::memcmp(wave, "WAVE", 4) != 0)
		{
			std::fclose(file);
			return false;
		}

		uint16_t format = 0, channels = 0, bitsPerSample = 0;
		sampleRate = 0;
		bool haveFormat = false;

		for (;;)
		{
			char chunkId[4];
			uint32_t chunkSize;
			if (std::fread(chunkId, 1, 4, file) != 4 || std::fread(&chunkSize, 4, 1, file) != 1) break;

			if (std::memcmp(chunkId, "fmt ", 4) == 0)
			{
				uint8_t fmt[16] = {};
				const uint32_t toRead = chunkSize < 16 ? chunkSize : 16;
				if (std::fread(fmt, 1, toRead, file) != toRead) break;
				std::memcpy(&format, fmt + 0, 2);
				std::memcpy(&channels, fmt + 2, 2);
				std::memcpy(&sampleRate, fmt + 4, 4);
				std::memcpy(&bitsPerSample, fmt + 14, 2);
				if (chunkSize > toRead) std::fseek(file, chunkSize - toRead, SEEK_CUR);
				haveFormat = true;
			}
			else if (std::memcmp(chunkId, "data", 4) == 0 && haveFormat)
			{
				const bool isFloat32 = format == 3 && bitsPerSample == 32;
				const bool isPcm16 = format == 1 && bitsPerSample == 16;
				if ((!isFloat32 && !isPcm16) || channels == 0)
				{
					std::fclose(file);
					return false;
				}

				const uint32_t bytesPerSample = bitsPerSample / 8;
				const uint32_t frameBytes = bytesPerSample * channels;
				const uint32_t frames = chunkSize / frameBytes;
				samples.resize(frames);

				std::vector<uint8_t> frame(frameBytes);
				for (uint32_t n = 0; n < frames; ++n)
				{
					if (std::fread(frame.data(), 1, frameBytes, file) != frameBytes)
					{
						samples.resize(n);
						break;
					}
					if (isFloat32)
					{
						float value;
						std::memcpy(&value, frame.data(), 4);
						samples[n] = value;
					}
					else
					{
						int16_t value;
						std::memcpy(&value, frame.data(), 2);
						samples[n] = value / 32768.0f;
					}
				}

				std::fclose(file);
				return true;
			}
			else
			{
				std::fseek(file, chunkSize + (chunkSize & 1), SEEK_CUR);
			}
		}

		std::fclose(file);
		return false;
	}

	bool WriteWav(const char* path, const std::vector<float>& samples, uint32_t sampleRate)
	{
		FILE* file = std::fopen(path, "wb");
		if (!file) return false;

		const uint32_t dataSize = (uint32_t)samples.size() * 4;
		const uint32_t riffSize = 36 + dataSize;
		const uint16_t format = 3, channels = 1, bitsPerSample = 32;
		const uint32_t byteRate = sampleRate * 4;
		const uint16_t blockAlign = 4;

		std::fwrite("RIFF", 1, 4, file);
		std::fwrite(&riffSize, 4, 1, file);
		std::fwrite("WAVE", 1, 4, file);
		std::fwrite("fmt ", 1, 4, file);
		const uint32_t fmtSize = 16;
		std::fwrite(&fmtSize, 4, 1, file);
		std::fwrite(&format, 2, 1, file);
		std::fwrite(&channels, 2, 1, file);
		std::fwrite(&sampleRate, 4, 1, file);
		std::fwrite(&byteRate, 4, 1, file);
		std::fwrite(&blockAlign, 2, 1, file);
		std::fwrite(&bitsPerSample, 2, 1, file);
		std::fwrite("data", 1, 4, file);
		std::fwrite(&dataSize, 4, 1, file);
		std::fwrite(samples.data(), 4, (size_t)samples.size(), file);
		std::fclose(file);
		return true;
	}

	int RunWavMode(const char* inputPath, int semitones, const char* outputPath)
	{
		std::vector<float> input;
		uint32_t sampleRate = 0;

		if (!ReadWav(inputPath, input, sampleRate))
		{
			std::printf("Failed to read %s (need 16-bit PCM or 32-bit float WAV)\n", inputPath);
			return 1;
		}

		std::printf("Loaded %s: %zu samples at %uHz (%.2fs), shift %+d\n",
			inputPath, input.size(), sampleRate, input.size() / (double)sampleRate, semitones);
		if (sampleRate != 48000)
		{
			std::printf("Note: shifter constants are tuned for 48kHz; results at %uHz are indicative only.\n", sampleRate);
		}

		Audio::DelayLinePitchShifter shifter(semitones);
		Audio::CaptureFormat format;
		shifter.Prepare(format);

		std::vector<float> output = input;
		for (size_t offset = 0; offset < output.size(); offset += chunkFrames)
		{
			const uint32_t count = (uint32_t)std::min<size_t>((size_t)chunkFrames, output.size() - offset);
			shifter.Process(output.data() + offset, count);
		}

		const auto inputPops = DetectPops(input, 12.0f, 0.02f);
		const auto outputPops = DetectPops(output, 12.0f, 0.02f);

		std::printf("Pops in/out: %zu -> %zu\n", inputPops.size(), outputPops.size());

		int artifacts = 0;
		for (const auto& event : outputPops)
		{
			bool nearInputPop = false;
			for (const auto& inputEvent : inputPops)
			{
				if (std::abs(event.sample - inputEvent.sample) < 1200) { nearInputPop = true; break; }
			}
			if (!nearInputPop)
			{
				++artifacts;
				std::printf("    POP at %.3fs  magnitude %.4f  envelope %.5f  (x%.0f)\n",
					event.sample / (double)sampleRate, event.magnitude, event.localEnvelope,
					event.magnitude / event.localEnvelope);
			}
		}
		std::printf("Shifter artifact pops (not near an input transient): %d\n", artifacts);

		if (outputPath)
		{
			if (WriteWav(outputPath, output, sampleRate)) std::printf("Wrote shifted output to %s\n", outputPath);
			else std::printf("Failed to write %s\n", outputPath);
		}

		return 0;
	}

	void RunCase(const char* name, std::vector<float> input, int semitones, double inputFrequency)
	{
		Audio::DelayLinePitchShifter shifter(semitones);
		Audio::CaptureFormat format;
		shifter.Prepare(format);

		std::vector<float> output = input;

		// Process in ASIO-sized callbacks (chunkFrames, default 96).
		for (size_t offset = 0; offset < output.size(); offset += chunkFrames)
		{
			const uint32_t count = (uint32_t)std::min<size_t>((size_t)chunkFrames, output.size() - offset);
			shifter.Process(output.data() + offset, count);
		}

		const auto inputPops = DetectPops(input, 12.0f, 0.02f);
		const auto outputPops = DetectPops(output, 12.0f, 0.02f);

		const double ratio = std::pow(2.0, semitones / 12.0);
		const double expected = inputFrequency * ratio;

		// Analysis window: past the attack and the shifter's lock-in, in the sustain.
		const int start = (int)(0.5 * SAMPLE_RATE);
		const int length = 16384;

		double purity = -99.0;
		double dominant = 0.0;
		if ((int)output.size() > start + length && inputFrequency > 0.0)
		{
			purity = HarmonicPurityDb(output, start, length, expected);
			dominant = DominantFrequency(output, start, length, expected * 0.85, expected * 1.15);
		}

		std::printf("%-34s | shift %+3d | pops in/out: %2zu -> %2zu", name, semitones, inputPops.size(), outputPops.size());
		if (inputFrequency > 0.0)
		{
			std::printf(" | pitch %7.2fHz (expect %7.2f, %+.1f cents) | purity %+6.1f dB",
				dominant, expected, 1200.0 * std::log2(dominant / expected), purity);
		}
		std::printf("\n");

		for (const auto& event : outputPops)
		{
			bool nearInputPop = false;
			for (const auto& inputEvent : inputPops)
			{
				if (std::abs(event.sample - inputEvent.sample) < 1200) { nearInputPop = true; break; }
			}
			if (!nearInputPop)
			{
				std::printf("    POP at %.3fs  magnitude %.4f  envelope %.5f  (x%.0f)\n",
					event.sample / SAMPLE_RATE, event.magnitude, event.localEnvelope,
					event.magnitude / event.localEnvelope);
			}
		}
	}

	void Report(const char* name, const std::vector<float>& input, const std::vector<float>& output)
	{
		const auto inputPops = DetectPops(input, 12.0f, 0.02f);
		const auto outputPops = DetectPops(output, 12.0f, 0.02f);

		std::printf("%-40s | pops in/out: %2zu -> %2zu\n", name, inputPops.size(), outputPops.size());

		for (const auto& event : outputPops)
		{
			bool nearInputPop = false;
			for (const auto& inputEvent : inputPops)
			{
				if (std::abs(event.sample - inputEvent.sample) < 1200) { nearInputPop = true; break; }
			}
			if (!nearInputPop)
			{
				std::printf("    POP at %.3fs  magnitude %.4f  envelope %.5f  (x%.0f)\n",
					event.sample / SAMPLE_RATE, event.magnitude, event.localEnvelope,
					event.magnitude / event.localEnvelope);
			}
		}
	}

	void RunFixed(const char* name, std::vector<float> input, int semitones)
	{
		Audio::DelayLinePitchShifter shifter(semitones);
		Audio::CaptureFormat format;
		shifter.Prepare(format);

		std::vector<float> output = input;
		for (size_t offset = 0; offset < output.size(); offset += chunkFrames)
		{
			const uint32_t count = (uint32_t)std::min<size_t>((size_t)chunkFrames, output.size() - offset);
			shifter.Process(output.data() + offset, count);
		}

		Report(name, input, output);
	}

	void RunRetune(const char* name, std::vector<float> input, int firstSemitones, int secondSemitones, double switchSeconds)
	{
		Audio::DelayLinePitchShifter shifter(firstSemitones);
		Audio::CaptureFormat format;
		shifter.Prepare(format);

		std::vector<float> output = input;
		const size_t switchAt = (size_t)(switchSeconds * SAMPLE_RATE);
		bool switched = false;

		for (size_t offset = 0; offset < output.size(); offset += chunkFrames)
		{
			if (!switched && offset >= switchAt)
			{
				shifter.SetSemitones(secondSemitones);
				switched = true;
			}
			const uint32_t count = (uint32_t)std::min<size_t>((size_t)chunkFrames, output.size() - offset);
			shifter.Process(output.data() + offset, count);
		}

		Report(name, input, output);
	}
}

int main(int argc, char** argv)
{
	// WAV mode: record a dry DI take, run it through the shifter, count artifact
	// pops, and optionally write the shifted result to listen to without the game.
	//   harness.exe <input.wav> <semitones> [output.wav] [chunkFrames]
	//   harness.exe <input.wav>             defaults to -2 and writes shifted_<input>.wav
	// Synthetic battery, optionally at a specific callback size:
	//   harness.exe [chunkFrames]
	if (argc == 2 && EndsWithWavExtension(argv[1]))
	{
		std::string outputPath = DefaultShiftedPath(argv[1]);
		std::printf("Callback size: %d frames\n", chunkFrames);
		const int result = RunWavMode(argv[1], -2, outputPath.c_str());
		WaitIfLaunchedByExplorer();
		return result;
	}

	if (argc >= 3)
	{
		if (argc >= 5)
		{
			const int requested = std::atoi(argv[4]);
			if (requested >= 16 && requested <= 4096) chunkFrames = requested;
		}
		std::printf("Callback size: %d frames\n", chunkFrames);
		const int result = RunWavMode(argv[1], std::atoi(argv[2]), argc >= 4 ? argv[3] : nullptr);
		WaitIfLaunchedByExplorer();
		return result;
	}

	if (argc == 2)
	{
		const int requested = std::atoi(argv[1]);
		if (requested >= 16 && requested <= 4096) chunkFrames = requested;
	}
	std::printf("Callback size: %d frames\n\n", chunkFrames);

	std::printf("=== Held notes (harmonic sum, decay, 5Hz/8c vibrato), shift -2 ===\n");
	RunCase("held E2  82.4Hz", HeldNote(82.41, 3.0, 1.0f, 5.0, 8.0), -2, 82.41);
	RunCase("held A2 110.0Hz", HeldNote(110.0, 3.0, 1.0f, 5.0, 8.0), -2, 110.0);
	RunCase("held D3 146.8Hz", HeldNote(146.83, 3.0, 1.0f, 5.0, 8.0), -2, 146.83);
	RunCase("held G3 196.0Hz", HeldNote(196.0, 3.0, 1.0f, 5.0, 8.0), -2, 196.0);
	RunCase("held B3 246.9Hz", HeldNote(246.94, 3.0, 1.0f, 5.0, 8.0), -2, 246.94);
	RunCase("held e4 329.6Hz", HeldNote(329.63, 3.0, 1.0f, 5.0, 8.0), -2, 329.63);

	std::printf("\n=== Held notes, shift -12 ===\n");
	RunCase("held E2  82.4Hz", HeldNote(82.41, 3.0, 1.0f, 5.0, 8.0), -12, 82.41);
	RunCase("held e4 329.6Hz", HeldNote(329.63, 3.0, 1.0f, 5.0, 8.0), -12, 329.63);

	std::printf("\n=== Held notes, shift +2 ===\n");
	RunCase("held E2  82.4Hz", HeldNote(82.41, 3.0, 1.0f, 5.0, 8.0), +2, 82.41);
	RunCase("held e4 329.6Hz", HeldNote(329.63, 3.0, 1.0f, 5.0, 8.0), +2, 329.63);

	std::printf("\n=== Plucks (Karplus-Strong, real attack transient), shift -2 ===\n");
	RunCase("pluck E2", Pluck(82.41, 2.0, 0.8f), -2, 82.41);
	RunCase("pluck A2", Pluck(110.0, 2.0, 0.8f), -2, 110.0);
	RunCase("pluck e4", Pluck(329.63, 2.0, 0.8f), -2, 329.63);

	std::printf("\n=== Light playing with noise floor (-40dB signal, -66dB noise), shift -2 ===\n");
	RunCase("light e4 + noise", AddNoise(HeldNote(329.63, 3.0, 0.01f, 5.0, 8.0), 0.0005f), -2, 329.63);
	RunCase("light E2 + noise", AddNoise(HeldNote(82.41, 3.0, 0.01f, 5.0, 8.0), 0.0005f), -2, 82.41);

	std::printf("\n=== String changes: E2 pluck then e4 pluck back to back, shift -2 ===\n");
	{
		auto first = Pluck(82.41, 1.2, 0.8f);
		auto second = Pluck(329.63, 1.2, 0.8f);
		first.insert(first.end(), second.begin(), second.end());
		RunCase("E2 -> e4 sequence", first, -2, 0.0);
	}
	{
		auto first = Pluck(329.63, 1.2, 0.8f);
		auto second = Pluck(82.41, 1.2, 0.8f);
		first.insert(first.end(), second.begin(), second.end());
		RunCase("e4 -> E2 sequence", first, -2, 0.0);
	}

	std::printf("\n=== Silence and pure noise floor, shift -2 (expect zero pops) ===\n");
	RunCase("silence", std::vector<float>((size_t)(2.0 * SAMPLE_RATE), 0.0f), -2, 0.0);
	RunCase("noise floor -60dB", AddNoise(std::vector<float>((size_t)(2.0 * SAMPLE_RATE), 0.0f), 0.001f), -2, 0.0);

	std::printf("\n=== Bends (held note, pitch rises mid-note), shift -2 ===\n");
	RunFixed("bend A2 +200 cents over 1s", Bend(110.0, 200.0, 3.0, 1.0f), -2);
	RunFixed("bend G3 +100 cents over 1s", Bend(196.0, 100.0, 3.0, 1.0f), -2);
	RunFixed("bend e4 +200 cents over 1s", Bend(329.63, 200.0, 3.0, 1.0f), -2);

	std::printf("\n=== Double stops, shift -2 ===\n");
	RunFixed("fifth  A2+E3 (110+164.8)", DoubleStop(110.0, 164.81, 3.0, 1.0f), -2);
	RunFixed("fourth D3+G3 (146.8+196)", DoubleStop(146.83, 196.0, 3.0, 1.0f), -2);
	RunFixed("third  G3+B3 (196+246.9)", DoubleStop(196.0, 246.94, 3.0, 1.0f), -2);

	std::printf("\n=== Staccato palm-mute-ish repeats, shift -2 ===\n");
	RunFixed("staccato E2 x8, 120ms + 60ms gap", Staccato(82.41, 8, 0.12, 0.06, 0.9f), -2);
	RunFixed("staccato A2 x8, 120ms + 60ms gap", Staccato(110.0, 8, 0.12, 0.06, 0.9f), -2);

	std::printf("\n=== Retune mid-note (F9/F10 during sustain) ===\n");
	RunRetune("held A2, -2 -> -4 at 1.5s", Bend(110.0, 0.0, 3.0, 1.0f), -2, -4, 1.5);
	RunRetune("held A2, -2 -> 0 at 1.5s (disengage)", Bend(110.0, 0.0, 3.0, 1.0f), -2, 0, 1.5);
	RunRetune("held A2, 0 -> -2 at 1.5s (engage)", Bend(110.0, 0.0, 3.0, 1.0f), 0, -2, 1.5);
	RunRetune("held E2, -1 -> -12 at 1.5s (slam)", Bend(82.41, 0.0, 3.0, 1.0f), -1, -12, 1.5);

	// Keep the window open when launched by double-click rather than from a terminal.
	std::printf("\nDone.");
	WaitIfLaunchedByExplorer();

	return 0;
}
