#pragma once

#include "CCStructs.hpp"

using namespace CrowdControl::Structs;

namespace CrowdControl {
	void StartServer();
	void StartServerLoop();
	Response RunCommand(Request request);
}