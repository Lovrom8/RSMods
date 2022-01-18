#include "Root.hpp"

// No documentation

namespace Wwise {

	/// <summary>
	/// No documentation.
	/// </summary>
	/// <returns>Restore Sink Requested</returns>
	bool IsRestoreSinkRequested() {
		tIsRestoreSinkRequested func = (tIsRestoreSinkRequested)Wwise::Exports::func_Wwise_Root_IsRestoreSinkRequested;
		return func();
	}

	/// <summary>
	/// No documentation.
	/// </summary>
	/// <returns>Using a dummy sink</returns>
	bool IsUsingDummySink() {
		tIsUsingDummySink func = (tIsUsingDummySink)Wwise::Exports::func_Wwise_Root_IsUsingDummySink;
		return func();
	}
}