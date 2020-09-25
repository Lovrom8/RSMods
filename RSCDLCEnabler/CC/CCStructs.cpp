#pragma once

#include <string>
#include "CCStructs.hpp"
#include "CCEnums.hpp"

using namespace CrowdControl::Enums;
using nlohmann::json;

namespace CrowdControl::Structs {
	void to_json(json& j, const Request& p) {
		j = json{
			{"id", p.id},
			{"code", p.code},
			{"viewer", p.viewer},
			{"type", p.type}
		};
	}
	void from_json(const json& j, Request& p) {
		j.at("id").get_to(p.id);
		j.at("code").get_to(p.code);
		j.at("viewer").get_to(p.viewer);
		j.at("type").get_to(p.type);
	}

	void to_json(json& j, const Response& p) {
		j = json{
			{"id", p.id},
			{"status", p.status},
			{"message", p.message}
		};
	}
	void from_json(const json& j, Response& p) {
		j.at("id").get_to(p.id);
		j.at("status").get_to(p.status);
		j.at("message").get_to(p.message);
	}
}