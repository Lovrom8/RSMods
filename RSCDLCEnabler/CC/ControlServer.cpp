#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "ControlServer.hpp"
#include "../D3D/D3DHooks.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include "CCEffect.hpp"
#include <nlohmann\json.hpp>
#include "Effects/RainbowEffect.hpp"
#include "Effects/DrunkModeEffect.hpp"
#include "Effects/FYourFCEffect.hpp"
#include "Effects/SolidNotesEffect.hpp"
#include "Effects/TransparentNotesEffect.hpp"
#include "Effects/RemoveNotesEffect.hpp"
#include "Effects/ChangeToToneSlot.hpp"
#include "Effects/KillGuitarVolumeEffect.hpp"
#include "Effects/KillMusicVolumeEffect.hpp"

//Link the winsock2 lib
#pragma comment(lib, "Ws2_32.lib")

using namespace CrowdControl::Effects;
using namespace CrowdControl::Structs;
using nlohmann::json;

namespace CrowdControl {
	int sock;

	std::map<std::string, CCEffect*> AllEffects{
		{ "rainbow60", new RainbowEffect(60) },
		{ "drunk20", new DrunkModeEffect(20) },
		{ "fyourfc5", new FYourFCEffect(5) },
		{ "solidrandom10", new SolidNotesRandomEffect(10) },
		{ "solidcustom10", new SolidNotesCustomEffect(10) },
		{ "solidcustomrgb10", new SolidNotesCustomRGBEffect(10) },
		{ "removenotes20", new RemoveNotesEffect(20) },
		{ "transparentnotes20", new TransparentNotesEffect(20) },
		{ "changetoneslot1", new ChangeToToneSlot(1) },
		{ "changetoneslot2", new ChangeToToneSlot(2) },
		{ "changetoneslot3", new ChangeToToneSlot(3) },
		{ "changetoneslot4", new ChangeToToneSlot(4) },
		{ "killguitarvolume10", new KillGuitarVolumeEffect(10) },
		{ "killmusicvolume10", new KillMusicVolumeEffect(10) }
	};

	Response RunCommand(Request request) {
		Response resp{
			request.id,
			EffectResult::Unavailable,
			("Effect: " + request.code)
		};

		// Find the effect
		if (AllEffects.find(request.code) != AllEffects.end()) {
			// Get pointer to effect
			CCEffect* effect = AllEffects[request.code];

			// Start/Stop the effect
			switch (request.type)
			{
			case Test:
				resp.status = effect->Test(request);
				break;
			case Start:
				resp.status = effect->Start(request);
				break;
			case Stop:
				resp.status = effect->Stop();
				break;
			default:
				break;
			}
		}

		return resp;
	}

	void SendResponse(Response response) {
		//Serialize response
		json j = response;
		std::string jsonstr = j.dump();

		std::cout << "Responding: " << jsonstr.c_str() << std::endl;

		//Send response
		send(sock, jsonstr.c_str(), jsonstr.length(), NULL);

		//Send null terminator
		char buffer[1] = { 0 };
		send(sock, buffer, sizeof(buffer), NULL);
	}

	void ClientLoop() {
		std::cout << "Starting crowd control client loop" << std::endl;

		int currentMessageLength = 0;
		int bytesRead = 0;
		char buffer[1024];

		while (!D3DHooks::GameClosing) {
			//Receive command
			currentMessageLength = 0;

			do {
				//Read one byte at a time until null byte is read
				if (currentMessageLength >= sizeof(buffer)) {
					std::cout << "Current message is longer than buffer size" << std::endl;
					return;
				}

				//Read 1 byte from socket
				bytesRead = recv(sock, buffer + currentMessageLength, 1, NULL);

				//If last byte was null byte, exit recv loop
				if (buffer[currentMessageLength] == NULL) break;

				currentMessageLength += bytesRead;
			} while (bytesRead > 0);

			//Parse command
			std::string command = std::string(&buffer[0], &buffer[currentMessageLength]);

			json j = json::parse(command);
			auto request = j.get<Request>();

			std::cout << "Received command:" << std::endl;
			std::cout << j.dump(2) << std::endl;

			//Run command
			std::cout << "Running command" << std::endl;
			Response response = RunCommand(request);

			//Respond
			std::cout << "Responding to command" << std::endl;

			SendResponse(response);
		}
	}

	unsigned WINAPI CrowdControlThread(void*) {
		while (!D3DHooks::GameLoaded) // We are in no hurry :)
			Sleep(5000);

		std::cout << "Crowd control server starting" << std::endl;

		//Create server address struct
		struct sockaddr_in server_address = {};

		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(45659);

		//Resolve and convert ip address
		if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
			std::cout << "Invalid address" << std::endl;
			return -1;
		}

		while (!D3DHooks::GameClosing) {
			std::cout << "Trying to connect to crowd control" << std::endl;

			//Open socket
			if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				std::cout << "Unable to open socket for crowd control" << std::endl;
				return -1;
			}

			//Connect
			if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
				std::cout << "Unable to connect to crowd control" << std::endl;
				return -1;
			}

			std::cout << "Connected to crowd control" << std::endl;

			//Do client loop
			ClientLoop();

			std::cout << "Disconnected from crowd control" << std::endl;
		}

		std::cout << "Crowd control stopping" << std::endl;

		return 0;
	}

	unsigned WINAPI EffectRunThread(void*) {
		while (!D3DHooks::GameLoaded) // We are in no hurry :)
			Sleep(5000);

		while (!D3DHooks::GameClosing) {
			// Iterate through all effects
			for (std::map<std::string, CrowdControl::Effects::CCEffect*>::iterator it = AllEffects.begin(); it != AllEffects.end(); ++it) {
				// Run/Update all effects
				it->second->Run();
			}

			Sleep(10);
		}

		return 0;
	}

	void StartServer() {
		// Main TCP socket thread
		_beginthreadex(NULL, 0, &CrowdControlThread, NULL, 0, 0);
		// Effect updating thread
		_beginthreadex(NULL, 0, &EffectRunThread, NULL, 0, 0);
	}
}