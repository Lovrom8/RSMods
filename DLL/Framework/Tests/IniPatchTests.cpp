#include "../../IniPatch.hpp"

#include <iostream>
#include <string>

namespace {
	int g_failures = 0;

	void Check(const std::string& name, const std::string& actual, const std::string& expected) {
		if (actual == expected) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		std::cout << "  FAIL  " << name << "\n    expected: [" << expected << "]\n    actual:   [" << actual << "]\n";
	}
}

int main() {
	std::cout << "IniPatch tests\n";
	using IniPatch::SetValue;

	Check("replaces a value in place, other lines untouched",
		SetValue("[Toggle Switches]\nA=on\nB=off\n[Mod Settings]\nB=3\n", "Toggle Switches", "B", "on"),
		"[Toggle Switches]\nA=on\nB=on\n[Mod Settings]\nB=3\n");

	Check("keeps CRLF, comments, unknown keys and spacing",
		SetValue("; header\r\n[Toggle Switches]\r\n;B=old\r\nUnknown = 1\r\nB = off\r\n", "Toggle Switches", "B", "on"),
		"; header\r\n[Toggle Switches]\r\n;B=old\r\nUnknown = 1\r\nB = on\r\n");

	Check("matches section and key case-insensitively, keeps the file's spelling",
		SetValue("[toggle switches]\nToggleLoft=off\n", "Toggle Switches", "toggleloft", "on"),
		"[toggle switches]\nToggleLoft=on\n");

	Check("leaves a commented copy alone",
		SetValue("[S]\n;K=1\nK=2\n", "S", "K", "3"),
		"[S]\n;K=1\nK=3\n");

	Check("sets every active duplicate",
		SetValue("[S]\nK=1\nK=2\n", "S", "K", "3"),
		"[S]\nK=3\nK=3\n");

	Check("same key in another section is untouched",
		SetValue("[A]\nK=1\n[B]\nK=1\n", "B", "K", "2"),
		"[A]\nK=1\n[B]\nK=2\n");

	Check("missing key goes after the section's last key, before the blank line",
		SetValue("[A]\nX=1\n\n[B]\nY=2\n", "A", "K", "v"),
		"[A]\nX=1\nK=v\n\n[B]\nY=2\n");

	Check("missing key skips the next section's header comments",
		SetValue("[A]\nX=1\n; about B\n[B]\n", "A", "K", "v"),
		"[A]\nX=1\nK=v\n; about B\n[B]\n");

	Check("missing key in the last section without a final line break",
		SetValue("[A]\r\nX=1", "A", "K", "v"),
		"[A]\r\nX=1\r\nK=v\r\n");

	Check("missing key in an empty section goes under its header",
		SetValue("[A]\n[B]\nY=2\n", "A", "K", "v"),
		"[A]\nK=v\n[B]\nY=2\n");

	Check("missing section is appended",
		SetValue("[A]\nX=1\n", "B", "K", "v"),
		"[A]\nX=1\n\n[B]\nK=v\n");

	Check("empty file gets the section",
		SetValue("", "B", "K", "v"),
		"[B]\nK=v\n");

	Check("empty value is written",
		SetValue("[A]\nK=old\n", "A", "K", ""),
		"[A]\nK=\n");

	std::cout << "IniPatch tests complete with " << g_failures << " failures.\n";
	return g_failures == 0 ? 0 : 1;
}
