#pragma once

#include "../Log.hpp"
#include "CCStructs.hpp"

namespace CrowdControl {
	void StartServer();
	void RequestReconnect(); // Any thread; returns at once.
	void StopServer();
	Structs::Response RunCommand(const Structs::Request& request);

	namespace Structs {
		void to_json_request(nlohmann::json& j, const Request& p);
		void from_json_request(const nlohmann::json& j, Request& p);
		void to_json_response(nlohmann::json& j, const Response& p);
		void from_json_response(const nlohmann::json& j, Response& p);
	}
}
