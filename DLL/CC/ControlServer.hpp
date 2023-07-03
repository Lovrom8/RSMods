#pragma once

#include "CCStructs.hpp"
#include "../Log.hpp"

using namespace CrowdControl::Structs;

namespace CrowdControl {
	void StartServer();
	void StartServerLoop();
	Response RunCommand(const Request& request);
}