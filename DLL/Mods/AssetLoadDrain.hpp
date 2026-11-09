#pragma once

// Batch loader steps during startup and DLC scans; restore native pacing after the first menu.
namespace AssetLoadDrain {
	void Install();
	void SetActive(bool active);
	void SetBootActive(bool active);
}
