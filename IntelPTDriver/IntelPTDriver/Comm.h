#ifndef _COMM_H_
#define _COMM_H_

#include "DriverCommon.h"

typedef struct _IO_QUEUE_SETTINGS {

    PFN_WDF_IO_QUEUE_IO_DEFAULT IoQueueIoDefault;
    PFN_WDF_IO_QUEUE_IO_READ IoQueueIoRead;
    PFN_WDF_IO_QUEUE_IO_WRITE IoQueueIoWrite;
    PFN_WDF_IO_QUEUE_IO_STOP IoQueueIoStop;
    PFN_WDF_IO_QUEUE_IO_RESUME IoQueueIoResume;
    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE IoQueueIoCanceledOnQueue;
    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL IoQueueIoDeviceControl;
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL IoQueueIoInternalDeviceControl;

} IO_QUEUE_SETTINGS;

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
);

NTSTATUS
InitCommQueue(
    _In_ WDFDEVICE* ControlDevice,
    _In_ COMM_QUEUE_DEVICE_CONTEXT* Data,
    _In_ IO_QUEUE_SETTINGS* IoQueueSettings
)

#endif
