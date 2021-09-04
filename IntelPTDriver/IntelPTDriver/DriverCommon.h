#ifndef _DRIVER_COMMON_H_
#define _DRIVER_COMMON_H_

#include <ntddk.h>
#include <wdf.h>
#include <wdfobject.h>
#include <intrin.h>

typedef struct _COMM_QUEUE_DEVICE_CONTEXT {
    PVOID            Data;
    WDFQUEUE         NotificationQueue;
    volatile LONG    Sequence;
} COMM_QUEUE_DEVICE_CONTEXT, * PCOMM_QUEUE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(COMM_QUEUE_DEVICE_CONTEXT, CommGetContextFromDevice)

#endif