#pragma once

#include <string>
#include "CCStructs.hpp"
#include "CCEnums.hpp"

using namespace CrowdControl::Enums;
using nlohmann::json;

namespace CrowdControl::Structs {
	
	/// <summary>
	/// Convert the information about the current effect which contains special parameters to JSON
	/// </summary>
	/// <param name="j"> - Reference to the destination JSON object</param>
	/// <param name="p"> - Struct describing the current effect</param>
	/// <returns>Serialized JSON for the current effect</returns>
	void to_json_request(json& j, const Request& p) {
		j = json{
			{"id", p.id},
			{"code", p.code},
			{"parameters", p.parameters},
			{"duration", p.duration},
			{"viewer", p.viewer},
			{"type", p.type}
		};
	}

	/// <summary>
	/// Convert the information about the current effect which contains special parameters from JSON to Request struct
	/// </summary>
	/// <param name="j"> - Reference to the JSON object describing the current effect</param>
	/// <param name="p"> - Destination Request struct</param>
	/// <returns>Request object describing the current effect</returns>
	void from_json_request(const json& j, Request& p) {
		j.at("id").get_to(p.id);
		j.at("code").get_to(p.code);
		j.at("viewer").get_to(p.viewer);
		j.at("type").get_to(p.type);

		if (j.contains("duration"))
		{
			j.at("duration").get_to(p.duration);
		}

		if(j.contains("parameters"))
		{
			j.at("parameters").get_to(p.parameters);
		}
	}

	/// <summary>
	/// Convert the information about the current effect which doesn't contain special parameters to JSON
	/// </summary>
	/// <param name="j"> - Reference to the destination JSON object</param>
	/// <param name="p"> - Struct describing the current effect</param>
	/// <returns>Serialized JSON for the current effect</returns>
	void to_json_response(json& j, const Response& p) {
		j = json{
			{"id", p.id},
			{"status", p.status},
			{"code", p.code},
			{"timeRemaining", p.timeRemaining},
			{"type", p.type}
		};
	}


	/// <summary>
	/// Convert the information about the current effect which doesn't contain special parameters from JSON to Request struct
	/// </summary>
	/// <param name="j"> - Reference to the JSON object describing the current effect</param>
	/// <param name="p"> - Destination Request struct</param>
	/// <returns>Request object describing the current effect</returns>
	void from_json_response(const json& j, Response& p) {
		j.at("id").get_to(p.id);
		j.at("status").get_to(p.status);
		j.at("code").get_to(p.code);
		j.at("timeRemaining").get_to(p.timeRemaining);
		j.at("type").get_to(p.type);
	}
}