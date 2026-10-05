#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

// Sets one INI key in place, leaving every other byte as it was: comments, ordering, unknown keys,
// spacing and line endings. The GUI owns the file's layout; the game only ever changes values.
namespace IniPatch {
	namespace Detail {
		inline std::string_view Trim(std::string_view s) {
			const size_t first = s.find_first_not_of(" \t");
			return first == std::string_view::npos ? std::string_view() : s.substr(first, s.find_last_not_of(" \t") - first + 1);
		}

		inline bool EqualsNoCase(std::string_view a, std::string_view b) {
			return std::ranges::equal(a, b, [](char x, char y) {
				return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
			});
		}

		inline bool IsComment(std::string_view trimmed) {
			return !trimmed.empty() && (trimmed.front() == ';' || trimmed.front() == '#');
		}

		// "[ Name ]" -> "Name"
		inline std::optional<std::string_view> SectionName(std::string_view trimmed) {
			if (trimmed.size() < 2 || trimmed.front() != '[' || trimmed.back() != ']')
				return std::nullopt;
			return Trim(trimmed.substr(1, trimmed.size() - 2));
		}

		// Where the value starts on an active "key = value" line for `key`, keeping the spacing after '='.
		inline std::optional<size_t> ValueStart(std::string_view line, std::string_view key) {
			const size_t equals = line.find('=');
			if (equals == std::string_view::npos || IsComment(Trim(line)) || !EqualsNoCase(Trim(line.substr(0, equals)), key))
				return std::nullopt;
			const size_t value = line.find_first_not_of(" \t", equals + 1);
			return value == std::string_view::npos ? line.size() : value;
		}
	}

	// `section` is the bare name ("Toggle Switches"). Every active copy of the key in every copy of the section
	// is set, since both the game and the GUI let the last one win. A missing key goes after the last key of the
	// section's last copy, and a missing section at the end of the file.
	inline std::string SetValue(std::string_view text, std::string_view section, std::string_view key, std::string_view value) {
		using namespace Detail;

		const std::string_view newline = text.find("\r\n") != std::string_view::npos ? "\r\n" : "\n";
		std::string out;
		out.reserve(text.size() + key.size() + value.size() + 32);

		bool inSection = false;
		bool sectionSeen = false;
		bool keySet = false;
		size_t insertAt = 0; // Offset in `out` just past the section's last key line, or its header

		for (size_t pos = 0; pos < text.size(); ) {
			// One line, then its line break (none on a last line without one).
			size_t lineEnd = text.find('\n', pos);
			if (lineEnd == std::string_view::npos) lineEnd = text.size();
			const size_t next = lineEnd < text.size() ? lineEnd + 1 : lineEnd;
			const size_t contentEnd = lineEnd > pos && text[lineEnd - 1] == '\r' ? lineEnd - 1 : lineEnd;
			const std::string_view line = text.substr(pos, contentEnd - pos);
			const std::string_view trimmed = Trim(line);
			pos = next;

			const auto name = SectionName(trimmed);
			if (name) {
				inSection = EqualsNoCase(*name, section);
				sectionSeen |= inSection;
			}

			const auto valueStart = inSection && !name ? ValueStart(line, key) : std::nullopt;
			if (valueStart) {
				out += line.substr(0, *valueStart);
				out += value;
				keySet = true;
			}
			else {
				out += line;
			}
			out += text.substr(contentEnd, next - contentEnd);

			// Not after a trailing comment: the GUI writes a section's comments just above its header.
			if (inSection && !trimmed.empty() && !IsComment(trimmed))
				insertAt = out.size();
		}

		if (keySet)
			return out;

		std::string entry = std::string(key) + "=" + std::string(value) + std::string(newline);
		if (sectionSeen) {
			// The section's last line may be the file's last, without a line break of its own.
			if (insertAt == out.size() && !out.empty() && out.back() != '\n')
				entry.insert(0, newline);
			out.insert(insertAt, entry);
			return out;
		}

		if (!out.empty() && out.back() != '\n')
			out += newline;
		if (!out.empty())
			out += newline;
		out += "[" + std::string(section) + "]" + std::string(newline) + entry;
		return out;
	}
}
