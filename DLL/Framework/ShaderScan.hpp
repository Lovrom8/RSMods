#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace Framework::ShaderScan {
	inline bool ShouldSkip(bool enabled, bool installingDlc, bool provenShaderFree) {
		return enabled && installingDlc && provenShaderFree;
	}

	// Only a complete, validated package file list can establish shader absence.
	inline bool ProvesNoShaders(std::string_view names, std::size_t expectedFiles) {
		if (names.empty() || expectedFiles == 0)
			return false;
		std::size_t count = 0;
		while (!names.empty()) {
			const auto newline = names.find('\n');
			std::string path(names.substr(0, newline));
			if (path.empty())
				return false;
			for (char& c : path) {
				if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F)
					return false;
				if (c == '\\') c = '/';
				if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
			}
			if (path.front() == '/' || path.back() == '/' || path.find(':') != std::string::npos
				|| path == "." || path == ".." || path.starts_with("./") || path.starts_with("../")
				|| path.find("/./") != std::string::npos || path.find("/../") != std::string::npos
				|| path.ends_with("/.") || path.ends_with("/..") || path.find("//") != std::string::npos)
				return false;
			// Native mappings live under Shaders/Generated; retain other shader-like assets too.
			if (path.find("shader") != std::string::npos)
				return false;
			for (const auto suffix : { ".sha1", ".fx", ".fxo", ".hlsl", ".cso", ".vsh", ".psh", ".nsb" })
				if (path.ends_with(suffix))
					return false;
			++count;
			if (count > expectedFiles)
				return false;
			if (newline == std::string_view::npos)
				break;
			names.remove_prefix(newline + 1);
		}
		return count == expectedFiles;
	}
}
