#pragma once

#include "CCStructs.hpp"

using namespace CrowdControl::Structs;

namespace CrowdControl {
	void StartServer();
	Response RunCommand(Request request);
}