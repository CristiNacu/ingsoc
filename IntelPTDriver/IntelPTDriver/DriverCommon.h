#ifndef _DRIVER_COMMON_H_
#define _DRIVER_COMMON_H_

#include <ntddk.h>
#include <wdf.h>
#include <wdfobject.h>
#include <intrin.h>

#define DEBUGGING_ACTIVE TRUE      //  Set to true for debugging logs / dbgbreaks

typedef struct _COMM_QUEUE_DEVICE_CONTEXT {
    PVOID            Data;
    WDFQUEUE         NotificationQueue;
    volatile LONG    Sequence;
} COMM_QUEUE_DEVICE_CONTEXT, * PCOMM_QUEUE_DEVICE_CONTEXT;


// TODO: Add driver relevant data here
typedef struct _DRIVER_GLOBAL_DATA {

    BOOLEAN placeholder;

} DRIVER_GLOBAL_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(COMM_QUEUE_DEVICE_CONTEXT, CommGetContextFromDevice);

DRIVER_GLOBAL_DATA gDriverData;

#endif