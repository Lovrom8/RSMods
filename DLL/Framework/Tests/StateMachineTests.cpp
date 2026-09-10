#include <iomanip>

#include "../Framework.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using Framework::ModRegistry;
using Framework::GamePhase;

namespace {
	std::vector<std::string> g_events;
	std::mutex g_eventsMutex;

	void RecordEvent(std::string event) {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		g_events.push_back(std::move(event));
	}

	void ClearEvents() {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		g_events.clear();
	}

	int IndexOf(const std::string& e) {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		for (int i = 0; i < (int)g_events.size(); ++i) if (g_events[i] == e) return i;
		return -1;
	}
	bool Has(const std::string& e) { return IndexOf(e) >= 0; }
	bool EventsEmpty() {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		return g_events.empty();
	}

	int g_failures = 0;

	void Expect(bool cond, const std::string& name) {
		std::cout << (cond ? "  PASS  " : "  FAIL  ") << name << "\n";
		if (!cond) ++g_failures;
	}

	void ExpectSeq(const std::vector<std::string>& want, const std::string& name) {
		std::vector<std::string> got;
		{
			std::lock_guard<std::mutex> lock(g_eventsMutex);
			got = g_events;
		}
		const bool ok = got == want;
		Expect(ok, name);
		if (!ok) {
			auto join = [](const std::vector<std::string>& v) {
				std::string s; for (auto& x : v) { if (!s.empty()) s += ", "; s += x; } return s;
			};
			std::cout << "        got:  [" << join(got) << "]\n";
			std::cout << "        want: [" << join(want) << "]\n";
		}
	}

}

namespace {
	class TestMod : public Framework::IMod {
	public:
		std::string id;
		std::string tag;
		bool enabled = true;
		int prio = 0;
		std::vector<std::string> claimStrings;
		std::string throwOn;
		bool recordDestruction = false;
		std::string commandSetting;
		Framework::Availability availability = Framework::Availability::Active;
		int commandCalls = 0;
		bool lastCommandControl = false;

		explicit TestMod(std::string i) : id(std::move(i)), tag(id) {}
		~TestMod() override {
			if (recordDestruction)
				RecordEvent(tag + ":destroy");
		}

		std::string_view Id() const override { return id; }
		bool IsEnabled(const Framework::ModContext&) const override { return enabled; }
		int Priority() const override { return prio; }

		std::vector<std::string_view> ClaimsExclusive() const override {
			std::vector<std::string_view> v; for (auto& s : claimStrings) v.push_back(s); return v;
		}

		Framework::SettingDefs declaredSettings;
		Framework::SettingDefs Settings() const override { return declaredSettings; }

		void OnInitialize(Framework::ModContext& c) override {
			Rec("OnInitialize");
			if (!commandSetting.empty()) {
				c.Commands().BindSetting(commandSetting, Framework::KeyEdge::Up, availability,
					[this](Framework::ModContext&, const Framework::KeyEvent& event) {
						++commandCalls;
						lastCommandControl = event.control;
					});
			}
		}
		void OnShutdown(Framework::ModContext&) override { Rec("OnShutdown"); }
		void OnSettingsChanged(Framework::ModContext&) override { Rec("OnSettingsChanged"); }
		void OnEnabled(Framework::ModContext&) override { Rec("OnEnabled"); }
		void OnDisabled(Framework::ModContext&) override { Rec("OnDisabled"); }
		void OnTick(Framework::ModContext&) override { Rec("OnTick"); }
		void OnMenuTick(Framework::ModContext&) override { Rec("OnMenuTick"); }
		void OnSongEnter(Framework::ModContext&) override { Rec("OnSongEnter"); }
		void OnSongTick(Framework::ModContext&) override { Rec("OnSongTick"); }
		void OnSongExit(Framework::ModContext&) override { Rec("OnSongExit"); }

	private:
		void Rec(const char* hook) {
			RecordEvent(tag + ":" + hook);
			if (throwOn == hook) throw std::runtime_error("injected failure in " + std::string(hook));
		}
	};

	TestMod* Add(ModRegistry& reg, const std::string& id, bool enabled = true, int prio = 0) {
		auto m = std::make_unique<TestMod>(id);
		m->enabled = enabled;
		m->prio = prio;
		TestMod* raw = m.get();
		reg.Register(std::move(m));
		return raw;
	}
}

static void Test_InitializeFaultIsIsolated() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "A"); a->throwOn = "OnInitialize";
	Add(reg, "B");
	reg.DispatchInitialize();
	Expect(Has("A:OnInitialize"), "A initialization attempted");
	Expect(Has("B:OnInitialize"), "B unaffected by A's initialization fault");
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(!Has("A:OnEnabled") && Has("B:OnEnabled"), "initialization fault prevents only A from activating");
	reg.Shutdown();
}

static void Test_ActivateInMenu() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "M:OnEnabled", "M:OnTick", "M:OnMenuTick" }, "activate in menu: no song edges");
	reg.Shutdown();
}

static void Test_EnableMidSong() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "S");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" }, "enable mid-song fires OnSongEnter");
	reg.Shutdown();
}

static void Test_DisableMidSong() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnSongExit", "S:OnDisabled" }, "disable mid-song: OnSongExit before OnDisabled");
	reg.Shutdown();
}

static void Test_ReenableAfterDisable() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	s->enabled = false;
	reg.Tick(GamePhase::Menu);
	ClearEvents();
	s->enabled = true;
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" },
		"disabled mod reactivates through the shared inactive state");
	reg.Shutdown();
}

static void Test_DisabledSameTickAsSongExit() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Menu);
	Expect(Has("S:OnSongExit") && Has("S:OnDisabled"), "OnSongExit not missed when disabled on song-exit tick");
	Expect(IndexOf("S:OnSongExit") < IndexOf("S:OnDisabled"), "OnSongExit ordered before OnDisabled");
	reg.Shutdown();
}

static void Test_ConflictSuppressionOrder() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "A", true, 1);
	TestMod* b = Add(reg, "B", false, 2);
	a->claimStrings = { "R" };
	b->claimStrings = { "R" };
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	Expect(Has("A:OnEnabled") && !Has("B:OnEnabled"), "A active alone");
	ClearEvents();
	b->enabled = true;
	reg.Tick(GamePhase::Menu);
	Expect(Has("A:OnDisabled"), "loser A gets OnDisabled on suppression");
	Expect(Has("B:OnEnabled"), "winner B activates");
	Expect(IndexOf("A:OnDisabled") < IndexOf("B:OnEnabled"), "deactivation precedes activation (resource handoff)");
	reg.Shutdown();
}

static void Test_OnEnabledThrowsFaults() {
	ClearEvents();
	ModRegistry reg;
	TestMod* f = Add(reg, "F"); f->throwOn = "OnEnabled";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "F:OnEnabled" }, "OnEnabled throw: no tick hooks run");
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(EventsEmpty(), "faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_OnSongEnterThrowsShortCircuits() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S"); s->throwOn = "OnSongEnter";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnDisabled" },
		"OnSongEnter throw faults immediately and skips tick hooks");
	reg.Shutdown();
}

static void Test_TickFailureFaultsImmediately() {
	ClearEvents();
	ModRegistry reg;
	TestMod* t = Add(reg, "T"); t->throwOn = "OnTick";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "T:OnEnabled", "T:OnTick", "T:OnDisabled" }, "tick exception faults and tears down immediately");
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(EventsEmpty(), "tick-faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_SettingsAppliedThenNotifiedOnTick() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.EnqueueSettingsUpdate([] { RecordEvent("SETTINGS:apply-1"); });
	reg.EnqueueSettingsUpdate([] { RecordEvent("SETTINGS:apply-2"); });
	Expect(!Has("SETTINGS:apply-1") && !Has("SETTINGS:apply-2"),
		"settings not applied on the message thread (before tick)");
	reg.Tick(GamePhase::Menu);
	Expect(Has("SETTINGS:apply-1") && Has("SETTINGS:apply-2") && Has("M:OnSettingsChanged"),
		"settings batch applied and mods notified on tick");
	Expect(IndexOf("SETTINGS:apply-1") < IndexOf("SETTINGS:apply-2"),
		"settings closures preserve FIFO order within a batch");
	Expect(IndexOf("SETTINGS:apply-2") < IndexOf("M:OnSettingsChanged"),
		"complete settings batch precedes notification");
	Expect(IndexOf("M:OnSettingsChanged") < IndexOf("M:OnEnabled"), "notify precedes activation resolve");
	reg.Shutdown();
}

static Framework::KeyEvent TestKeyEvent(unsigned int key, bool control = false) {
	Framework::KeyEvent event;
	event.virtualKey = key;
	event.edge = Framework::KeyEdge::Up;
	event.control = control;
	return event;
}

static void Test_KeyAvailabilityTracksRegistryLifecycle() {
	Framework::Commands().SetKeyResolver([](std::string_view setting) {
		return setting == "TestEffectiveCommand" ? 80u : setting == "TestInitializedCommand" ? 81u : 0u;
	});

	ModRegistry reg;
	TestMod* effective = Add(reg, "CommandEffective", false);
	effective->commandSetting = "TestEffectiveCommand";
	TestMod* initialized = Add(reg, "CommandInitialized", false);
	initialized->commandSetting = "TestInitializedCommand";
	initialized->availability = Framework::Availability::Initialized;
	reg.DispatchInitialize();

	Framework::Inbox().PostKeyEvent(TestKeyEvent(80, true));
	Framework::Inbox().PostKeyEvent(TestKeyEvent(81, true));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 0 && initialized->commandCalls == 1 && initialized->lastCommandControl,
		"registry exposes initialized binding but gates inactive active-only binding");

	effective->enabled = true;
	reg.Tick(GamePhase::Menu);
	Framework::Inbox().PostKeyEvent(TestKeyEvent(80, true));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 1 && effective->lastCommandControl,
		"registry activation enables command with captured modifier state");

	effective->enabled = false;
	reg.Tick(GamePhase::Menu);
	Framework::Inbox().PostKeyEvent(TestKeyEvent(80));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 1, "registry deactivation disables effective command before delivery");
	reg.Shutdown();
}

static void Test_ConflictSuppressionGatesEffectiveCommand() {
	Framework::Commands().SetKeyResolver([](std::string_view setting) {
		return setting == "SuppressedCommand" ? 82u : 0u;
	});

	ModRegistry reg;
	TestMod* suppressed = Add(reg, "Suppressed", true, 0);
	suppressed->claimStrings = { "R" };
	suppressed->commandSetting = "SuppressedCommand";
	TestMod* winner = Add(reg, "Winner", true, 10);
	winner->claimStrings = { "R" };
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);

	Framework::Inbox().PostKeyEvent(TestKeyEvent(82));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(suppressed->commandCalls == 0,
		"conflict-suppressed mod cannot bypass exclusive resource through effective command");
	reg.Shutdown();
}

static void Test_DuplicateIdRejected() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "Dup"); a->tag = "d1";
	auto dup = std::make_unique<TestMod>("Dup"); dup->tag = "d2";
	reg.Register(std::move(dup));
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	Expect(Has("d1:OnEnabled"), "first Dup registration active");
	Expect(!Has("d2:OnInitialize") && !Has("d2:OnEnabled"), "duplicate Id rejected (never runs)");
	reg.Shutdown();
}

static void Test_ShutdownRevertsAndDestroysInOrder() {
	ClearEvents();
	ModRegistry reg;
	TestMod* mod = Add(reg, "S");
	mod->recordDestruction = true;
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song); // active in a song, so inSong is set
	Expect(Has("S:OnEnabled") && Has("S:OnSongEnter"), "mod is active in a song before shutdown");

	ClearEvents();
	reg.Shutdown();
	Expect(Has("S:OnSongExit") && Has("S:OnDisabled") && Has("S:OnShutdown") && Has("S:destroy"),
		"shutdown runs the full teardown and destroys the mod");
	Expect(IndexOf("S:OnSongExit") < IndexOf("S:OnDisabled") &&
		IndexOf("S:OnDisabled") < IndexOf("S:OnShutdown") &&
		IndexOf("S:OnShutdown") < IndexOf("S:destroy"),
		"shutdown order: OnSongExit -> OnDisabled -> OnShutdown -> destruction");
}

static void Test_StartupOrderHarvestsSettingsBeforeDispatchInitialize() {
	ClearEvents();
	ModRegistry reg;
	auto m = std::make_unique<TestMod>("WithSettings");
	m->declaredSettings = {
		Framework::SettingDef::Toggle("TestToggle", "TestIni", "Test Label")
	};
	// Register mod (analogous to InstantiatePending / static ModRegistrar)
	reg.Register(std::move(m));

	// Before DispatchInitialize runs, SettingsSchema() must already contain the harvested setting
	const auto* def = Framework::SettingsSchema().Find("TestToggle");
	Expect(def != nullptr, "mod settings harvested immediately upon registration (before DispatchInitialize)");
	if (def) {
		Expect(def->ini.name == "TestIni", "harvested setting has correct ini.name");
		Expect(def->ini.section == "Toggle Switches", "harvested setting has correct default section");
	}

	// DispatchInitialize runs afterwards
	reg.DispatchInitialize();
	Expect(Has("WithSettings:OnInitialize"), "mod initializes after settings were registered");

	reg.Shutdown();
	// After shutdown, SettingsSchema cleans up settings owned by the mod
	Expect(Framework::SettingsSchema().Find("TestToggle") == nullptr, "schema cleaned up after mod shutdown");
}

int main() {
	std::cout << "ModRegistry state-machine tests\n";

	Test_InitializeFaultIsIsolated();
	Test_ActivateInMenu();
	Test_EnableMidSong();
	Test_DisableMidSong();
	Test_ReenableAfterDisable();
	Test_DisabledSameTickAsSongExit();
	Test_ConflictSuppressionOrder();
	Test_OnEnabledThrowsFaults();
	Test_OnSongEnterThrowsShortCircuits();
	Test_TickFailureFaultsImmediately();
	Test_SettingsAppliedThenNotifiedOnTick();
	Test_KeyAvailabilityTracksRegistryLifecycle();
	Test_ConflictSuppressionGatesEffectiveCommand();
	Test_DuplicateIdRejected();
	Test_StartupOrderHarvestsSettingsBeforeDispatchInitialize();
	Test_ShutdownRevertsAndDestroysInOrder();

	std::cout << (g_failures == 0 ? "\nALL PASS\n" : "\nFAILURES: " + std::to_string(g_failures) + "\n");
	return g_failures == 0 ? 0 : 1;
}
