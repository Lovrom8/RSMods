#pragma once

// Hash interned JSON doubles without truncation; equal numbers retain equal hashes.
// Installation is byte-checked and idempotent.
namespace JsonNumberHash {
	bool Install();
}
