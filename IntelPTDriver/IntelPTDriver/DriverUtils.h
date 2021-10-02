#ifndef _DRIVER_UTILS_H_
#define _DRIVER_UTILS_H_

#include "DriverCommon.h"

#define DuFreeBuffer(BufferVa, BufferSizeInBytes, CachingType)     MmFreeContiguousMemorySpecifyCache(BufferVa, BufferSizeInBytes, CachingType)

NTSTATUS
DuAllocateBuffer(
    size_t BufferSizeInBytes,
    MEMORY_CACHING_TYPE CachingType,
    BOOLEAN ZeroBuffer,
    PVOID* BufferVa,
    PVOID* BufferPa
);

NTSTATUS
DuMapBufferInUserspace(
    PVOID BufferKernelAddress,
    size_t BufferSizeInBytes,
    PMDL* Mdl,
    PVOID* BufferUserAddress
);


#endif