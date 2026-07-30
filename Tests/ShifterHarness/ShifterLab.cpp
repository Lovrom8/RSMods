// ShifterLab: a small Windows app for listening to DelayLinePitchShifter.
//
// Two modes:
//  - Test signals: pick a generated signal, play it dry or through the shifter,
//    to hear exactly what the harness measures.
//  - Live input: routes the default recording device (guitar interface) through
//    the shifter to the default output, recording the dry take to a WAV for use
//    with harness.exe. Uses waveIn/waveOut for simplicity, so monitoring latency
//    is tens of milliseconds; fine for artifact listening, not a playing tool.
//
// Build: see build.bat.

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <windows.h>
#include <mmsystem.h>

#include "DelayLinePitchShifter.hpp"

namespace
{
	constexpr double SAMPLE_RATE = 48000.0;
	constexpr double PI = 3.14159265358979323846;

	constexpr int ID_SIGNAL_LIST = 101;
	constexpr int ID_SEMITONES = 102;
	constexpr int ID_PLAY_DRY = 103;
	constexpr int ID_PLAY_SHIFTED = 104;
	constexpr int ID_STOP = 105;
	constexpr int ID_LIVE_TOGGLE = 106;
	constexpr int ID_STATUS = 107;
	constexpr int ID_BUFFER = 108;
	constexpr UINT WM_LIVE_STOPPED = WM_APP + 1;

	// Callback sizes offered for the offline "Play shifted" path. The shifter is a
	// per-sample loop with no per-callback state, so output is identical at every
	// size; the selector exists to demonstrate that ASIO buffer settings cannot be
	// the source of shifter artifacts.
	constexpr int BUFFER_SIZES[] = { 64, 96, 128, 256, 512 };

	std::mt19937 rng(1337);

	// ---- Signal generators (mirrors the console harness) ----

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

	std::vector<float> HeldNote(double frequency, double seconds, float level)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		std::vector<float> out(total);

		const double amps[6] = { 1.0, 0.55, 0.32, 0.2, 0.12, 0.07 };
		double phases[6] = {};

		for (int n = 0; n < total; ++n)
		{
			const double t = n / SAMPLE_RATE;
			const double vibrato = std::pow(2.0, (8.0 / 1200.0) * std::sin(2.0 * PI * 5.0 * t));
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
				double envelope = std::exp(-t * 18.0);
				if (n < 48) envelope *= n / 48.0;
				const int remaining = noteSamples - n;
				if (remaining < 240) envelope *= remaining / 240.0;	// release ramp: no click at note end

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

	std::vector<float> Bend(double startFrequency, double cents, double seconds, float level)
	{
		const int total = (int)(seconds * SAMPLE_RATE);
		std::vector<float> out(total);

		const double amps[6] = { 1.0, 0.55, 0.32, 0.2, 0.12, 0.07 };
		double phases[6] = {};

		for (int n = 0; n < total; ++n)
		{
			const double t = n / SAMPLE_RATE;
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

	struct TestSignal
	{
		const wchar_t* name;
		std::vector<float> (*generate)();
	};

	const TestSignal TEST_SIGNALS[] =
	{
		{ L"Held note E2 (82.4Hz, vibrato, decay)", [] { return HeldNote(82.41, 3.0, 1.0f); } },
		{ L"Held note A2 (110Hz)", [] { return HeldNote(110.0, 3.0, 1.0f); } },
		{ L"Held note G3 (196Hz)", [] { return HeldNote(196.0, 3.0, 1.0f); } },
		{ L"Held note e4 (329.6Hz)", [] { return HeldNote(329.63, 3.0, 1.0f); } },
		{ L"Pluck E2 (Karplus-Strong attack)", [] { return Pluck(82.41, 2.5, 0.8f); } },
		{ L"Pluck e4", [] { return Pluck(329.63, 2.5, 0.8f); } },
		{ L"Double stop: fifth A2+E3", [] { return DoubleStop(110.0, 164.81, 3.0, 1.0f); } },
		{ L"Double stop: fourth D3+G3", [] { return DoubleStop(146.83, 196.0, 3.0, 1.0f); } },
		{ L"Double stop: third G3+B3", [] { return DoubleStop(196.0, 246.94, 3.0, 1.0f); } },
		{ L"Staccato palm mutes E2 x8", [] { return Staccato(82.41, 8, 0.12, 0.06, 0.9f); } },
		{ L"Bend A2 +200 cents", [] { return Bend(110.0, 200.0, 3.0, 1.0f); } },
	};

	// ---- Playback of a rendered buffer (waveOut, one-shot) ----

	class Player
	{
	public:
		~Player()
		{
			Stop();
		}

		bool Play(const std::vector<float>& samples)
		{
			Stop();

			pcm.resize(samples.size());
			for (size_t n = 0; n < samples.size(); ++n)
			{
				float value = samples[n];
				if (value > 1.0f) value = 1.0f;
				if (value < -1.0f) value = -1.0f;
				pcm[n] = (int16_t)(value * 32000.0f);
			}

			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 1;
			format.nSamplesPerSec = 48000;
			format.wBitsPerSample = 16;
			format.nBlockAlign = 2;
			format.nAvgBytesPerSec = 48000 * 2;

			if (waveOutOpen(&device, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
			{
				return false;
			}

			std::memset(&header, 0, sizeof(header));
			header.lpData = (LPSTR)pcm.data();
			header.dwBufferLength = (DWORD)(pcm.size() * 2);

			waveOutPrepareHeader(device, &header, sizeof(header));
			waveOutWrite(device, &header, sizeof(header));
			return true;
		}

		void Stop()
		{
			if (!device) return;
			waveOutReset(device);
			waveOutUnprepareHeader(device, &header, sizeof(header));
			waveOutClose(device);
			device = nullptr;
		}

	private:
		HWAVEOUT device = nullptr;
		WAVEHDR header = {};
		std::vector<int16_t> pcm;
	};

	// ---- Live loopthrough: default input -> shifter -> default output ----
	//
	// CALLBACK_EVENT plus a worker thread that owns all audio pumping. The UI thread
	// only flags the worker and joins it before touching the devices; calling
	// waveInReset while a CALLBACK_FUNCTION callback is in flight is winmm's classic
	// deadlock, and this structure makes it impossible. The dry input is recorded
	// while running and written to a timestamped WAV on stop, ready to feed to
	// harness.exe as a regression take.

	class LiveLoop
	{
	public:
		bool Start(int semitones, std::wstring& error)
		{
			if (running.load()) return true;

			shifter = new Audio::DelayLinePitchShifter(semitones);
			Audio::CaptureFormat captureFormat;
			shifter->Prepare(captureFormat);

			capture.clear();
			capture.reserve(48000 * 60);

			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 1;
			format.nSamplesPerSec = 48000;
			format.wBitsPerSample = 16;
			format.nBlockAlign = 2;
			format.nAvgBytesPerSec = 48000 * 2;

			bufferEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!bufferEvent)
			{
				error = L"Could not create the audio event.";
				Cleanup();
				return false;
			}

			if (waveInOpen(&input, WAVE_MAPPER, &format, (DWORD_PTR)bufferEvent, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
			{
				error = L"Could not open the default recording device at 48kHz mono.";
				Cleanup();
				return false;
			}

			if (waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
			{
				error = L"Could not open the default playback device at 48kHz mono.";
				Cleanup();
				return false;
			}

			for (int i = 0; i < BUFFER_COUNT; ++i)
			{
				inputPcm[i].assign(FRAMES_PER_BUFFER, 0);
				std::memset(&inputHeaders[i], 0, sizeof(WAVEHDR));
				inputHeaders[i].lpData = (LPSTR)inputPcm[i].data();
				inputHeaders[i].dwBufferLength = FRAMES_PER_BUFFER * 2;
				waveInPrepareHeader(input, &inputHeaders[i], sizeof(WAVEHDR));
				waveInAddBuffer(input, &inputHeaders[i], sizeof(WAVEHDR));

				outputPcm[i].assign(FRAMES_PER_BUFFER, 0);
				std::memset(&outputHeaders[i], 0, sizeof(WAVEHDR));
				outputHeaders[i].lpData = (LPSTR)outputPcm[i].data();
				outputHeaders[i].dwBufferLength = FRAMES_PER_BUFFER * 2;
				waveOutPrepareHeader(output, &outputHeaders[i], sizeof(WAVEHDR));
				outputHeaders[i].dwFlags |= WHDR_DONE;
			}

			nextInput = 0;
			nextOutput = 0;
			running.store(true);

			worker = CreateThread(nullptr, 0, WorkerEntry, this, 0, nullptr);
			if (!worker)
			{
				running.store(false);
				error = L"Could not start the audio thread.";
				Cleanup();
				return false;
			}

			waveInStart(input);
			return true;
		}

		bool BeginStop(HWND window)
		{
			if (stopping.load()) return true;
			if (!running.load()) return false;

			notifyWindow = window;
			stopping.store(true);
			stopWorker = CreateThread(nullptr, 0, StopWorkerEntry, this, 0, nullptr);
			if (stopWorker) return true;

			stopping.store(false);
			notifyWindow = nullptr;
			return false;
		}

		void CompleteStop(std::wstring& savedPath, double& seconds)
		{
			if (stopWorker)
			{
				WaitForSingleObject(stopWorker, 0);
				CloseHandle(stopWorker);
				stopWorker = nullptr;
			}

			savedPath = stopSavedPath;
			seconds = stopCapturedSeconds;
			stopSavedPath.clear();
			stopCapturedSeconds = 0.0;
		}

		void SetSemitones(int semitones)
		{
			if (shifter && running.load()) shifter->SetSemitones(semitones);
		}

		bool IsRunning() const
		{
			return running.load();
		}

		bool IsStopping() const
		{
			return stopping.load();
		}

		double CapturedSeconds() const
		{
			return capture.size() / 48000.0;
		}

	private:
		static constexpr int FRAMES_PER_BUFFER = 480;	// 10ms at 48k
		static constexpr int BUFFER_COUNT = 4;
		static constexpr size_t CAPTURE_LIMIT = 48000 * 600;	// 10 minutes

		static DWORD WINAPI WorkerEntry(LPVOID parameter)
		{
			((LiveLoop*)parameter)->WorkerLoop();
			return 0;
		}

		static DWORD WINAPI StopWorkerEntry(LPVOID parameter)
		{
			((LiveLoop*)parameter)->StopWorkerLoop();
			return 0;
		}

		void WorkerLoop()
		{
			while (running.load())
			{
				WaitForSingleObject(bufferEvent, 100);

				while (running.load() && (inputHeaders[nextInput].dwFlags & WHDR_DONE))
				{
					WAVEHDR& header = inputHeaders[nextInput];
					ProcessBuffer(header);
					header.dwFlags &= ~WHDR_DONE;
					waveInAddBuffer(input, &header, sizeof(WAVEHDR));
					nextInput = (nextInput + 1) % BUFFER_COUNT;
				}
			}
		}

		void StopWorkerLoop()
		{
			// All potentially slow stop work happens here so the window thread remains
			// responsive while the driver releases buffers and the capture is written.
			running.store(false);
			SetEvent(bufferEvent);

			if (worker)
			{
				WaitForSingleObject(worker, INFINITE);
				CloseHandle(worker);
				worker = nullptr;
			}

			waveInStop(input);
			waveInReset(input);
			waveOutReset(output);

			for (int i = 0; i < BUFFER_COUNT; ++i)
			{
				waveInUnprepareHeader(input, &inputHeaders[i], sizeof(WAVEHDR));
				waveOutUnprepareHeader(output, &outputHeaders[i], sizeof(WAVEHDR));
			}

			stopCapturedSeconds = capture.size() / 48000.0;
			Cleanup();
			stopSavedPath = WriteCapture();
			stopping.store(false);

			if (notifyWindow) PostMessageW(notifyWindow, WM_LIVE_STOPPED, 0, 0);
		}

		void ProcessBuffer(const WAVEHDR& header)
		{
			const int frames = header.dwBytesRecorded / 2;
			const int16_t* in = (const int16_t*)header.lpData;
			const int count = frames < FRAMES_PER_BUFFER ? frames : FRAMES_PER_BUFFER;

			if (capture.size() < CAPTURE_LIMIT)
			{
				capture.insert(capture.end(), in, in + count);
			}

			float work[FRAMES_PER_BUFFER];
			for (int n = 0; n < count; ++n) work[n] = in[n] / 32768.0f;

			shifter->Process(work, (uint32_t)count);

			WAVEHDR& out = outputHeaders[nextOutput];
			if (out.dwFlags & WHDR_DONE)
			{
				int16_t* pcm = (int16_t*)out.lpData;
				for (int n = 0; n < count; ++n)
				{
					float value = work[n];
					if (value > 1.0f) value = 1.0f;
					if (value < -1.0f) value = -1.0f;
					pcm[n] = (int16_t)(value * 32000.0f);
				}
				out.dwBufferLength = count * 2;
				out.dwFlags &= ~WHDR_DONE;
				waveOutWrite(output, &out, sizeof(WAVEHDR));
				nextOutput = (nextOutput + 1) % BUFFER_COUNT;
			}
		}

		std::wstring WriteCapture()
		{
			if (capture.size() < 4800) return L"";	// under 100ms: nothing worth keeping

			SYSTEMTIME now;
			GetLocalTime(&now);
			wchar_t name[64];
			swprintf(name, 64, L"live_input_%02d%02d%02d.wav", now.wHour, now.wMinute, now.wSecond);

			FILE* file = _wfopen(name, L"wb");
			if (!file) return L"";

			const uint32_t dataSize = (uint32_t)capture.size() * 2;
			const uint32_t riffSize = 36 + dataSize;
			const uint16_t pcmFormat = 1, channels = 1, bits = 16;
			const uint32_t sampleRate = 48000, byteRate = 48000 * 2, fmtSize = 16;
			const uint16_t blockAlign = 2;

			std::fwrite("RIFF", 1, 4, file);
			std::fwrite(&riffSize, 4, 1, file);
			std::fwrite("WAVE", 1, 4, file);
			std::fwrite("fmt ", 1, 4, file);
			std::fwrite(&fmtSize, 4, 1, file);
			std::fwrite(&pcmFormat, 2, 1, file);
			std::fwrite(&channels, 2, 1, file);
			std::fwrite(&sampleRate, 4, 1, file);
			std::fwrite(&byteRate, 4, 1, file);
			std::fwrite(&blockAlign, 2, 1, file);
			std::fwrite(&bits, 2, 1, file);
			std::fwrite("data", 1, 4, file);
			std::fwrite(&dataSize, 4, 1, file);
			std::fwrite(capture.data(), 2, capture.size(), file);
			std::fclose(file);

			return name;
		}

		void Cleanup()
		{
			if (input) { waveInClose(input); input = nullptr; }
			if (output) { waveOutClose(output); output = nullptr; }
			if (bufferEvent) { CloseHandle(bufferEvent); bufferEvent = nullptr; }
			delete shifter;
			shifter = nullptr;
		}

		HWAVEIN input = nullptr;
		HWAVEOUT output = nullptr;
		HANDLE bufferEvent = nullptr;
		HANDLE worker = nullptr;
		HANDLE stopWorker = nullptr;
		HWND notifyWindow = nullptr;
		WAVEHDR inputHeaders[BUFFER_COUNT] = {};
		WAVEHDR outputHeaders[BUFFER_COUNT] = {};
		std::vector<int16_t> inputPcm[BUFFER_COUNT];
		std::vector<int16_t> outputPcm[BUFFER_COUNT];
		std::vector<int16_t> capture;
		std::wstring stopSavedPath;
		double stopCapturedSeconds = 0.0;
		int nextInput = 0;
		int nextOutput = 0;
		std::atomic_bool running = false;
		std::atomic_bool stopping = false;
		Audio::DelayLinePitchShifter* shifter = nullptr;
	};

	// ---- Application state ----

	HWND signalList = nullptr;
	HWND semitoneCombo = nullptr;
	HWND bufferCombo = nullptr;
	HWND liveButton = nullptr;
	HWND statusLabel = nullptr;

	Player player;
	LiveLoop live;
	bool closeAfterLiveStop = false;

	int SelectedSemitones()
	{
		const int index = (int)SendMessageW(semitoneCombo, CB_GETCURSEL, 0, 0);
		return index - 12;	// items run -12 .. +12
	}

	int SelectedBufferFrames()
	{
		const int index = (int)SendMessageW(bufferCombo, CB_GETCURSEL, 0, 0);
		if (index < 0 || index >= (int)(sizeof(BUFFER_SIZES) / sizeof(BUFFER_SIZES[0]))) return 256;
		return BUFFER_SIZES[index];
	}

	void SetStatus(const wchar_t* text)
	{
		SetWindowTextW(statusLabel, text);
	}

	void PlaySelected(bool shifted)
	{
		const int index = (int)SendMessageW(signalList, LB_GETCURSEL, 0, 0);
		if (index < 0 || index >= (int)(sizeof(TEST_SIGNALS) / sizeof(TEST_SIGNALS[0])))
		{
			SetStatus(L"Pick a test signal first.");
			return;
		}

		std::vector<float> samples = TEST_SIGNALS[index].generate();

		if (shifted)
		{
			const int semitones = SelectedSemitones();
			const size_t chunk = (size_t)SelectedBufferFrames();
			Audio::DelayLinePitchShifter shifter(semitones);
			Audio::CaptureFormat format;
			shifter.Prepare(format);

			// Same chunking the harness uses, at the selected ASIO-style buffer size.
			for (size_t offset = 0; offset < samples.size(); offset += chunk)
			{
				const uint32_t count = (uint32_t)(samples.size() - offset < chunk ? samples.size() - offset : chunk);
				shifter.Process(samples.data() + offset, count);
			}
		}

		if (player.Play(samples))
		{
			wchar_t text[256];
			wsprintfW(text, L"Playing %s (%s)", TEST_SIGNALS[index].name, shifted ? L"shifted" : L"dry");
			SetStatus(text);
		}
		else
		{
			SetStatus(L"Could not open the playback device.");
		}
	}

	void ToggleLive(HWND window)
	{
		if (live.IsRunning() || live.IsStopping())
		{
			if (live.IsStopping()) return;
			if (live.BeginStop(window))
			{
				SetWindowTextW(liveButton, L"Stopping...");
				EnableWindow(liveButton, FALSE);
				SetStatus(L"Stopping live input and saving the dry WAV...");
			}
			else
			{
				SetStatus(L"Could not start the live input stop worker.");
			}
			return;
		}

		player.Stop();

		std::wstring error;
		if (live.Start(SelectedSemitones(), error))
		{
			SetWindowTextW(liveButton, L"Stop live input");
			SetStatus(L"Live: default input through the shifter, recording the dry take. Latency is tens of ms; for artifact listening, not playing feel.");
		}
		else
		{
			MessageBoxW(window, error.c_str(), L"ShifterLab", MB_ICONWARNING);
			SetStatus(L"Live input failed to start.");
		}
	}

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_LIVE_STOPPED:
		{
			std::wstring saved;
			double seconds = 0.0;
			live.CompleteStop(saved, seconds);

			if (liveButton)
			{
				SetWindowTextW(liveButton, L"Start live input");
				EnableWindow(liveButton, TRUE);
			}

			if (!saved.empty())
			{
				wchar_t text[256];
				swprintf(text, 256, L"Stopped. Saved dry input: %s (%.1fs). Drag it onto harness.exe or run harness.exe %s", saved.c_str(), seconds, saved.c_str());
				SetStatus(text);
			}
			else
			{
				SetStatus(L"Live input stopped. Capture too short to save.");
			}

			if (closeAfterLiveStop)
			{
				closeAfterLiveStop = false;
				player.Stop();
				DestroyWindow(window);
			}
			return 0;
		}

		case WM_CREATE:
		{
			CreateWindowW(L"STATIC", L"Shift (semitones):", WS_CHILD | WS_VISIBLE,
				16, 16, 120, 20, window, nullptr, nullptr, nullptr);

			semitoneCombo = CreateWindowW(L"COMBOBOX", nullptr,
				WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
				140, 12, 80, 300, window, (HMENU)(INT_PTR)ID_SEMITONES, nullptr, nullptr);

			for (int s = -12; s <= 12; ++s)
			{
				wchar_t item[8];
				swprintf(item, 8, L"%+d", s);	// wsprintfW lacks the '+' flag
				SendMessageW(semitoneCombo, CB_ADDSTRING, 0, (LPARAM)item);
			}
			SendMessageW(semitoneCombo, CB_SETCURSEL, 12 - 2, 0);	// default -2

			CreateWindowW(L"STATIC", L"Buffer:", WS_CHILD | WS_VISIBLE,
				235, 16, 50, 20, window, nullptr, nullptr, nullptr);
			bufferCombo = CreateWindowW(L"COMBOBOX", nullptr,
				WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
				288, 12, 70, 200, window, (HMENU)(INT_PTR)ID_BUFFER, nullptr, nullptr);
			for (const int size : BUFFER_SIZES)
			{
				wchar_t item[8];
				wsprintfW(item, L"%d", size);
				SendMessageW(bufferCombo, CB_ADDSTRING, 0, (LPARAM)item);
			}
			SendMessageW(bufferCombo, CB_SETCURSEL, 2, 0);	// default 256

			CreateWindowW(L"STATIC", L"Test signals:", WS_CHILD | WS_VISIBLE,
				16, 48, 200, 20, window, nullptr, nullptr, nullptr);

			signalList = CreateWindowW(L"LISTBOX", nullptr,
				WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
				16, 70, 340, 210, window, (HMENU)(INT_PTR)ID_SIGNAL_LIST, nullptr, nullptr);

			for (const auto& signal : TEST_SIGNALS)
			{
				SendMessageW(signalList, LB_ADDSTRING, 0, (LPARAM)signal.name);
			}
			SendMessageW(signalList, LB_SETCURSEL, 0, 0);

			CreateWindowW(L"BUTTON", L"Play dry", WS_CHILD | WS_VISIBLE,
				16, 290, 100, 28, window, (HMENU)(INT_PTR)ID_PLAY_DRY, nullptr, nullptr);
			CreateWindowW(L"BUTTON", L"Play shifted", WS_CHILD | WS_VISIBLE,
				124, 290, 100, 28, window, (HMENU)(INT_PTR)ID_PLAY_SHIFTED, nullptr, nullptr);
			CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE,
				232, 290, 60, 28, window, (HMENU)(INT_PTR)ID_STOP, nullptr, nullptr);

			liveButton = CreateWindowW(L"BUTTON", L"Start live input", WS_CHILD | WS_VISIBLE,
				16, 330, 140, 28, window, (HMENU)(INT_PTR)ID_LIVE_TOGGLE, nullptr, nullptr);

			statusLabel = CreateWindowW(L"STATIC", L"Ready.", WS_CHILD | WS_VISIBLE,
				16, 370, 340, 40, window, (HMENU)(INT_PTR)ID_STATUS, nullptr, nullptr);

			return 0;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case ID_PLAY_DRY:
				PlaySelected(false);
				return 0;
			case ID_PLAY_SHIFTED:
				PlaySelected(true);
				return 0;
			case ID_STOP:
				player.Stop();
				SetStatus(L"Stopped.");
				return 0;
			case ID_LIVE_TOGGLE:
				ToggleLive(window);
				return 0;
			case ID_SEMITONES:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					live.SetSemitones(SelectedSemitones());
				}
				return 0;
			}
			break;

		case WM_CLOSE:
			if (live.IsRunning() || live.IsStopping())
			{
				closeAfterLiveStop = true;
				if (!live.IsStopping())
				{
					if (live.BeginStop(window))
					{
						if (liveButton)
						{
							SetWindowTextW(liveButton, L"Stopping...");
							EnableWindow(liveButton, FALSE);
						}
						SetStatus(L"Stopping live input and saving the dry WAV...");
					}
					else
					{
						SetStatus(L"Could not start the live input stop worker.");
					}
				}
				return 0;
			}
			player.Stop();
			DestroyWindow(window);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(window, message, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"ShifterLabWindow";
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClassW(&windowClass);

	HWND window = CreateWindowW(L"ShifterLabWindow", L"ShifterLab - Drop Pedal Shifter Test",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 390, 460,
		nullptr, nullptr, instance, nullptr);

	ShowWindow(window, showCommand);

	MSG message;
	while (GetMessageW(&message, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	return 0;
}
