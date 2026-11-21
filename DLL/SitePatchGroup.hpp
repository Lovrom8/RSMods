#pragma once

#include "Framework/InstructionPatch.hpp"
#include "Log.hpp"
#include "MemUtil.hpp"
#include "LnpSites.hpp"
#include "VersioningStruct.h"

#include <memory>
#include <sstream>
#include <vector>

namespace SitePatch {
	using namespace Framework::InstructionPatch;

	inline constexpr auto ReadSite = [](std::uintptr_t address, auto& bytes) {
		return MemUtil::TryRead(address, bytes);
	};

	template <std::size_t Size>
	SiteDescriptor<Size> MakeSite(const char* name, std::uintptr_t address,
		const VersionedPattern<Size>& expected, const std::array<std::uint8_t, Size>& replacement,
		typename SiteDescriptor<Size>::ReplacementBuilder buildReplacement = nullptr) {
		return Framework::InstructionPatch::MakeSite(name, address, ::GameVersion::Current(),
			expected, replacement, buildReplacement);
	}

	template <std::size_t Size>
	BytePattern<Size> ExpandLnpPattern(const char* text) {
		std::vector<unsigned char> expanded;
		if (!LnpSites::Expand(text, static_cast<std::uint32_t>(GameVersion::ModuleBase()), expanded)
			|| expanded.size() != Size)
			return UnavailablePattern<Size>();
		std::array<std::uint8_t, Size> bytes{};
		std::copy(expanded.begin(), expanded.end(), bytes.begin());
		return ExactPattern(bytes);
	}

	// State is unsynchronized; the owner must serialize install and restore calls.
	class PatchedSiteGroup {
	public:
		explicit PatchedSiteGroup(const char* tag) : tag_(tag) {}

		template <typename Read, typename... Sites>
		MemUtil::PatchResult Install(const char* action, Read&& read, const Sites&... sites) {
			if (patches_)
				return MemUtil::ToPatchResult(patches_->Outcome());
			return InstallCaptured(action, CaptureSites(std::forward<Read>(read), sites...));
		}

		MemUtil::PatchResult InstallCaptured(const char* action, CaptureResult captured) {
			if (patches_)
				return MemUtil::ToPatchResult(patches_->Outcome());

			captured_ = std::move(captured);
			if (captured_.failure != ValidationFailure::None) {
				LOG_WARNING("(" << tag_ << ") " << action << " left unchanged at " << captured_.site
					<< ": " << FailureName(captured_.failure) << std::endl);
				return MemUtil::PatchResult::Original;
			}

			patches_ = std::make_unique<MemUtil::PreparedPatchSet>();
			const bool appended = AppendCaptured(captured_, [&](const CapturedSite& site) {
				MemUtil::PreparedPatchRequest request{};
				request.address = reinterpret_cast<LPVOID>(site.address);
				request.kind = MemUtil::PreparedPatchKind::Bytes;
				request.expectedBytes = site.original;
				request.replacementBytes = site.replacement;
				request.length = site.original.size();
				return patches_->Append(request);
			});
			if (!appended) {
				LogResult(action, MemUtil::PatchResult::Original, patches_->FailureAddress(), false);
				patches_.reset();
				return MemUtil::PatchResult::Original;
			}

			for (const CapturedSite& site : captured_.sites)
				LogOriginal(site);

			const auto result = patches_->Commit();
			LogResult(action, result, patches_->FailureAddress(), false);
			LogProtectionFailure(action);
			if (result == MemUtil::PatchResult::Original)
				patches_.reset();
			return result;
		}

		MemUtil::PatchResult Restore(const char* action) {
			if (!patches_)
				return MemUtil::PatchResult::Original;

			for (const CapturedSite& site : captured_.sites)
				LogOriginal(site);

			const auto result = patches_->Rollback();
			LogResult(action, result, patches_->FailureAddress(), true);
			LogProtectionFailure(action);
			if (result == MemUtil::PatchResult::Original) {
				patches_.reset();
				captured_ = {};
			}
			return result;
		}

		bool NeedsLifetime() const noexcept { return patches_ != nullptr; }
		const CaptureResult& Captured() const noexcept { return captured_; }
		std::uintptr_t FailureAddress() const noexcept { return patches_ ? patches_->FailureAddress() : 0; }
		MemUtil::PatchOutcome Outcome() const noexcept { return patches_ ? patches_->Outcome() : MemUtil::PatchOutcome{}; }

	private:
		void LogOriginal(const CapturedSite& site) const {
			std::ostringstream text;
			for (std::uint8_t byte : site.original)
				text << std::hex << static_cast<unsigned int>(byte) << ' ';
			LOG_INFO("(" << tag_ << ") " << site.name << " original at 0x" << std::hex
				<< site.address << ": " << text.str() << std::dec << std::endl);
		}

		void LogResult(const char* action, MemUtil::PatchResult result, std::uintptr_t failureAddress,
			bool restoring) const {
			const char* site = SiteNameAt(captured_, failureAddress);
			if ((!restoring && result == MemUtil::PatchResult::Installed)
				|| (restoring && result == MemUtil::PatchResult::Original)) {
				LOG_INFO("(" << tag_ << ") " << action << " confirmed" << std::endl);
			}
			else if (result == MemUtil::PatchResult::RollbackIncomplete) {
				LOG_ERROR("(" << tag_ << ") " << action << " outcome is indeterminate at "
					<< (site ? site : "patch group") << std::endl);
			}
			else {
				LOG_WARNING("(" << tag_ << ") " << action << " left unchanged at "
					<< (site ? site : "patch group") << ": write was not applied" << std::endl);
			}
		}

		void LogProtectionFailure(const char* action) const {
			if (patches_ && !patches_->Outcome().protectionRestored)
				LOG_WARNING("(MEMORY) Page protection could not be restored after " << tag_ << " "
					<< action << std::endl);
		}

		const char* tag_;
		std::unique_ptr<MemUtil::PreparedPatchSet> patches_;
		CaptureResult captured_;
	};

	inline MemUtil::PatchResult InstallRetained(std::vector<std::unique_ptr<PatchedSiteGroup>>& owner,
		const char* tag, const char* action, CaptureResult captured) {
		owner.push_back(std::make_unique<PatchedSiteGroup>(tag));
		PatchedSiteGroup& group = *owner.back();
		const auto result = group.InstallCaptured(action, std::move(captured));
		if (!group.NeedsLifetime())
			owner.pop_back();
		return result;
	}

	inline void LogUnavailable(const char* tag, const char* action, const char* site) {
		LOG_WARNING("(" << tag << ") " << action << " left unchanged at " << site << ": "
			<< FailureName(ValidationFailure::Unavailable) << std::endl);
	}
}
