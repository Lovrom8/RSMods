#pragma once

#include <string>
#include "CCEnums.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Structs {
	struct Request {
		unsigned int id;
		std::string code;
		std::string viewer;
		RequestType type;
	};
	
	struct Response {
		unsigned int id;
		EffectResult status;
		std::string message;
	};
}