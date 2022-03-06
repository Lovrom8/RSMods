#pragma once

#include "Types.hpp"
#include "Exports.hpp"

namespace Wwise::MemoryMgr {
	AKRESULT CheckPoolId(AkMemPoolId in_poolId);
	AKRESULT CreatePool(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign);
	AKRESULT DestroyPool(AkMemPoolId in_poolId);
	AKRESULT Falign(AkMemPoolId in_poolId, void* in_pMemAddress);
	void* GetBlock(AkMemPoolId in_poolId);
	AkUInt32 GetBlockSize(AkMemPoolId in_poolId);
	AkInt32 GetMaxPools(void);
	AkInt32 GetNumPools(void);
	AkMemPoolAttributes GetPoolAttributes(AkMemPoolId in_poolId);
	void GetPoolMemoryUsed(AkMemPoolId in_poolId, PoolMemInfo* out_memInfo);
	AkOSChar* GetPoolName(AkMemPoolId in_poolId);
	AKRESULT GetPoolStats(AkMemPoolId in_poolId, PoolStats* out_stats);
	bool IsInitialized(void);
	void* Malign(AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment);
	void* Malloc(AkMemPoolId in_poolId, size_t in_uSize);
	AKRESULT ReleaseBlock(AkMemPoolId in_poolId, void* in_pMemAddress);
	AKRESULT SetMonitoring(AkMemPoolId in_poolId, bool in_bDoMonitor);
	AKRESULT SetPoolName(AkMemPoolId in_poolId, const char* in_pszPoolName);
	void Term(void);
}
typedef AKRESULT(__cdecl* tMemory_CheckPoolId)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_CreatePool)(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign);
typedef AKRESULT(__cdecl* tMemory_DestroyPool)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_Falign)(AkMemPoolId in_poolId, void* in_pMemAddress);
typedef void* (__cdecl* tMemory_GetBlock)(AkMemPoolId in_poolId);
typedef AkUInt32(__cdecl* tMemory_GetBlockSize)(AkMemPoolId in_poolId);
typedef AkInt32(__cdecl* tMemory_GetMaxPools)(void);
typedef AkInt32(__cdecl* tMemory_GetNumPools)(void);
typedef AkMemPoolAttributes(__cdecl* tMemory_GetPoolAttributes)(AkMemPoolId in_poolId);
typedef void(__cdecl* tMemory_GetPoolMemoryUsed)(AkMemPoolId in_poolId, PoolMemInfo* out_memInfo);
typedef AkOSChar* (__cdecl* tMemory_GetPoolName)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_GetPoolStats)(AkMemPoolId in_poolId, PoolStats* out_stats);
typedef bool(__cdecl* tMemory_IsInitialized)(void);
typedef void* (__cdecl* tMemory_Malign)(AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment);
typedef void* (__cdecl* tMemory_Malloc)(AkMemPoolId in_poolId, size_t in_uSize);
typedef AKRESULT(__cdecl* tMemory_ReleaseBlock)(AkMemPoolId in_poolId, void* in_pMemAddress);
typedef AKRESULT(__cdecl* tMemory_SetMonitoring)(AkMemPoolId in_poolId, bool in_bDoMonitor);
typedef AKRESULT(__cdecl* tMemory_SetPoolName)(AkMemPoolId in_poolId, const char* in_pszPoolName);
typedef void(__cdecl* tMemory_Term)(void);
