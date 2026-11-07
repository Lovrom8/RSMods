#pragma once

#include <atomic>
#include <memory>
#include <utility>
#include <version>

namespace Framework {
	// A shared_ptr swapped atomically, for snapshots the render thread reads. libc++ (Xcode) has no
	// std::atomic<std::shared_ptr>, so there it's the older free functions, which take a brief internal lock.
#if defined(__cpp_lib_atomic_shared_ptr) && !defined(RSMODS_NO_ATOMIC_SHARED_PTR)
	template <typename T>
	using AtomicSharedPtr = std::atomic<std::shared_ptr<T>>;
#else
	template <typename T>
	class AtomicSharedPtr {
	public:
		[[nodiscard]] std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const {
			return std::atomic_load_explicit(&value, order);
		}

		void store(std::shared_ptr<T> next, std::memory_order order = std::memory_order_seq_cst) {
			std::atomic_store_explicit(&value, std::move(next), order);
		}

	private:
		std::shared_ptr<T> value;
	};
#endif
}
