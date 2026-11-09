#pragma once

// Defer DLC PSARC opens until needed; cache boot reads by path, size and write time.
namespace LazyVfs {
	// Install callbacks at DLL attach.
	void Install();

	// True when cached boot reads make file warming unnecessary.
	bool Covers(const wchar_t* path);

	void Report(const char* when);
}
