#include "../AudioInput.hpp"
#include "../IMod.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using Framework::AudioInputChain;
using Framework::CaptureFormat;
using Framework::IInputProcessor;
using Framework::IMod;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		std::cout << "  FAIL  " << name << "\n";
	}

	class MockMod : public IMod {
		std::string id;
	public:
		explicit MockMod(std::string name) : id(std::move(name)) {}
		std::string_view Id() const override { return id; }
	};

	// Records each call into a shared log, so tests can see the chain's order.
	class Recorder : public IInputProcessor {
	public:
		Recorder(std::string name, std::vector<std::string>& log, std::uint32_t latency = 0, bool failPrepare = false)
			: name(std::move(name)), log(log), latency(latency), failPrepare(failPrepare) {}

		void Prepare(std::uint32_t input, const CaptureFormat& format) override {
			if (failPrepare) throw std::runtime_error("no");
			prepared = format;
			preparedInput = input;
			++prepareCount;
		}

		void Process(std::uint32_t input, float* samples, std::uint32_t, bool active) noexcept override {
			log.push_back(name + (active ? "+" : "-") + (input ? std::to_string(input) : ""));
			if (active) samples[0] += 1.0f;
		}

		std::uint32_t GetLatencyFrames(std::uint32_t input) const override {
			if (failLatency) throw std::runtime_error("no latency");
			return latency + input * extraPerInput;
		}

		std::string name;
		std::vector<std::string>& log;
		std::uint32_t latency;
		bool failPrepare;
		bool failLatency = false;
		std::uint32_t extraPerInput = 0;
		CaptureFormat prepared{};
		std::uint32_t preparedInput = 99;
		int prepareCount = 0;
	};

	// Holds Process on one input until opened, so tests can see what waits for an in-flight block.
	class Gate : public IInputProcessor {
	public:
		explicit Gate(std::uint32_t blockedInput) : blockedInput(blockedInput) {}

		void Prepare(std::uint32_t input, const CaptureFormat&) override { ++prepareCount[input]; }

		void Process(std::uint32_t input, float*, std::uint32_t, bool) noexcept override {
			if (input != blockedInput) return;
			entered = true;
			while (!open) std::this_thread::yield();
		}

		std::uint32_t GetLatencyFrames(std::uint32_t) const override { return 0; }

		std::uint32_t blockedInput;
		std::atomic<bool> entered{ false };
		std::atomic<bool> open{ false };
		std::atomic<int> prepareCount[Framework::kMaxAudioInputs]{};
	};

	const CaptureFormat kFormat{ 48000, 1, 480 };
	auto AllActive = [](const IMod*) { return true; };
}

int main() {
	std::cout << "AudioInput tests\n";

	MockMod modA("ModA");
	MockMod modB("ModB");
	MockMod modC("ModC");

	// Order: lower first, ties by owner Id, regardless of registration order.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder b("b", log), a("a", log), c("c", log);
		chain.Add(&modB, b, 10);
		chain.Add(&modA, a, 10);
		chain.Add(&modC, c, 5);
		chain.PublishActive(AllActive);

		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("nothing runs before the tap prepares", log.empty() && !chain.TapLive());

		chain.PrepareTap(0, kFormat);
		chain.ProcessTap(0, &sample, 1);
		Check("runs in order, ties by owner Id", log == std::vector<std::string>{ "c+", "a+", "b+" });
		Check("each processor edits the same buffer", sample == 3.0f);
		Check("prepare sees the tap's format", a.prepared.sampleRate == 48000 && a.prepared.maxFrames == 480);
	}

	// Latency is the sum, and stays fixed while owners come and go.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log, 480), b("b", log, 64);
		chain.Add(&modA, a, 0);
		chain.Add(&modB, b, 1);
		chain.PublishActive(AllActive);

		Check("PrepareTap returns total latency", chain.PrepareTap(0, kFormat) == 544 && chain.LatencyFrames(0) == 544);

		chain.PublishActive([&](const IMod* m) { return m == &modB; });
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("inactive owner still runs, told it's inactive", log == std::vector<std::string>{ "a-", "b+" });
		Check("latency unchanged by deactivation", chain.LatencyFrames(0) == 544);
	}

	// The chain is fixed at the first PrepareTap.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log), late("late", log);
		chain.Add(&modA, a, 0);
		chain.PrepareTap(0, kFormat);
		chain.Add(&modB, late, 0);
		chain.PublishActive(AllActive);

		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("a processor added after the tap started is ignored", log == std::vector<std::string>{ "a+" });
	}

	// Removal before and after the chain is fixed.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log, 10), b("b", log, 20);
		chain.Add(&modA, a, 0);
		chain.Add(&modB, b, 1);
		chain.RemoveMod(&modA);
		chain.PublishActive(AllActive);
		Check("removed before the tap starts: dropped", chain.PrepareTap(0, kFormat) == 20);

		chain.RemoveMod(&modB);
		chain.PublishActive(AllActive);
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("removed after: stays in, inactive even when reported active", log == std::vector<std::string>{ "b-" });
		Check("removed after: latency kept", chain.LatencyFrames(0) == 20);

		// A retried mod re-runs OnInitialize and adds the same processor again.
		chain.Add(&modB, b, 1);
		chain.PublishActive(AllActive);
		log.clear();
		chain.ProcessTap(0, &sample, 1);
		Check("re-adding the same processor after removal reactivates it", log == std::vector<std::string>{ "b+" });
	}

	// Re-adding before the tap starts doesn't duplicate.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log);
		chain.Add(&modA, a, 0);
		chain.Add(&modA, a, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, kFormat);
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("same processor added twice runs once", log.size() == 1);
	}

	// A processor that fails to prepare drops out; the rest run.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder bad("bad", log, 100, true), good("good", log, 7);
		chain.Add(&modA, bad, 0);
		chain.Add(&modB, good, 1);
		chain.PublishActive(AllActive);
		Check("failed prepare adds no latency", chain.PrepareTap(0, kFormat) == 7);
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("failed prepare is skipped", log == std::vector<std::string>{ "good+" });
	}

	// A processor whose latency throws after Prepare drops out too, so no delay runs uncounted.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder bad("bad", log, 100), good("good", log, 7);
		bad.failLatency = true;
		chain.Add(&modA, bad, 0);
		chain.Add(&modB, good, 1);
		chain.PublishActive(AllActive);
		Check("failed latency adds no latency", chain.PrepareTap(0, kFormat) == 7);
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("failed latency is skipped", log == std::vector<std::string>{ "good+" });
	}

	// Release and re-prepare (driver reset).
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log);
		chain.Add(&modA, a, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, kFormat);
		chain.ReleaseTap(0);
		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		Check("released tap processes nothing", log.empty() && !chain.TapLive());

		chain.PrepareTap(0, { 44100, 2, 256 });
		Check("re-prepare passes the new format", a.prepareCount == 2 && a.prepared.sampleRate == 44100 && chain.TapLive());
	}

	// Inputs are independent: own format, latency and live state.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log, 100);
		a.extraPerInput = 10;
		chain.Add(&modA, a, 0);
		chain.PublishActive(AllActive);

		Check("input 1 latency is its own", chain.PrepareTap(1, { 44100, 1, 256 }) == 110 && chain.LatencyFrames(1) == 110);
		Check("prepare is told the input", a.preparedInput == 1 && a.prepared.sampleRate == 44100);
		Check("preparing input 1 leaves input 0 down", chain.TapLive(1) && !chain.TapLive(0) && chain.TapLive());

		float sample = 0;
		chain.ProcessTap(0, &sample, 1);
		chain.ProcessTap(1, &sample, 1);
		Check("only the prepared input runs, told its index", log == std::vector<std::string>{ "a+1" });

		chain.PrepareTap(0, kFormat);
		chain.ReleaseTap(1);
		log.clear();
		chain.ProcessTap(0, &sample, 1);
		chain.ProcessTap(1, &sample, 1);
		Check("releasing input 1 leaves input 0 running", log == std::vector<std::string>{ "a+" } && chain.LatencyFrames(0) == 100);
	}

	// Indices past the last input are refused.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log, 5);
		chain.Add(&modA, a, 0);
		chain.PublishActive(AllActive);
		float sample = 0;
		Check("out of range prepare returns 0", chain.PrepareTap(Framework::kMaxAudioInputs, kFormat) == 0);
		chain.ProcessTap(Framework::kMaxAudioInputs, &sample, 1);
		Check("out of range does nothing", log.empty() && !chain.TapLive() && a.prepareCount == 0);
	}

	// Re-preparing an input waits for its in-flight block; another input's block doesn't hold it up.
	{
		AudioInputChain chain;
		Gate gate(0);
		chain.Add(&modA, gate, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, kFormat);
		chain.PrepareTap(1, kFormat);

		float sample = 0;
		std::thread audio([&] { chain.ProcessTap(0, &sample, 1); });
		while (!gate.entered) std::this_thread::yield();

		chain.PrepareTap(1, kFormat);
		Check("another input prepares while input 0 is mid-block", gate.prepareCount[1] == 2);

		std::atomic<bool> prepared{ false };
		std::thread main([&] { chain.PrepareTap(0, kFormat); prepared = true; });
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		Check("same input's prepare waits for the block", !prepared && gate.prepareCount[0] == 1);

		gate.open = true;
		audio.join();
		main.join();
		Check("then prepares", prepared && gate.prepareCount[0] == 2 && chain.TapLive(0));
	}

	if (g_failures) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All AudioInput tests passed\n";
	return 0;
}
