#include "MemoryMgr.hpp"

// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_memory_mgr.html

namespace Wwise::MemoryMgr {

	/// <summary>
	/// Test the validity of a pool ID. This is used to verify the validity of a memory pool ID.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool to test</param>
	/// <returns>AK_Success if the pool exists, AK_InvalidID otherwise.</returns>
	AKRESULT CheckPoolId(AkMemPoolId in_poolId) {
		tMemory_CheckPoolId func = (tMemory_CheckPoolId)Wwise::Exports::func_Wwise_Memory_CheckPoolId;
		return func(in_poolId);
	}
	/// <summary>
	/// Create a new memory pool.
	/// </summary>
	/// <param name="in_pMemAddress">- Memory address of the pool, or NULL if it should be allocated</param>
	/// <param name="in_uMemSize"> - Size of the pool (in bytes)</param>
	/// <param name="in_uBlockSize"> - Size of a block (in bytes)</param>
	/// <param name="in_eAttributes"> - Memory pool attributes: use values of AkMemPoolAttributes</param>
	/// <param name="in_uBlockAlign"> - Alignment of memory blocks</param>
	/// <returns>The ID of the created memory pool, or AK_INVALID_POOL_ID if creation failed.</returns>
	AKRESULT CreatePool(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign) {
		tMemory_CreatePool func = (tMemory_CreatePool)Wwise::Exports::func_Wwise_Memory_CreatePool;
		return func(in_pMemAddress, in_uMemSize, in_uBlockSize, in_eAttributes, in_uBlockAlign);
	}

	/// <summary>
	/// Destroy a memory pool.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <returns>AK_Success if successful</returns>
	AKRESULT DestroyPool(AkMemPoolId in_poolId) {
		tMemory_DestroyPool func = (tMemory_DestroyPool)Wwise::Exports::func_Wwise_Memory_DestroyPool;
		return func(in_poolId);
	}

	/// <summary>
	/// Free memory from a pool, overriding the pool's default memory alignment. Needs to be used in conjunction with MemoryMgr::Malign.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <param name="in_pMemAddress"> - Pointer to the start of memory allocated with Malloc</param>
	/// <returns>AK_Success if successful</returns>
	AKRESULT Falign(AkMemPoolId in_poolId, void* in_pMemAddress) {
		tMemory_Falign func = (tMemory_Falign)Wwise::Exports::func_Wwise_Memory_Falign;
		return func(in_poolId, in_pMemAddress);
	}

	/// <summary>
	/// NOT THREAD SAFE! Get a block from a Fixed-Size Block type pool. To be used with pools created with AkFixedSizeBlocksMode block management type, along with any of the block allocation types.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <returns>A pointer to the start of the allocated memory (NULL if the system is out of memory) The size of the memory block is always in_uBlockSize, specified in AK::MemoryMgr::CreatePool.</returns>
	void* GetBlock(AkMemPoolId in_poolId) {
		tMemory_GetBlock func = (tMemory_GetBlock)Wwise::Exports::func_Wwise_Memory_GetBlock;
		return func(in_poolId);
	}

	/// <summary>
	/// Get block size of blocks obtained with GetBlock() for a given memory pool. The block size is fixed and set when creating a pool with AkFixedSizeBlocksMode.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <returns>Block size</returns>
	AkUInt32 GetBlockSize(AkMemPoolId in_poolId) {
		tMemory_GetBlockSize func = (tMemory_GetBlockSize)Wwise::Exports::func_Wwise_Memory_GetBlockSize;
		return func(in_poolId);
	}

	/// <summary>
	/// Get the maximum number of memory pools.
	/// </summary>
	/// <returns>The maximum number of memory pools</returns>
	AkInt32 GetMaxPools() {
		tMemory_GetMaxPools func = (tMemory_GetMaxPools)Wwise::Exports::func_Wwise_Memory_GetMaxPools;
		return func();
	}

	/// <summary>
	/// Get the current number of memory pools.
	/// </summary>
	/// <returns>The current number of memory pools</returns>
	AkInt32 GetNumPools() {
		tMemory_GetNumPools func = (tMemory_GetNumPools)Wwise::Exports::func_Wwise_Memory_GetNumPools;
		return func();
	}

	/// <summary>
	/// Get pool attributes.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <returns>The memory pool's attributes.</returns>
	AkMemPoolAttributes GetPoolAttributes(AkMemPoolId in_poolId) {
		tMemory_GetPoolAttributes func = (tMemory_GetPoolAttributes)Wwise::Exports::func_Wwise_Memory_GetPoolAttributes;
		return func(in_poolId);
	}

	/// <summary>
	/// Get a memory pool current used size. Mostly used by the memory threshold features. If this function cannot be implemented if your memory manager, at least set the member uUsed to 0, that will disable the memory threshold feature.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <param name="out_memInfo"> - Returned statistics structure</param>
	void GetPoolMemoryUsed(AkMemPoolId in_poolId, PoolMemInfo* out_memInfo) {
		tMemory_GetPoolMemoryUsed func = (tMemory_GetPoolMemoryUsed)Wwise::Exports::func_Wwise_Memory_GetPoolMemoryUsed;
		return func(in_poolId, out_memInfo);
	}

	/// <summary>
	/// Get the name of a memory pool.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <returns>A pointer to the name of the memory pool (NULL if the operation failed)</returns>
	AkOSChar* GetPoolName(AkMemPoolId in_poolId) {
		tMemory_GetPoolName func = (tMemory_GetPoolName)Wwise::Exports::func_Wwise_Memory_GetPoolName;
		return func(in_poolId);
	}

	/// <summary>
	/// Get a memory pool's statistics.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <param name="out_stats"> - Returned statistics structure</param>
	/// <returns></returns>
	AKRESULT GetPoolStats(AkMemPoolId in_poolId, PoolStats* out_stats) {
		tMemory_GetPoolStats func = (tMemory_GetPoolStats)Wwise::Exports::func_Wwise_Memory_GetPoolStats;
		return func(in_poolId, out_stats);
	}

	/// <summary>
	/// NOT THREAD SAFE! Query whether the Memory Manager has been sucessfully initialized.
	/// </summary>
	/// <returns>True if the Memory Manager is initialized, False otherwise</returns>
	bool IsInitialized() {
		tMemory_IsInitialized func = (tMemory_IsInitialized)Wwise::Exports::func_Wwise_Memory_IsInitialized;
		return func();
	}

	/// <summary>
	/// Allocate memory from a pool, overriding the pool's default memory alignment. Needs to be used in conjunction with AK::MemoryMgr::Falign.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <param name="in_uSize"> - Number of bytes to allocate</param>
	/// <param name="in_uAlignment"> - Alignment (in bytes)</param>
	/// <returns>A pointer to the start of the allocated memory (NULL if the system is out of memory)</returns>
	void* Malign(AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment) {
		tMemory_Malign func = (tMemory_Malign)Wwise::Exports::func_Wwise_Memory_Malign;
		return func(in_poolId, in_uSize, in_uAlignment);
	}

	/// <summary>
	/// Allocate memory from a pool.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <param name="in_uSize"> - Number of bytes to allocate</param>
	/// <returns>A pointer to the start of the allocated memory (NULL if the system is out of memory)</returns>
	void* Malloc(AkMemPoolId in_poolId, size_t in_uSize) {
		tMemory_Malloc func = (tMemory_Malloc)Wwise::Exports::func_Wwise_Memory_Malloc;
		return func(in_poolId, in_uSize);
	}

	/// <summary>
	/// NOT THREAD SAFE! Free memory from a Fixed-Size Block type pool.
	/// </summary>
	/// <param name="in_poolId"> - ID of the memory pool</param>
	/// <param name="in_pMemAddress"> - Pointer to the start of memory allocated with Malloc</param>
	/// <returns>AK_Success if successful</returns>
	AKRESULT ReleaseBlock(AkMemPoolId in_poolId, void* in_pMemAddress) {
		tMemory_ReleaseBlock func = (tMemory_ReleaseBlock)Wwise::Exports::func_Wwise_Memory_ReleaseBlock;
		return func(in_poolId, in_pMemAddress);
	}

	/// <summary>
	/// Enables or disables error notifications posted by a memory pool. The notifications are enabled by default when creating a pool. They are always disabled in the Release build.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <param name="in_bDoMonitor"> - Enables error monitoring (has no effect in Release build)</param>
	/// <returns>AK_Success if the pool exists</returns>
	AKRESULT SetMonitoring(AkMemPoolId in_poolId, bool in_bDoMonitor) {
		tMemory_SetMonitoring func = (tMemory_SetMonitoring)Wwise::Exports::func_Wwise_Memory_SetMonitoring;
		return func(in_poolId, in_bDoMonitor);
	}

	/// <summary>
	/// Set the name of a memory pool.
	/// </summary>
	/// <param name="in_poolId"> - ID of memory pool</param>
	/// <param name="in_pszPoolName"> - Pointer to name string</param>
	/// <returns>AK_Success if successful</returns>
	AKRESULT SetPoolName(AkMemPoolId in_poolId, const char* in_pszPoolName) {
		tMemory_SetPoolName func = (tMemory_SetPoolName)Wwise::Exports::func_Wwise_Memory_SetPoolName;
		return func(in_poolId, in_pszPoolName);
	}

	/// <summary>
	/// NOT THREAD SAFE! Terminate the Memory Manager.
	/// </summary>
	void Term() {
		tMemory_Term func = (tMemory_Term)Wwise::Exports::func_Wwise_Memory_Term;
		return func();
	}
}