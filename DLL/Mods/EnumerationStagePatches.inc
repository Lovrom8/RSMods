	bool AddStagePatches(std::vector<Patch>& patches) {

		if (!Offsets::enumStageInternCall || !Offsets::enumStageIntern
			|| !Offsets::enumStageReleaseStart || !Offsets::enumStageReleaseEnd) return false;
		g_nativeIntern = Offsets::enumStageIntern;
		g_internFallback = g_nativeIntern;
		g_internLock = Offsets::enumStageInternLock;
		g_releaseBody = uintptr_t(Offsets::enumStageReleaseStart) + 10;
		g_releaseDone = Offsets::enumStageReleaseEnd;
		patches.push_back(Branch(Offsets::enumStageInternCall, g_nativeIntern, &BorrowInternHook, 0xE8));
		if (!Offsets::enumStageInternLock || !Offsets::enumStagePackages) return false;
		patches.push_back(Branch(Offsets::enumStageReleaseStart, 0, &ReleaseStage, 0xE9,
			Span(Offsets::enumStageReleaseStart, { 0x85, 0xFF, 0x74, 0x77, 0x8B, 0x35, 0, 0, 0, 0 },
				{ Abs(6, Offsets::enumStageInternLock) })));
		if (!Offsets::enumStageDuplicateStart || !Offsets::enumStageDuplicateCreate
			|| !Offsets::enumStageDuplicateHit || !Offsets::enumStageInsert
			|| !Offsets::enumStageRemove || !Offsets::enumStageTeardown) return false;
		g_packages = Offsets::enumStagePackages;
		g_duplicateBody = uintptr_t(Offsets::enumStageDuplicateStart) + 5;
		g_indexCreate = Offsets::enumStageDuplicateCreate;
		g_indexHit = Offsets::enumStageDuplicateHit;
		g_insertDlc = uintptr_t(Offsets::enumStageInsert) + 8;
		g_insertDone = Offsets::enumStageInsertNonDlc;
		g_removeDone = uintptr_t(Offsets::enumStageRemove) + 6;
		g_teardownDone = uintptr_t(Offsets::enumStageTeardown) + 6;
		patches.push_back(Branch(Offsets::enumStageDuplicateStart, 0, &IndexedDuplicateHook, 0xE9,
			Span(Offsets::enumStageDuplicateStart, { 0xA1, 0, 0, 0, 0 }, { Abs(1, Offsets::enumStagePackages) })));
		patches.push_back(Branch(Offsets::enumStageInsert, 0, &IndexInsertHook, 0xE9,
			{ 0x84, 0xDB, 0x0F, 0x85, 0x0F, 0x01, 0x00, 0x00 }));
		patches.push_back(Branch(Offsets::enumStageRemove, 0, &IndexRemoveHook, 0xE9,
			{ 0x83, 0x46, 0x04, 0xFC, 0x85, 0xDB }));
		patches.push_back(Branch(Offsets::enumStageTeardown, 0, &IndexTeardownHook, 0xE9,
			Span(Offsets::enumStageTeardown, { 0x89, 0x1D, 0, 0, 0, 0 }, { Abs(2, Offsets::enumStagePackages) })));
		return true;
	}
