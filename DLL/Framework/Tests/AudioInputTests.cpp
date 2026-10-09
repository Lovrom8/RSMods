#include "../AudioInput.hpp"
#include "../IMod.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
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

		void Prepare(const CaptureFormat& format) override {
			if (failPrepare) throw std::runtime_error("no");
			prepared = format;
			++prepareCount;
		}

		void Process(float* samples, std::uint32_t, bool active) noexcept override {
			log.push_back(name + (active ? "+" : "-"));
			if (active) samples[0] += 1.0f;
		}

		std::uint32_t GetLatencyFrames() const override { return latency; }

		std::string name;
		std::vector<std::string>& log;
		std::uint32_t latency;
		bool failPrepare;
		CaptureFormat prepared{};
		int prepareCount = 0;
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
		chain.ProcessTap(&sample, 1);
		Check("nothing runs before the tap prepares", log.empty() && !chain.TapLive());

		chain.PrepareTap(kFormat);
		chain.ProcessTap(&sample, 1);
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

		Check("PrepareTap returns total latency", chain.PrepareTap(kFormat) == 544 && chain.LatencyFrames() == 544);

		chain.PublishActive([&](const IMod* m) { return m == &modB; });
		float sample = 0;
		chain.ProcessTap(&sample, 1);
		Check("inactive owner still runs, told it's inactive", log == std::vector<std::string>{ "a-", "b+" });
		Check("latency unchanged by deactivation", chain.LatencyFrames() == 544);
	}

	// The chain is fixed at the first PrepareTap.
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log), late("late", log);
		chain.Add(&modA, a, 0);
		chain.PrepareTap(kFormat);
		chain.Add(&modB, late, 0);
		chain.PublishActive(AllActive);

		float sample = 0;
		chain.ProcessTap(&sample, 1);
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
		Check("removed before the tap starts: dropped", chain.PrepareTap(kFormat) == 20);

		chain.RemoveMod(&modB);
		chain.PublishActive(AllActive);
		float sample = 0;
		chain.ProcessTap(&sample, 1);
		Check("removed after: stays in, inactive even when reported active", log == std::vector<std::string>{ "b-" });
		Check("removed after: latency kept", chain.LatencyFrames() == 20);

		// A retried mod re-runs OnInitialize and adds the same processor again.
		chain.Add(&modB, b, 1);
		chain.PublishActive(AllActive);
		log.clear();
		chain.ProcessTap(&sample, 1);
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
		chain.PrepareTap(kFormat);
		float sample = 0;
		chain.ProcessTap(&sample, 1);
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
		Check("failed prepare adds no latency", chain.PrepareTap(kFormat) == 7);
		float sample = 0;
		chain.ProcessTap(&sample, 1);
		Check("failed prepare is skipped", log == std::vector<std::string>{ "good+" });
	}

	// Release and re-prepare (driver reset).
	{
		AudioInputChain chain;
		std::vector<std::string> log;
		Recorder a("a", log);
		chain.Add(&modA, a, 0);
		chain.PublishActive(AllActive);
		chain.PrepareTap(kFormat);
		chain.ReleaseTap();
		float sample = 0;
		chain.ProcessTap(&sample, 1);
		Check("released tap processes nothing", log.empty() && !chain.TapLive());

		chain.PrepareTap({ 44100, 2, 256 });
		Check("re-prepare passes the new format", a.prepareCount == 2 && a.prepared.sampleRate == 44100 && chain.TapLive());
	}

	if (g_failures) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All AudioInput tests passed\n";
	return 0;
}
