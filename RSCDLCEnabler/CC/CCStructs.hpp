#pragma once

#include <string>
#include "CCEnums.hpp"
#include "../Lib/Json/json.hpp"

namespace CrowdControl::Structs {
	extern struct Request {
		unsigned int id;
		std::string code;
		std::string viewer;
		CrowdControl::Enums::RequestType type;
		nlohmann::json parameters;
	};

	extern struct Response {
		unsigned int id;
		CrowdControl::Enums::EffectResult status;
		std::string message;
	};

	extern void to_json(nlohmann::json& j, const Request& p);
	extern void from_json(const nlohmann::json& j, Request& p);
	extern void to_json(nlohmann::json& j, const Response& p);
	extern void from_json(const nlohmann::json& j, Response& p);
}