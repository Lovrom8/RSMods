	// The zero-keep-names path only reads node+0 before release.
	struct BorrowedName { const char* text = nullptr; };
	static_assert(offsetof(BorrowedName, text) == 0, "Populate reads text at node+0");
	thread_local BorrowedName g_borrowedName;
	void* __cdecl BorrowName(int length, const char* text, const unsigned char* archive) {
		if (!active || !active->valid || length < 0 || !archive || archive[0x61]
			|| *reinterpret_cast<const void* const*>(archive + 0x58) != active->nativeToc)
			return nullptr;
		const std::string_view name(text, static_cast<size_t>(length));
		const auto& hashes = active->package.hashes;
		const auto found = std::lower_bound(hashes.begin(), hashes.end(), name,
			[](const EnumerationPackage::Hash& hash, std::string_view key) {
				return hash.name < key;
			});
		if (found == hashes.end() || found->name != name) return nullptr;
		g_borrowedName.text = found->name.c_str();
		return &g_borrowedName;
	}
	bool __cdecl IsBorrowedName(const void* node) {
		return active && node == &g_borrowedName;
	}
