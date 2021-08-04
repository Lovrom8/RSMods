#pragma once

#include <string>
#include "CCEnums.hpp"
#include "../Lib/Json/json.hpp"

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

	extern void to_json_request(json& j, const Request& p);
	extern void from_json_request(const json& j, Request& p);
	extern void to_json_response(json& j, const Response& p);
	extern void from_json_response(const json& j, Response& p);
}