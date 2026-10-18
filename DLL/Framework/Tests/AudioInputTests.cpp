#include "../AudioInput.hpp"
#include "../IMod.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
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

		bool Process(std::uint32_t input, float* const* channels, std::uint32_t, std::uint32_t, bool active) noexcept override {
			log.push_back(name + (active ? "+" : "-") + (input ? std::to_string(input) : ""));
			if (active) channels[0][0] += 1.0f;
			return active;
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

		bool Process(std::uint32_t input, float* const*, std::uint32_t, std::uint32_t, bool) noexcept override {
			if (input != blockedInput) return false;
			entered = true;
			while (!open) std::this_thread::yield();
			return false;
		}

		std::uint32_t GetLatencyFrames(std::uint32_t) const override { return 0; }

		std::uint32_t blockedInput;
		std::atomic<bool> entered{ false };
		std::atomic<bool> open{ false };
		std::atomic<int> prepareCount[Framework::kMaxAudioInputs]{};
	};

	// Passes audio through unchanged, reporting whatever latency it's told to. Can fail its prepare, poison its
	// output or take too long.
	class Tunable : public IInputProcessor {
	public:
		explicit Tunable(std::uint32_t latency) : latency(latency) {}

		void Prepare(std::uint32_t, const CaptureFormat&) override {
			if (failPrepare) throw std::runtime_error("no");
		}

		bool Process(std::uint32_t, float* const* channels, std::uint32_t, std::uint32_t frames, bool) noexcept override {
			++processCount;
			frameSizes.push_back(frames);
			if (writeNaN) channels[0][0] = std::numeric_limits<float>::quiet_NaN();
			const auto until = std::chrono::steady_clock::now() + spin;
			while (std::chrono::steady_clock::now() < until) {}
			return writeNaN;
		}

		std::uint32_t GetLatencyFrames(std::uint32_t) const override { return latency; }

		std::uint32_t latency;
		bool failPrepare = false;
		bool writeNaN = false;
		std::chrono::steady_clock::duration spin{};
		int processCount = 0;
		std::vector<std::uint32_t> frameSizes;
	};

	std::vector<float> Impulse(size_t frames) {
		std::vector<float> block(frames, 0.0f);
		block[0] = 1.0f;
		return block;
	}

	// Where the impulse ended up, or -1.
	int ImpulseAt(const std::vector<float>& block) {
		for (size_t i = 0; i < block.size(); ++i)
			if (block[i] == 1.0f) return static_cast<int>(i);
		return -1;
	}

	const CaptureFormat kFormat{ 48000, 1, 480 };

	// One mono block through the chain.
	bool Tap(AudioInputChain& chain, std::uint32_t input, float* samples, std::uint32_t frames) {
		float* channels[] = { samples };
		return chain.ProcessTap(input, channels, frames);
	}

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
		Tap(chain, 0, &sample, 1);
		Check("nothing runs before the tap prepares", log.empty() && !chain.TapLive());

		chain.PrepareTap(0, kFormat);
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
		Check("removed after: stays in, inactive even when reported active", log == std::vector<std::string>{ "b-" });
		Check("removed after: latency kept", chain.LatencyFrames(0) == 20);

		// A retried mod re-runs OnInitialize and adds the same processor again.
		chain.Add(&modB, b, 1);
		chain.PublishActive(AllActive);
		log.clear();
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
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
		Tap(chain, 0, &sample, 1);
		Tap(chain, 1, &sample, 1);
		Check("only the prepared input runs, told its index", log == std::vector<std::string>{ "a+1" });

		chain.PrepareTap(0, kFormat);
		chain.ReleaseTap(1);
		log.clear();
		Tap(chain, 0, &sample, 1);
		Tap(chain, 1, &sample, 1);
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
		Tap(chain, Framework::kMaxAudioInputs, &sample, 1);
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
		std::thread audio([&] { Tap(chain, 0, &sample, 1); });
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

	// MainThread's PublishActive never waits for an audio block, even while a re-prepare does.
	{
		AudioInputChain chain;
		Gate gate(0);
		chain.Add(&modA, gate, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, kFormat);

		float sample = 0;
		std::thread audio([&] { Tap(chain, 0, &sample, 1); });
		while (!gate.entered) std::this_thread::yield();

		std::thread prepare([&] { chain.PrepareTap(0, kFormat); });
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		std::atomic<bool> published{ false };
		std::thread mainThread([&] { chain.PublishActive(AllActive); published = true; });
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		Check("PublishActive returns while a prepare waits on the audio thread", published.load());

		gate.open = true;
		audio.join();
		prepare.join();
		mainThread.join();
	}

	// A stream's latency holds: a processor that fails or changes its latency on a re-prepare becomes a plain delay.
	{
		AudioInputChain chain;
		Tunable tunable(4);
		chain.Add(&modA, tunable, 0);
		chain.PublishActive(AllActive);
		const CaptureFormat format{ 48000, 1, 16 };
		Check("first prepare fixes the stream's latency", chain.PrepareTap(0, format) == 4);

		tunable.failPrepare = true;
		Check("a failed re-prepare keeps the latency", chain.PrepareTap(0, format) == 4 && chain.LatencyFrames(0) == 4);
		auto block = Impulse(16);
		Tap(chain, 0, block.data(), 16);
		Check("and a plain delay of that length stands in", ImpulseAt(block) == 4 && tunable.processCount == 0);

		tunable.failPrepare = false;
		tunable.latency = 8;
		Check("a changed latency on a re-prepare keeps the old one", chain.PrepareTap(0, format) == 4);
		block = Impulse(16);
		Tap(chain, 0, block.data(), 16);
		Check("and is bypassed the same way", ImpulseAt(block) == 4 && tunable.processCount == 0);

		chain.ReleaseTap(0);
		Check("a new stream fixes the new latency", chain.PrepareTap(0, format) == 8);
		block = Impulse(16);
		Tap(chain, 0, block.data(), 16);
		Check("and runs the processor again", ImpulseAt(block) == 0 && tunable.processCount == 1);
	}

	// NaN or infinity out of a processor: zeroed, the processor taken out as a delay, the fault reported once.
	{
		AudioInputChain chain;
		Tunable poison(2);
		chain.Add(&modA, poison, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, { 48000, 1, 8 });

		poison.writeNaN = true;
		auto block = Impulse(8);
		Tap(chain, 0, block.data(), 8);
		bool finite = true;
		for (float s : block) finite = finite && std::isfinite(s);
		Check("non-finite output never reaches the game", finite);

		auto faults = chain.TakeFaults();
		Check("reported once, with its owner and input", faults.size() == 1 && faults[0].owner == &modA && faults[0].input == 0
			&& chain.TakeFaults().empty());

		const int callsBefore = poison.processCount;
		block = Impulse(8);
		Tap(chain, 0, block.data(), 8);
		Check("then bypassed as a delay of its latency", poison.processCount == callsBefore && ImpulseAt(block) == 2);

		poison.writeNaN = false;
		chain.PrepareTap(0, { 48000, 1, 8 });
		block = Impulse(8);
		Tap(chain, 0, block.data(), 8);
		Check("the next prepare gives it another chance", poison.processCount == callsBefore + 1);
	}

	// A processor that keeps running over half the block's real time is taken out.
	{
		AudioInputChain chain;
		Tunable slow(0);
		slow.spin = std::chrono::milliseconds(2); // A 16-frame block at 48 kHz lasts a third of a millisecond.
		chain.Add(&modA, slow, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, { 48000, 1, 16 });

		float block[16]{};
		for (int i = 0; i < 5; ++i) Tap(chain, 0, block, 16);
		const auto faults = chain.TakeFaults();
		Check("three over-budget blocks in a row take it out", slow.processCount == 3 && faults.size() == 1
			&& faults[0].detail.find("overloaded") != std::string::npos);
	}

	// Blocks bigger than the prepared maximum reach processors in pieces.
	{
		AudioInputChain chain;
		Tunable sizes(0);
		chain.Add(&modA, sizes, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(0, { 48000, 1, 4 });

		float block[10]{};
		Tap(chain, 0, block, 10);
		Check("a 10-frame block with a 4-frame maximum arrives as 4, 4, 2",
			sizes.frameSizes == std::vector<std::uint32_t>{ 4, 4, 2 });
	}

	// The tap learns whether to write the block back.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log);
		chain.Add(&modA, a, 0);
		chain.PublishActive([](const IMod*) { return false; });
		chain.PrepareTap(0, kFormat);

		float sample = 0;
		Check("nothing changed: the tap keeps its original packet", !Tap(chain, 0, &sample, 1));
		chain.PublishActive(AllActive);
		Check("a processor that changed samples says so", Tap(chain, 0, &sample, 1));
	}

	// A format the chain can't run is refused.
	{
		AudioInputChain chain;
		Tunable any(3);
		chain.Add(&modA, any, 0);
		Check("no channels: refused, input stays off", chain.PrepareTap(0, { 48000, 0, 16 }) == 0 && !chain.TapLive(0));
		Check("too many channels: refused", chain.PrepareTap(0, { 48000, Framework::kMaxInputChannels + 1, 16 }) == 0);
	}

	if (g_failures) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All AudioInput tests passed\n";
	return 0;
}
