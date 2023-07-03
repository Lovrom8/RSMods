#pragma once

namespace CrowdControl::Enums {
	enum class RequestType {
		/**
		 * \brief Respond as with a normal request but don’t actually activate the effect.
		 */
		Test = 0x00,
		/**
		 * \brief A standard effect request. A response is always required.
		 */
		Start = 0x01,
		/**
		 * \brief A request to stop all instances of the requested effect code.
		 */
		Stop = 0x02,
		/**
		 * \brief RESERVED This field will appear in a future release. Request information about the player.
		 */
		PlayerInfo = 0xE0,
		/**
		 * \brief Indicates that this is a login request or response.
		 */
		Login = 0xF0,
		/**
		 * \brief Can be used in either direction to keep connections open or test connection status. Responses are neither expected nor given.
		 */
		KeepAlive = 0xFF
	};

	enum class ResponseType
	{
		/**
		 * \brief A response to an effect request.
		 * \details The response ID should match the request ID. Multiple responses with the same ID are allowed in the case of timed effects.
		 */
		EffectRequest = 0x00,
		/**
		 * \brief A status update on a type of effect.
		 * \details These messages concern the specified effect in general, not a specific instance.
		 */
		EffectStatus = 0x01,
		/**
		 * \brief Indicates that this is a login request or response.
		 * \details Whether this represents a request or response depends on the value of AuthenticationMode.
		 */
		Login = 0xF0,
		/**
		 * \brief Indicates the login was successful.
		 */
		LoginSuccess = 0xF1,
		Disconnect = 0xFE,
		/**
		 * \brief Can be used in either direction to keep connections open or test connection status.
		 * \details Responses are neither expected nor given.Request ID will be 0.
		 */
		KeepAlive = 0xFF,
	};

	enum class EffectStatus {
		/**
		 * \brief The effect executed successfully.
		 */
		Success = 0x00,
		/**
		 * \brief The effect failed to trigger, but is still available for use. Viewer(s) will be refunded.
		 */
		Failure = 0x01,
		/**
		 * \brief Same as Failure but the effect is no longer available for use.
		 */
		Unavailable = 0x02,
		/**
		 * \brief The effect cannot be triggered right now, try again in a few seconds.
		 */
		Retry = 0x03
	};
}