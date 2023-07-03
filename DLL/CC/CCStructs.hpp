#pragma once

#include <string>
#include "CCEnums.hpp"
#include "../Lib/Json/json.hpp"

using namespace CrowdControl::Enums;
using nlohmann::json;

namespace CrowdControl::Structs {
	struct Request {
		/**
		 * \brief The request ID, all responses to this request should contain the same ID value.
		 * \details These are not guaranteed to be sequential or increasing.
		 */
		uint32_t id;
		/**
		 * \brief The identifier of the requested effect.
		 * \details Only present on request types 0 (Test), 1(Start) and 2 (Stop).
		 */
		std::string code;
		/**
		 * \brief This field contains any parameters the user has selected.
		 */
		json parameters;
		/**
		 * \brief The displayable name of the viewer who requested the effect.
		 */
		std::string viewer;
		/**
		 * \brief The requested quantity of the effect, if applicable.
		 */
		int64_t quantity;
		/**
		 * \brief The requested duration of the effect, in milliseconds.
		 */
		int64_t duration;
		/**
		 * \brief The type of message. See RequestType enum.
		 */
		RequestType type;
	};

	struct Response {
		/**
		 * \brief The request ID, all responses to a request should contain the same ID value.
		 */
		int64_t id;
		/**
		 * \brief The identifier of the requested effect.
		 */
		std::string code;
		/**
		 * \brief Indicates the status of the effect request or class.
		 */
		EffectStatus status;
		/**
		 * \brief The requested duration of the effect, in milliseconds.
		 */
		int64_t timeRemaining;
		/**
		 * \brief Indicates the type of message. See ResponseType enum below.
		 */
		ResponseType type;
	};

	extern void to_json_request(json& j, const Request& p);
	extern void from_json_request(const json& j, Request& p);
	extern void to_json_response(json& j, const Response& p);
	extern void from_json_response(const json& j, Response& p);
}