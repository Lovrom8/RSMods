#include "stdafx.h"
#include "SettingsMenu.hpp"
#include "Settings.hpp"
#include "SettingEdits.hpp"
#include "Framework/Framework.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <unordered_map>

using Framework::SettingDef;
using Framework::SettingType;

namespace SettingsMenu {
	namespace {
		struct Group {
			std::string title;
			std::vector<SettingDef> settings;
		};

		const ImVec4 kErrorColor(0.95f, 0.40f, 0.40f, 1.0f);

		// Written once by Register before the entry is published; read-only afterwards.
		std::vector<Group> g_groups;
		std::unordered_map<std::string, const SettingDef*> g_byKey;

		// Render-thread only: the draw callback is the sole reader and writer.
		std::unordered_map<std::string, std::string> g_dragging;   // Slider values until release, when they're applied
		std::unordered_map<std::string, std::array<char, 1024>> g_textBuffers;
		std::string g_editingText;                                  // Key of the text field being typed into
		char g_filter[64] = {};

		// "TwoRTCBypassMod" -> "Two RTC Bypass"
		std::string TitleFromId(std::string_view id) {
			if (id.ends_with("Mod")) id.remove_suffix(3);

			std::string out;
			for (size_t i = 0; i < id.size(); ++i) {
				const bool upper = std::isupper(static_cast<unsigned char>(id[i]));
				const bool prevLower = i > 0 && std::islower(static_cast<unsigned char>(id[i - 1]));
				const bool acronymEnd = i > 0 && std::isupper(static_cast<unsigned char>(id[i - 1]))
					&& i + 1 < id.size() && std::islower(static_cast<unsigned char>(id[i + 1]));
				if (upper && (prevLower || acronymEnd)) out += ' ';
				out += id[i];
			}
			return out;
		}

		bool ContainsNoCase(std::string_view haystack, std::string_view needle) {
			return std::ranges::search(haystack, needle, [](char a, char b) {
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
			}).begin() != haystack.end();
		}

		std::string CurrentValue(const SettingDef& def) {
			if (auto it = g_dragging.find(def.key); it != g_dragging.end())
				return it->second;
			if (auto pending = SettingEdits::PendingValue(def.key))
				return *pending;
			return def.type == SettingType::Int
				? std::to_string(Settings::GetModSetting(def.key))
				: Settings::ReturnSettingValue(def.key);
		}

		bool IsVisible(const SettingDef& def) {
			if (!def.visibleWhen)
				return true;
			auto parent = g_byKey.find(def.visibleWhen->key);
			const std::string parentValue = parent != g_byKey.end()
				? CurrentValue(*parent->second)
				: Settings::ReturnSettingValue(def.visibleWhen->key);
			return parentValue == def.visibleWhen->equals;
		}

		void Apply(const SettingDef& def, std::string value) {
			SettingEdits::Apply({ def.key, def.ini.section, def.ini.name, std::move(value), def.label, def.type == SettingType::Int });
		}

		void DrawReadOnly(const SettingDef& def, const std::string& value) {
			ImGui::TextDisabled("%s: %s", def.label.c_str(), value.empty() ? "(none)" : value.c_str());
		}

		// Mirrors the GUI: [Toggle Switches] is on/off, elsewhere keep whichever form is stored.
		bool IsNumericBool(const SettingDef& def, const std::string& value) {
			if (def.ini.section == "Toggle Switches") return false;
			if (value == "0" || value == "1") return true;
			if (value == "on" || value == "off") return false;
			return def.def == "0" || def.def == "1";
		}

		void DrawBool(const SettingDef& def, const std::string& value) {
			const bool numeric = IsNumericBool(def, value);
			bool on = value == (numeric ? "1" : "on");
			if (ImGui::Checkbox(def.label.c_str(), &on))
				Apply(def, on ? (numeric ? "1" : "on") : (numeric ? "0" : "off"));
		}

		// Every number is edited as a float, so a scaled one (ms shown as seconds) needs no branch of its own.
		// Applied on release: applying every drag frame would run OnSettingsChanged per pixel.
		void DrawInt(const SettingDef& def, const std::string& value) {
			int stored = 0;
			try { stored = std::stoi(value); } catch (...) {}

			const double scale = def.scale.value_or(1.0);
			const int decimals = std::clamp(static_cast<int>(std::ceil(-std::log10(scale))), 0, 3);
			char format[8];
			snprintf(format, sizeof(format), "%%.%df", decimals);

			float shown = static_cast<float>(stored * scale);
			if (def.min && def.max)
				ImGui::SliderFloat(def.label.c_str(), &shown, static_cast<float>(*def.min * scale), static_cast<float>(*def.max * scale), format);
			else
				ImGui::DragFloat(def.label.c_str(), &shown, static_cast<float>(def.scale ? scale * 10 : 1.0), 0.0f, 0.0f, format);

			const std::string edited = std::to_string(std::lround(shown / scale));
			if (ImGui::IsItemActive())
				g_dragging[def.key] = edited;
			else
				g_dragging.erase(def.key);

			if (ImGui::IsItemDeactivatedAfterEdit())
				Apply(def, edited);
		}

		void DrawEnum(const SettingDef& def, const std::string& value) {
			if (def.choices.empty()) {
				DrawReadOnly(def, value);
				return;
			}

			if (ImGui::BeginCombo(def.label.c_str(), value.c_str())) {
				for (const auto& choice : def.choices) {
					const bool selected = choice == value;
					if (ImGui::Selectable(choice.c_str(), selected) && !selected)
						Apply(def, choice);
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		// Device and font lists come from the GUI's providers, so those stay read-only here.
		void DrawString(const SettingDef& def, const std::string& value) {
			if (!def.choicesSource.empty()) {
				DrawReadOnly(def, value);
				return;
			}

			// Refilled from the setting except while it's being typed into.
			auto& buffer = g_textBuffers[def.key];
			if (g_editingText != def.key)
				strncpy_s(buffer.data(), buffer.size(), value.c_str(), _TRUNCATE);

			ImGui::InputText(def.label.c_str(), buffer.data(), buffer.size());
			if (ImGui::IsItemActive())
				g_editingText = def.key;
			else if (g_editingText == def.key)
				g_editingText.clear();

			if (ImGui::IsItemDeactivatedAfterEdit())
				Apply(def, buffer.data());
		}

		void DrawSetting(const SettingDef& def) {
			const std::string value = CurrentValue(def);

			ImGui::PushID(def.key.c_str());
			switch (def.type) {
				case SettingType::Bool:   DrawBool(def, value); break;
				case SettingType::Int:    DrawInt(def, value); break;
				case SettingType::Enum:   DrawEnum(def, value); break;
				case SettingType::String: DrawString(def, value); break;
				case SettingType::Key:    // Key capture and colour picking stay in the GUI
				case SettingType::Color:  DrawReadOnly(def, value); break;
			}

			if (!def.hint.empty() && ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", def.hint.c_str());
			ImGui::PopID();
		}

		void Draw() {
			ImGui::SetWindowSize(ImVec2(560, 640), ImGuiCond_FirstUseEver);

			ImGui::TextWrapped("Changes apply right away and are saved to RSMods.ini. "
				"Some settings only take effect when the game starts.");

			const auto status = SettingEdits::LastStatus();
			if (status.error)
				ImGui::TextColored(kErrorColor, "%s", status.text.c_str());
			else if (!status.text.empty())
				ImGui::TextDisabled("%s", status.text.c_str());

			ImGui::InputTextWithHint("##filter", "Filter settings", g_filter, sizeof(g_filter));
			const std::string_view filter = g_filter;
			ImGui::Separator();

			for (const auto& group : g_groups) {
				const bool titleMatches = filter.empty() || ContainsNoCase(group.title, filter);

				std::vector<const SettingDef*> shown;
				for (const auto& def : group.settings) {
					if (IsVisible(def) && (titleMatches || ContainsNoCase(def.label, filter)))
						shown.push_back(&def);
				}
				if (shown.empty())
					continue;

				if (!filter.empty())
					ImGui::SetNextItemOpen(true, ImGuiCond_Always);
				if (ImGui::CollapsingHeader(group.title.c_str())) {
					ImGui::Indent();
					for (const auto* def : shown)
						DrawSetting(*def);
					ImGui::Unindent();
				}
			}
		}
	}

	void Register() {
		const auto& schema = Framework::SettingsSchema();

		std::unordered_map<const Framework::IMod*, size_t> groupIndex;
		for (auto& def : schema.GetAll()) {
			if (!def.editedBy.empty()) continue;                                 // A bespoke GUI editor owns the value
			if (!def.editor.empty() && def.type != SettingType::Bool) continue;  // Placeholder for that editor's button

			const auto* owner = schema.OwnerOf(def.key);
			auto [it, inserted] = groupIndex.try_emplace(owner, g_groups.size());
			if (inserted)
				g_groups.push_back(Group{ owner ? TitleFromId(owner->Id()) : "Other", {} });
			g_groups[it->second].settings.push_back(std::move(def));
		}

		std::ranges::sort(g_groups, {}, &Group::title);
		for (const auto& group : g_groups) {
			for (const auto& def : group.settings)
				g_byKey.emplace(def.key, &def);
		}

		Framework::Menus().Register(nullptr, "settings", "Mod Settings", 0, Draw, Framework::Availability::Initialized,
			/*standaloneWindow*/ true, /*playerFacing*/ true);
	}
}
