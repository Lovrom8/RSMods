#include "../stdafx.h"
#include "ControlServer.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "CCEffect.hpp"
#include "CCEffectList.hpp"
#include "CCEnums.hpp"

#include "../D3D/D3DHooks.hpp"

#pragma comment(lib, "Ws2_32.lib")

using namespace CrowdControl::Effects;
using namespace CrowdControl::EffectList;
using namespace CrowdControl::Enums;
using namespace CrowdControl::Structs;
using nlohmann::json;

namespace CrowdControl {
	namespace {
		std::jthread connectionThread;
		std::jthread effectThread;
		std::jthread objectUpdateThread;

		std::mutex waitMutex;
		std::condition_variable_any wake;
		std::mutex effectsMutex;

		std::atomic<SOCKET> activeSocket{ INVALID_SOCKET };

		bool WaitFor(std::stop_token stop, std::chrono::milliseconds duration) {
			std::unique_lock lock(waitMutex);
			wake.wait_for(lock, stop, duration, [] { return false; });
			return stop.stop_requested();
		}

		void CloseActiveSocket() {
			const SOCKET socket = activeSocket.exchange(INVALID_SOCKET);
			if (socket == INVALID_SOCKET) return;

			closesocket(socket);
		}
	}

	/// <summary>
	/// Runs/stops the current effect
	/// Effects are started externally (from CC or RSMods GUI), but each effect stops itself by running the Run method, which in turn calls Stop when the effect's duration runs out
	/// </summary>
	Response RunCommand(const Request& request) {
		std::lock_guard lock(effectsMutex);

		Response resp{
			request.id,
			request.code,
			EffectStatus::Unavailable,
			0,
			ResponseType::EffectRequest
		};

		if (GetAllEffects().find(request.code) != GetAllEffects().end()) {
			CCEffect* effect = GetAllEffects().at(request.code);

			switch (request.type)
			{
				case RequestType::Test:
					resp.status = effect->Test(request);
					break;
				case RequestType::Start:
					resp.status = effect->Start(request);
					break;
				case RequestType::Stop:
					resp.status = effect->Stop();
					break;
				default:
					break;
			}

			resp.timeRemaining = effect->duration_ms;
		}

		return resp;
	}

	/// <summary>
	/// Sends response to Crowd Control over TCP socket.
	/// </summary>
	void SendResponse(SOCKET socket, const Response& response) {
		//Serialize response
		json j;
		CrowdControl::Structs::to_json_response(j, response);
		std::string jsonstr = j.dump();

		LOG_INFO("Responding: " << jsonstr.c_str() << std::endl);

		//Send response
		send(socket, jsonstr.c_str(), jsonstr.length() + 1, 0); // Include null terminator for CC
	}

	/// <summary>
	/// Read the socket and perform the effects.
	/// If the effect was started, it sends a Response with code 0 (Success), otherwise it sends a Response with code 3 (Retry)
	/// </summary>
	void ClientLoop(std::stop_token stop, SOCKET socket) {
		LOG_INFO("Starting crowd control client loop" << std::endl);

		std::array<char, 512> buffer{};
		while (!stop.stop_requested()) {
			size_t currentMessageLength = 0;
			while (currentMessageLength < buffer.size()) {
				const int bytesRead = recv(socket, buffer.data() + currentMessageLength, 1, 0);
				if (bytesRead <= 0) return;
				if (buffer[currentMessageLength] == '\0') break;
				++currentMessageLength;
			}

			if (currentMessageLength == buffer.size()) {
				LOG_ERROR("Current message is longer than buffer size" << std::endl);
				return;
			}

			try {
				const std::string command(buffer.data(), currentMessageLength);
				const json j = json::parse(command);

				LOG_INFO("Received command:" << std::endl);
				LOG_INFO(j.dump(2) << std::endl);

				Request request;
				CrowdControl::Structs::from_json_request(j, request);

				LOG_INFO("Running command" << std::endl);
				const Response response = RunCommand(request);

				LOG_INFO("Responding to command" << std::endl);
				SendResponse(socket, response);
			}
			catch (const std::exception& ex) {
				LOG_ERROR("Invalid Crowd Control request: " << ex.what() << std::endl);
			}
		}
	}

	/// <summary>
	/// Opens a TCP socket on localhost, port 45659
	/// If a socket has been successfully opened, it connects to the effect client (CrowdControl or RSMods GUI)
	/// </summary>
	void CrowdControlThread(std::stop_token stop) {
		using namespace std::chrono_literals;

		while (!GameState::GameLoaded.load()) {
			if (WaitFor(stop, 5s)) return;
		}

		LOG_INFO("Crowd control server starting" << std::endl);

		//Create server address struct
		struct sockaddr_in server_address = {};

		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(45659);

		//Resolve and convert ip address
		if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
			LOG_ERROR("Invalid address" << std::endl);
			return;
		}

		while (!stop.stop_requested()) {
			LOG_INFO("Trying to connect to crowd control" << std::endl);

			//Open socket
			const SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);
			if (socket == INVALID_SOCKET) {
				LOG_ERROR("Unable to open socket for crowd control" << std::endl);
				return;
			}

			activeSocket.store(socket);
			if (stop.stop_requested()) {
				CloseActiveSocket();
				return;
			}

			//Connect
			const int connectErr = connect(socket, reinterpret_cast<const sockaddr*>(&server_address), sizeof(server_address));
			if (connectErr == SOCKET_ERROR) {
				const int err = WSAGetLastError();
				LOG_ERROR("Unable to connect to crowd control - " << err << std::endl);
				CloseActiveSocket();
				return;
			}

			LOG_INFO("Connected to crowd control" << std::endl);

			//Do client loop
			ClientLoop(stop, socket);
			CloseActiveSocket();

			LOG_INFO("Disconnected from crowd control" << std::endl);
			break;
		}

		LOG_INFO("Crowd control stopping" << std::endl);
	}

	/// <summary>
	/// Continously keep the effects running by calling the Run() method in their classes
	/// Calls all the effects in the list (AllEffects)
	/// If the current effect isn't enabled, there will be no visible effects 
	/// </summary>
	void EffectRunThread(std::stop_token stop) {
		using namespace std::chrono_literals;

		while (!GameState::GameLoaded.load()) {
			if (WaitFor(stop, 5s)) return;
		}

		while (!stop.stop_requested()) {
			// Iterate through all effects
			{
				std::lock_guard lock(effectsMutex);
				for (const auto& entry : GetAllEffects()) {
					entry.second->Run();
				}
			}

			if (WaitFor(stop, 10ms)) return;
		}
	}

	/// <summary>
	/// Object scaling effects need to be reapplied, because they are applied for each object separately 
	/// These effects cannot be used in Guitarcade modes
	/// </summary>
	void ObjectUtilUpdateThread(std::stop_token stop) {
		using namespace std::chrono_literals;

		while (!GameState::GameLoaded.load()) {
			if (WaitFor(stop, 5s)) return;
		}

		while (!stop.stop_requested()) {
			if (GameState::IsInSong()) // Guitarcade games crash if UpdateScales is run. So we will just sleep.
				ObjectUtil::UpdateScales();

			if (WaitFor(stop, 1s)) return;
		}
	}

	/// <summary>
	/// Starts or restarts only the TCP connection worker.
	/// </summary>
	void StartServerLoop() {
		connectionThread.request_stop();
		CloseActiveSocket();
		wake.notify_all();

		if (connectionThread.joinable()) connectionThread.join();
		connectionThread = std::jthread(CrowdControlThread);
	}

	/// <summary>
	/// Starts the necessary threads to listen and respond to Crowd Control requests
	/// </summary>
	void StartServer() {
		if (connectionThread.joinable()) return;

		StartServerLoop();
		effectThread = std::jthread(EffectRunThread);
		objectUpdateThread = std::jthread(ObjectUtilUpdateThread);

		// Patch scroll speed to be 10x faster
		MemUtil::PatchAdr(Offsets::patch_scrollSpeedLTTarget, (UINT*)Offsets::patch_scrollSpeedChange, 3);
		MemUtil::PatchAdr(Offsets::patch_scrollSpeedGTTarget, (UINT*)Offsets::patch_scrollSpeedChange, 3);
	}

	void StopServer() {
		connectionThread.request_stop();
		effectThread.request_stop();
		objectUpdateThread.request_stop();

		// request_stop() cannot interrupt a blocking recv(), so close the socket as well.
		CloseActiveSocket();
		wake.notify_all();

		if (connectionThread.joinable()) connectionThread.join();
		if (effectThread.joinable()) effectThread.join();
		if (objectUpdateThread.joinable()) objectUpdateThread.join();
	}

	namespace Structs {
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
			if (j.contains("id"))
				j.at("id").get_to(p.id);

			if (j.contains("code"))
				j.at("code").get_to(p.code);

			if (j.contains("viewer"))
				j.at("viewer").get_to(p.viewer);

			if (j.contains("type"))
				j.at("type").get_to(p.type);

			if (j.contains("duration"))
				j.at("duration").get_to(p.duration);

			if (j.contains("parameters"))
				j.at("parameters").get_to(p.parameters);
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
}
