#include "../ShaderScan.hpp"
#include <cassert>
#include <iostream>

int main() {
	using Framework::ShaderScan::ProvesNoShaders;
	using Framework::ShaderScan::ShouldSkip;
	const auto song = "appid.appid\nsongs/arr/song_lead.xml\naudio/windows/song.wem\n";
	assert(ProvesNoShaders(song, 3));
	assert(ProvesNoShaders("appid.appid\nsongs\\arr\\song.json", 2));
	for (const auto name : { "Shaders/Generated/cache.sha1", "shaders/generated/cache",
		"data/SHADERS/GENERATED/cache", "Shaders\\Generated\\cache.sha1", "cache.cso",
		"material.fx", "program.hlsl", "cache.nsb", "some_shader.bin" }) {
		assert(!ProvesNoShaders(std::string("appid.appid\n") + name, 2));
	}
	assert(!ProvesNoShaders({}, 0));
	assert(!ProvesNoShaders({}, 1));
	assert(!ProvesNoShaders(song, 2));
	assert(!ProvesNoShaders(song, 4));
	assert(!ProvesNoShaders("appid.appid\n\nsong.json", 3));
	assert(!ProvesNoShaders(std::string_view("appid.appid\0song.json", 21), 1));
	for (const auto name : { "../song.json", "songs/../song.json", "songs/./song.json",
		"/song.json", "c:/song.json", "songs/", "songs//song.json", "song.json\r" }) {
		assert(!ProvesNoShaders(name, 1));
	}
	for (const bool enabled : { false, true })
		for (const bool installing : { false, true })
			for (const bool proven : { false, true })
				assert(ShouldSkip(enabled, installing, proven) == (enabled && installing && proven));
	std::cout << "Shader scan decisions ALL PASS\n";
}
