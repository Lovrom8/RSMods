#include "MemoryMgr.hpp"

namespace Wwise::MemoryMgr {
	AKRESULT CheckPoolId(AkMemPoolId in_poolId) {
		tMemory_CheckPoolId func = (tMemory_CheckPoolId)Wwise::Exports::func_Wwise_Memory_CheckPoolId;
		return func(in_poolId);
	}

	AKRESULT CreatePool(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign) {
		tMemory_CreatePool func = (tMemory_CreatePool)Wwise::Exports::func_Wwise_Memory_CreatePool;
		return func(in_pMemAddress, in_uMemSize, in_uBlockSize, in_eAttributes, in_uBlockAlign);
	}

	AKRESULT DestroyPool(AkMemPoolId in_poolId) {
		tMemory_DestroyPool func = (tMemory_DestroyPool)Wwise::Exports::func_Wwise_Memory_DestroyPool;
		return func(in_poolId);
	}

	AKRESULT Falign(AkMemPoolId in_poolId, void* in_pMemAddress) {
		tMemory_Falign func = (tMemory_Falign)Wwise::Exports::func_Wwise_Memory_Falign;
		return func(in_poolId, in_pMemAddress);
	}

	void* GetBlock(AkMemPoolId in_poolId) {
		tMemory_GetBlock func = (tMemory_GetBlock)Wwise::Exports::func_Wwise_Memory_GetBlock;
		return func(in_poolId);
	}

	AkUInt32 GetBlockSize(AkMemPoolId in_poolId) {
		tMemory_GetBlockSize func = (tMemory_GetBlockSize)Wwise::Exports::func_Wwise_Memory_GetBlockSize;
		return func(in_poolId);
	}

	AkInt32 GetMaxPools() {
		tMemory_GetMaxPools func = (tMemory_GetMaxPools)Wwise::Exports::func_Wwise_Memory_GetMaxPools;
		return func();
	}

	AkInt32 GetNumPools() {
		tMemory_GetNumPools func = (tMemory_GetNumPools)Wwise::Exports::func_Wwise_Memory_GetNumPools;
		return func();
	}

	AkMemPoolAttributes GetPoolAttributes(AkMemPoolId in_poolId) {
		tMemory_GetPoolAttributes func = (tMemory_GetPoolAttributes)Wwise::Exports::func_Wwise_Memory_GetPoolAttributes;
		return func(in_poolId);
	}

	void GetPoolMemoryUsed(AkMemPoolId in_poolId, PoolMemInfo* out_memInfo) {
		tMemory_GetPoolMemoryUsed func = (tMemory_GetPoolMemoryUsed)Wwise::Exports::func_Wwise_Memory_GetPoolMemoryUsed;
		return func(in_poolId, out_memInfo);
	}

	AkOSChar* GetPoolName(AkMemPoolId in_poolId) {
		tMemory_GetPoolName func = (tMemory_GetPoolName)Wwise::Exports::func_Wwise_Memory_GetPoolName;
		return func(in_poolId);
	}

	AKRESULT GetPoolStats(AkMemPoolId in_poolId, PoolStats* out_stats) {
		tMemory_GetPoolStats func = (tMemory_GetPoolStats)Wwise::Exports::func_Wwise_Memory_GetPoolStats;
		return func(in_poolId, out_stats);
	}

	bool IsInitialized() {
		tMemory_IsInitialized func = (tMemory_IsInitialized)Wwise::Exports::func_Wwise_Memory_IsInitialized;
		return func();
	}

	void* Malign(AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment) {
		tMemory_Malign func = (tMemory_Malign)Wwise::Exports::func_Wwise_Memory_Malign;
		return func(in_poolId, in_uSize, in_uAlignment);
	}

	void* Malloc(AkMemPoolId in_poolId, size_t in_uSize) {
		tMemory_Malloc func = (tMemory_Malloc)Wwise::Exports::func_Wwise_Memory_Malloc;
		return func(in_poolId, in_uSize);
	}

	AKRESULT ReleaseBlock(AkMemPoolId in_poolId, void* in_pMemAddress) {
		tMemory_ReleaseBlock func = (tMemory_ReleaseBlock)Wwise::Exports::func_Wwise_Memory_ReleaseBlock;
		return func(in_poolId, in_pMemAddress);
	}

	AKRESULT SetMonitoring(AkMemPoolId in_poolId, bool in_bDoMonitor) {
		tMemory_SetMonitoring func = (tMemory_SetMonitoring)Wwise::Exports::func_Wwise_Memory_SetMonitoring;
		return func(in_poolId, in_bDoMonitor);
	}

	AKRESULT SetPoolName(AkMemPoolId in_poolId, const char* in_pszPoolName) {
		tMemory_SetPoolName func = (tMemory_SetPoolName)Wwise::Exports::func_Wwise_Memory_SetPoolName;
		return func(in_poolId, in_pszPoolName);
	}

	void Term() {
		tMemory_Term func = (tMemory_Term)Wwise::Exports::func_Wwise_Memory_Term;
		return func();
	}
}