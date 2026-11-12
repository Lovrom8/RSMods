#pragma once

#include "EnumerationPackage.hpp"

namespace EnumerationPreparation {
	struct CrtFree {
		void operator()(unsigned char* data) const;
	};
	using Buffer = std::unique_ptr<unsigned char, CrtFree>;

	struct Prepared {
		EnumerationPackage::Package package;
		Buffer toc;
		Buffer names;
		const void* nativeToc = nullptr;
		void* sourceStream = nullptr;
		std::string snapshot;
		bool valid = false;
		bool fromCache = false;
		bool shaderFree = false;
	};

	void NoteInstallFault();
	void ResetMapperRestore();
	void LogMapperRestore();
	void ResetIndexStats();
	void LogIndexStats();
	bool Install();
	bool IsInstalled();
	bool ActivePackageIsShaderFree();
	unsigned int CacheOnlyHits();
	std::unique_ptr<Prepared> Prepare(const std::wstring& path, std::string snapshot);
	void Begin(Prepared* prepared);
	// Main-thread VFS open duration in QPC ticks; Debug only.
	long long End();
}
