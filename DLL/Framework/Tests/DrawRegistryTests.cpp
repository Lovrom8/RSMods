#include "../DrawRegistry.hpp"
#include "../IMod.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// Dummy types for unit tests
struct Mesh {
	unsigned int Stride = 0;
	unsigned int PrimCount = 0;
	unsigned int NumVertices = 0;
};

struct ThiccMesh {
	unsigned int Stride = 0;
	unsigned int PrimCount = 0;
	unsigned int NumVertices = 0;
};

using Framework::DrawRegistry;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
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
}

int main() {
	std::cout << "DrawRegistry tests\n";

	MockMod modA("ModA");
	MockMod modB("ModB");
	MockMod modC("ModC");
	Mesh dummyMesh;
	ThiccMesh dummyThicc;

	// 1. Priority ordering: lower priority runs first, tie-breaks on owner ID.
	{
		DrawRegistry reg;
		reg.Register(&modB, "interceptorB", 10, DrawPath::Indexed, [](DrawContext&) {
			return DrawResult{ DrawOutcome::Pass };
		});
		reg.Register(&modA, "interceptorA1", 10, DrawPath::Indexed, [](DrawContext&) {
			return DrawResult{ DrawOutcome::Pass };
		});
		reg.Register(&modC, "interceptorC", 5, DrawPath::Indexed, [](DrawContext&) {
			return DrawResult{ DrawOutcome::Pass };
		});

		reg.RebuildActive([](const IMod*) { return true; });
		auto snap = reg.ActiveSnapshot(DrawPath::Indexed);

		Check("snapshot has 3 entries", snap->size() == 3);
		Check("lowest priority (5) runs first", (*snap)[0].ownerId == "ModC" && (*snap)[0].priority == 5);
		Check("tied priority (10) breaks on ownerId asc (ModA)", (*snap)[1].ownerId == "ModA" && (*snap)[1].priority == 10);
		Check("tied priority (10) breaks on ownerId asc (ModB)", (*snap)[2].ownerId == "ModB" && (*snap)[2].priority == 10);
	}

	// 2. Snapshot rebuild: only currently-enabled owners are in snapshot.
	{
		DrawRegistry reg;
		reg.Register(&modA, "a", 0, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });
		reg.Register(&modB, "b", 0, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });

		// Only modA enabled
		reg.RebuildActive([&](const IMod* m) { return m == &modA; });
		auto snap1 = reg.ActiveSnapshot(DrawPath::Indexed);
		Check("only enabled mod appears in active snapshot", snap1->size() == 1 && (*snap1)[0].ownerId == "ModA");

		// Now enable modB, disable modA
		reg.RebuildActive([&](const IMod* m) { return m == &modB; });
		auto snap2 = reg.ActiveSnapshot(DrawPath::Indexed);
		Check("rebuild refreshes active entries", snap2->size() == 1 && (*snap2)[0].ownerId == "ModB");

		// All disabled
		reg.RebuildActive([](const IMod*) { return false; });
		auto snap3 = reg.ActiveSnapshot(DrawPath::Indexed);
		Check("disabled all results in empty snapshot", snap3->empty());
	}

	// 3. RemoveMod drops all interceptors owned by that mod.
	{
		DrawRegistry reg;
		reg.Register(&modA, "a1", 1, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });
		reg.Register(&modA, "a2", 2, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });
		reg.Register(&modB, "b1", 1, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });

		reg.RemoveMod(&modA);
		reg.RebuildActive([](const IMod*) { return true; });
		auto snap = reg.ActiveSnapshot(DrawPath::Indexed);
		Check("RemoveMod drops only target owner's interceptors", snap->size() == 1 && (*snap)[0].ownerId == "ModB");
	}

	// 4. Path routing: Indexed, Primitive, Both.
	{
		DrawRegistry reg;
		reg.Register(&modA, "idxOnly", 0, DrawPath::Indexed, [](DrawContext&) { return DrawResult{}; });
		reg.Register(&modB, "primOnly", 0, DrawPath::Primitive, [](DrawContext&) { return DrawResult{}; });
		reg.Register(&modC, "both", 0, DrawPath::Both, [](DrawContext&) { return DrawResult{}; });

		reg.RebuildActive([](const IMod*) { return true; });
		auto snapIdx = reg.ActiveSnapshot(DrawPath::Indexed);
		auto snapPrim = reg.ActiveSnapshot(DrawPath::Primitive);

		Check("Indexed path receives Indexed and Both", snapIdx->size() == 2);
		Check("Primitive path receives Primitive and Both", snapPrim->size() == 2);
	}

	// 5. Execution walk: outcomes Pass, ReplaceTexture, Hide, Show.
	{
		DrawRegistry reg;
		std::vector<std::string> executionOrder;

		reg.Register(&modA, "inter1", 1, DrawPath::Indexed, [&](DrawContext&) {
			executionOrder.push_back("inter1");
			return DrawResult{ DrawOutcome::ReplaceTexture, 1, nullptr };
		});
		reg.Register(&modB, "inter2", 2, DrawPath::Indexed, [&](DrawContext&) {
			executionOrder.push_back("inter2");
			return DrawResult{ DrawOutcome::Pass };
		});
		reg.Register(&modC, "inter3", 3, DrawPath::Indexed, [&](DrawContext&) {
			executionOrder.push_back("inter3");
			return DrawResult{ DrawOutcome::Hide };
		});

		MockMod modD("ModD");
		reg.Register(&modD, "inter4", 4, DrawPath::Indexed, [&](DrawContext&) {
			executionOrder.push_back("inter4");
			return DrawResult{ DrawOutcome::Pass };
		});

		reg.RebuildActive([](const IMod*) { return true; });
		auto snap = reg.ActiveSnapshot(DrawPath::Indexed);

		DrawContext ctx(nullptr, dummyMesh, dummyThicc, DrawPath::Indexed, false);
		DrawOutcome finalOutcome = DrawOutcome::Pass;
		for (const auto& e : *snap) {
			DrawResult r = e.fn(ctx);
			if (r.outcome == DrawOutcome::Hide || r.outcome == DrawOutcome::Show) {
				finalOutcome = r.outcome;
				break;
			}
		}

		Check("execution order stops at terminal outcome (Hide)",
			executionOrder.size() == 3 &&
			executionOrder[0] == "inter1" &&
			executionOrder[1] == "inter2" &&
			executionOrder[2] == "inter3");
		Check("terminal outcome is Hide", finalOutcome == DrawOutcome::Hide);
	}

	// 6. DrawContext StageCRC caching and custom provider injection.
	{
		int crcComputeCount = 0;
		auto customCrc = [&](IDirect3DDevice9*, DWORD stage) -> std::optional<DWORD> {
			++crcComputeCount;
			if (stage == 1) return 0x12345678;
			return std::nullopt;
		};

		DrawContext ctx(nullptr, dummyMesh, dummyThicc, DrawPath::Indexed, false, customCrc);

		auto crc1_first = ctx.StageCRC(1);
		auto crc1_second = ctx.StageCRC(1);
		auto crc2_first = ctx.StageCRC(2);

		Check("StageCRC returns computed CRC", crc1_first.has_value() && *crc1_first == 0x12345678);
		Check("StageCRC cached on repeated query", crc1_second.has_value() && *crc1_second == 0x12345678);
		Check("StageCRC compute count is 1 for stage 1", crcComputeCount == 2); // stage 1 once, stage 2 once
		Check("StageCRC returns nullopt for non-existent texture", !crc2_first.has_value());
	}

	// 7. Texture regeneration fan-out and lifecycle cleanup.
	{
		DrawRegistry reg;
		int modARegenCount = 0;
		int modBRegenCount = 0;
		IDirect3DDevice9* passedDevice = nullptr;

		reg.RegisterTextureRegen(&modA, [&](IDirect3DDevice9* dev) {
			++modARegenCount;
			passedDevice = dev;
		});

		reg.RegisterTextureRegen(&modB, [&](IDirect3DDevice9*) {
			++modBRegenCount;
		});

		// Fake device pointer for test
		IDirect3DDevice9* dummyDevice = reinterpret_cast<IDirect3DDevice9*>(static_cast<std::uintptr_t>(0xDEADBEEF));

		// Null device is a no-op
		reg.RegenerateAllTextures(nullptr);
		Check("null device does not invoke callbacks", modARegenCount == 0 && modBRegenCount == 0);

		// Non-null device runs all registered callbacks
		reg.RegenerateAllTextures(dummyDevice);
		Check("RegenerateAllTextures invokes registered callbacks", modARegenCount == 1 && modBRegenCount == 1);
		Check("passed device pointer matches", passedDevice == dummyDevice);

		// Re-registering replaces callback rather than duplicating
		reg.RegisterTextureRegen(&modA, [&](IDirect3DDevice9*) {
			modARegenCount += 10;
		});
		reg.RegenerateAllTextures(dummyDevice);
		Check("re-registration replaces callback without duplication", modARegenCount == 11 && modBRegenCount == 2);

		// RemoveMod unregisters texture regen callbacks for that mod
		reg.RemoveMod(&modA);
		reg.RegenerateAllTextures(dummyDevice);
		Check("RemoveMod unregisters texture regen callbacks for owner", modARegenCount == 11 && modBRegenCount == 3);
	}

	// 8. Deferred texture release lifecycle (RegisterTextureLifecycle, RequestTextureRelease, RunPendingReleases, CancelTextureRelease).
	{
		DrawRegistry reg;
		int modARegenCount = 0;
		int modAReleaseCount = 0;
		int modBReleaseCount = 0;

		reg.RegisterTextureLifecycle(
			&modA,
			[&](IDirect3DDevice9*) { ++modARegenCount; },
			[&]() { ++modAReleaseCount; }
		);

		reg.RegisterTextureLifecycle(
			&modB,
			nullptr,
			[&]() { ++modBReleaseCount; }
		);

		// Non-null device runs registered regen callback
		IDirect3DDevice9* dummyDevice = reinterpret_cast<IDirect3DDevice9*>(static_cast<std::uintptr_t>(0xDEADBEEF));
		reg.RegenerateAllTextures(dummyDevice);
		Check("RegisterTextureLifecycle wires regen callback", modARegenCount == 1);

		// No release requested yet -> RunPendingReleases is a no-op
		reg.RunPendingReleases();
		Check("RunPendingReleases without requests does nothing", modAReleaseCount == 0 && modBReleaseCount == 0);

		// Enqueue release for modA idempotently (multiple requests = 1 release invocation)
		reg.RequestTextureRelease(&modA);
		reg.RequestTextureRelease(&modA);
		reg.RequestTextureRelease(&modB);

		// Run pending releases drains both callbacks
		reg.RunPendingReleases();
		Check("RunPendingReleases drains requested releases once (idempotent enqueue)", modAReleaseCount == 1 && modBReleaseCount == 1);

		// Subsequent RunPendingReleases does not invoke again (drained)
		reg.RunPendingReleases();
		Check("RunPendingReleases is cleared after drain", modAReleaseCount == 1 && modBReleaseCount == 1);

		// CancelTextureRelease cancels a pending release before drain
		reg.RequestTextureRelease(&modA);
		reg.CancelTextureRelease(&modA);
		reg.RunPendingReleases();
		Check("CancelTextureRelease removes pending release before drain", modAReleaseCount == 1);

		// RemoveMod clears pending releases and unregisters callbacks
		reg.RequestTextureRelease(&modA);
		reg.RemoveMod(&modA);
		reg.RunPendingReleases();
		Check("RemoveMod cancels pending releases and unregisters release callback", modAReleaseCount == 1);
	}

	std::cout << (g_failures == 0 ? "ALL DRAWREGISTRY TESTS PASSED\n" : "DRAWREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
