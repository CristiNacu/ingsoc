#ifndef _DRIVER_UTILS_H_
#define _DRIVER_UTILS_H_

#include "DriverCommon.h"

NTSTATUS
DuAllocatePtBuffer(
    size_t BufferSizeInBytes,
    MEMORY_CACHING_TYPE CachingType,
    PVOID* BufferVaKm,
    PVOID* BufferVaUm,
    PVOID* BufferPa
);

#endif