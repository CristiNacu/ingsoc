#pragma once
#ifndef _DRIVER_COMMON_H_
#define _DRIVER_COMMON_H_

#include <ntddk.h>
#include <wdf.h>
#include <wdfobject.h>
#include <intrin.h>
#include "DriverUtils.h"

#include "Public.h"
#include "Debug.h"
#include "ProcessorTraceShared.h"

typedef struct _COMM_QUEUE_DEVICE_CONTEXT {
    PVOID            Data;
    WDFQUEUE         NotificationQueue;
    volatile LONG    Sequence;
} COMM_QUEUE_DEVICE_CONTEXT, * PCOMM_QUEUE_DEVICE_CONTEXT;


typedef NTSTATUS (*COMM_IO_COMMAND)(
        size_t InputBufferLength,
        size_t OutputBufferLength,
        PVOID *InputBuffer,
        PVOID *OutputBuffer,
        UINT32 *bytesWritten
    );

typedef struct _PER_CPU_DATA_STRUCTURE {

    unsigned CpuId;

} PER_CPU_DATA_STRUCTURE;

// TODO: Add driver relevant data here
typedef struct _DRIVER_GLOBAL_DATA {

    COMM_IO_COMMAND *IoCallbacks;
    KMUTEX SequenceMutex;
    volatile unsigned long SequenceIdCounter;
    volatile unsigned long PacketIdCounter;

} DRIVER_GLOBAL_DATA;


DRIVER_GLOBAL_DATA gDriverData;



WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(COMM_QUEUE_DEVICE_CONTEXT, CommGetContextFromDevice);


#endif