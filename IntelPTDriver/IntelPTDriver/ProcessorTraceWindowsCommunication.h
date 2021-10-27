#ifndef _PROCESSOR_TRACE_WINDOWS_COMMUNICATION_
#define _PROCESSOR_TRACE_WINDOWS_COMMUNICATION_

#include "DriverCommon.h"

typedef struct _IO_QUEUE_SETTINGS {

    BOOLEAN DefaultQueue;
    WDF_IO_QUEUE_DISPATCH_TYPE QueueType;

    struct CALLBACKS {
        PFN_WDF_IO_QUEUE_IO_DEFAULT IoQueueIoDefault;
        PFN_WDF_IO_QUEUE_IO_READ IoQueueIoRead;
        PFN_WDF_IO_QUEUE_IO_WRITE IoQueueIoWrite;
        PFN_WDF_IO_QUEUE_IO_STOP IoQueueIoStop;
        PFN_WDF_IO_QUEUE_IO_RESUME IoQueueIoResume;
        PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE IoQueueIoCanceledOnQueue;
        PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL IoQueueIoDeviceControl;
        PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL IoQueueIoInternalDeviceControl;
    } Callbacks;

} IO_QUEUE_SETTINGS;

VOID
PtwCommunicationIoControlCallback(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
);

NTSTATUS
PtwCommunicationInit(
    _In_ WDFDEVICE                  ControlDevice,
    _In_ IO_QUEUE_SETTINGS*         IoQueueSettings,
    _Inout_ WDFQUEUE*               Queue
);

NTSTATUS
PtwCommunicationUninit(
    _In_ WDFQUEUE Queue
);

VOID
__forceinline
PtwCommunicationIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}

#endif
