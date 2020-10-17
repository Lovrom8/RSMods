#pragma once

#include <string>
#include "CCEnums.hpp"
#include <nlohmann\json.hpp>

using namespace CrowdControl::Enums;
using nlohmann::json;

namespace CrowdControl::Structs {
	extern struct Request {
		unsigned int id;
		std::string code;
		std::string viewer;
		RequestType type;
		json parameters;
	};

	extern struct Response {
		unsigned int id;
		EffectResult status;
		std::string message;
	};

	extern void to_json(json& j, const Request& p);
	extern void from_json(const json& j, Request& p);
	extern void to_json(json& j, const Response& p);
	extern void from_json(const json& j, Response& p);
}