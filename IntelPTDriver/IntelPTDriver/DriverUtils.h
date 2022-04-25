#ifndef _DRIVER_UTILS_H_
#define _DRIVER_UTILS_H_

#include "DriverCommon.h"

#define DuFreeBuffer(BufferVa)                                      MmFreeContiguousMemory(BufferVa)
#define DuIsBufferEmpty(QueueHead)                                  IsListEmpty(QueueHead->ListEntry)

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

NTSTATUS
DuFreeUserspaceMapping(
    PVOID BaseAddress,
    PMDL Mdl
);

typedef struct _QUEUE_HEAD_STRUCTURE {

    LIST_ENTRY *ListEntry;
    BOOLEAN Interlocked;

} QUEUE_HEAD_STRUCTURE;

NTSTATUS
DuQueueInit(
    QUEUE_HEAD_STRUCTURE** QueueHead,
    BOOLEAN Interlocked
);

NTSTATUS
DuEnqueueElement(
    QUEUE_HEAD_STRUCTURE* QueueHead,
    PVOID Data
);

NTSTATUS
DuDequeueElement(
    QUEUE_HEAD_STRUCTURE* QueueHead,
    PVOID* Data
);

NTSTATUS
DuEnqueueElements(
    QUEUE_HEAD_STRUCTURE* QueueHead,
    unsigned NumberOfElements,
    PVOID Elements[]
);

void
DuDumpMemory(
    PVOID* Va,
    unsigned Size
);

QUEUE_HEAD_STRUCTURE*   gQueueHead;
KEVENT                  gPagesAvailableEvent;
KMUTEX                  gCommMutex;

#endif