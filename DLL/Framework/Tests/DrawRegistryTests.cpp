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

	std::cout << (g_failures == 0 ? "ALL DRAWREGISTRY TESTS PASSED\n" : "DRAWREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
