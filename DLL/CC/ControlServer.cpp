#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "ControlServer.hpp"
#include "../D3D/D3DHooks.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include "../Lib/Json/json.hpp"

#include "CCEffect.hpp"
#include "CCEffectList.hpp"
#include <thread>

//Link the winsock2 lib
#pragma comment(lib, "Ws2_32.lib")

using namespace CrowdControl::Effects;
using namespace CrowdControl::Structs;
using namespace CrowdControl::EffectList;
using nlohmann::json;

namespace CrowdControl {
	int sock;
	bool serverStarted = false;

	/// <summary>
	/// Runs/stops the current effect
	/// Effects are started externally (from CC or RSMods GUI), but each effect stops itself by running the Run method, which in turn calls Stop when the effect's duration runs out
	/// </summary>
	Response RunCommand(const Request& request) {
		Response resp{
			request.id,
			request.code,
			EffectStatus::Unavailable,
			0,
			ResponseType::EffectRequest
		};

		// Find the effect
		if (AllEffects.find(request.code) != AllEffects.end()) {
			// Get pointer to effect
			CCEffect* effect = AllEffects[request.code];

			// Start/Stop the effect
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
	/// Sends a response through the same socket it received the request from
	/// If the effect was started, it sends a Response with code 0 (Success), otherwise it sends a Response with code 3 (Retry)
	/// </summary>
	void SendResponse(const Response& response) {
		//Serialize response
		_LOG_INIT;

		json j;
		CrowdControl::Structs::to_json_response(j, response);
		std::string jsonstr = j.dump();

		_LOG("Responding: " << jsonstr.c_str() << std::endl);

		//Send response
		send(sock, jsonstr.c_str(), jsonstr.length(), NULL);

		//Send null terminator
		char buffer[1] = { 0 };
		send(sock, buffer, sizeof(buffer), NULL);
	}

	/// <summary>
	/// Waits for incoming requests, which are null terminated
	/// If the client's connection is closed, the current message length will be -1, hence the server can be stopped
	/// When a request is received, parse it as JSON, run the corresponding effect if no incompatible effects are currently running 
	/// If the effect was started, it sends a Response with code 0 (Success), otherwise it sends a Response with code 3 (Retry)
	/// </summary>
	void ClientLoop() {
		_LOG_INIT;

		_LOG("Starting crowd control client loop" << std::endl);

		int currentMessageLength = 0;
		int bytesRead = 0;
		char buffer[1024];

		while (!D3DHooks::GameClosing) {
			//Receive command
			currentMessageLength = 0;

			do {
				//Read one byte at a time until null byte is read
				if (currentMessageLength >= sizeof(buffer)) {
					_LOG_SETLEVEL(LogLevel::Error);
					_LOG("Current message is longer than buffer size" << std::endl);
					return;
				}

				//Read 1 byte from socket
				bytesRead = recv(sock, buffer + currentMessageLength, 1, NULL);

				//If last byte was null byte, exit recv loop
				if (bytesRead > 0)
					if (buffer[currentMessageLength] == NULL) break;

				currentMessageLength += bytesRead;
			} while (bytesRead > 0);

			if (currentMessageLength == -1) { //Usually happens when the connection closes (i.e. when the server is down)
				serverStarted = false;
				break;
			}

			//Parse command
			std::string command = std::string(&buffer[0], &buffer[currentMessageLength]);

			json j = json::parse(command);

			_LOG("Received command:" << std::endl);
			_LOG(j.dump(2) << std::endl);

			Request request;
			CrowdControl::Structs::from_json_request(j, request);

			//Run command
			_LOG("Running command" << std::endl);
			const Response response = RunCommand(request);

			//Respond
			_LOG("Responding to command" << std::endl);

			SendResponse(response);
		}
	}

	/// <summary>
	/// Opens a TCP socket on localhost, port 45659
	/// If a socket has been successfully opened, it connects to the effect client (CrowdControl or RSMods GUI)
	/// </summary>
	/// <returns>NULL. Loops while the game is open</returns>
	unsigned WINAPI CrowdControlThread() {
		_LOG_INIT;

		while (!D3DHooks::GameLoaded) 
			Sleep(5000);

		_LOG("Crowd control server starting" << std::endl);

		//Create server address struct
		struct sockaddr_in server_address = {};

		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(45659);

		//Resolve and convert ip address
		if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
			_LOG_SETLEVEL(LogLevel::Error);
			_LOG("Invalid address" << std::endl);
			return -1;
		}

		while (!D3DHooks::GameClosing) {
			_LOG("Trying to connect to crowd control" << std::endl);

			//Open socket
			if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				_LOG_SETLEVEL(LogLevel::Error);
				_LOG("Unable to open socket for crowd control" << std::endl);
				return -1;
			}

			//Connect
			if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
				_LOG_SETLEVEL(LogLevel::Error);
				_LOG("Unable to connect to crowd control" << std::endl);
				return -1;
			}
			else
				serverStarted = true;

			_LOG("Connected to crowd control" << std::endl);

			//Do client loop
			ClientLoop();

			_LOG("Disconnected from crowd control" << std::endl);
		}

		_LOG("Crowd control stopping" << std::endl);

		return 0;
	}

	/// <summary>
	/// Continously keep the effects running by calling the Run() method in their classes
	/// Calls all the effects in the list (AllEffects)
	/// If the current effect isn't enabled, there will be no visible effects 
	/// </summary>
	/// <returns>NULL. Loops while the game is open</returns>
	unsigned WINAPI EffectRunThread() {
		while (!D3DHooks::GameLoaded)
			Sleep(5000);

		while (!D3DHooks::GameClosing) {
			// Iterate through all effects
			for (auto it = AllEffects.begin(); it != AllEffects.end(); ++it) {
				// Run/Update all effects
				it->second->Run();
			}

			Sleep(10);
		}

		return 0;
	}

	/// <summary>
	/// Object scaling effects need to be reapplied, because they are applied for each object separately 
	/// These effects cannot be used in Guitarcade modes
	/// </summary>
	/// <returns>NULL. Loops while the game is open</returns>
	unsigned WINAPI ObjectUtilUpdateThread() {
		while (!D3DHooks::GameLoaded) 
			Sleep(5000);

		while (!D3DHooks::GameClosing) {
			if (MemHelpers::IsInSong()) // Guitarcade games crash if UpdateScales is run. So we will just sleep.
				ObjectUtil::UpdateScales();

			Sleep(1000);
		}

		return 0;
	}

	/// <summary>
	/// Starts the main server loop
	/// If it has been stopped (or hasn't ever been started), it restarts it
	/// </summary>
	/// <returns>Nothing</returns>
	void StartServerLoop() {
		if (!serverStarted)
			std::thread(CrowdControlThread).detach();
	}

	/// <summary>
	/// Starts the necessary threads to listen and respond to Crowd Control requests
	/// </summary>
	void StartServer() {
		// Main TCP socket thread
		StartServerLoop();
		// Effect updating thread
		std::thread(EffectRunThread).detach();
		// Object util scale updater thread
		std::thread(ObjectUtilUpdateThread).detach();

		// Patch scroll speed to be 10x faster
		MemUtil::PatchAdr((BYTE*)Offsets::patch_scrollSpeedLTTarget, (UINT*)Offsets::patch_scrollSpeedChange, 3);
		MemUtil::PatchAdr((BYTE*)Offsets::patch_scrollSpeedGTTarget, (UINT*)Offsets::patch_scrollSpeedChange, 3);
	}
}