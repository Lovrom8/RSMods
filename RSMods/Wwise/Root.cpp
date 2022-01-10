#include "Root.hpp"

namespace Wwise {
	bool IsRestoreSinkRequested() {
		tIsRestoreSinkRequested func = (tIsRestoreSinkRequested)Wwise::Exports::func_Wwise_Root_IsRestoreSinkRequested;
		return func();
	}

	bool IsUsingDummySink(void) {
		tIsUsingDummySink func = (tIsUsingDummySink)Wwise::Exports::func_Wwise_Root_IsUsingDummySink;
		return func();
	}
}