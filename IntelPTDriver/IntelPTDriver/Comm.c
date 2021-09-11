#include "Comm.h"
#include "Debug.h"

NTSTATUS
InitCommQueue(
    _In_ WDFDEVICE                   *ControlDevice,
    _In_ COMM_QUEUE_DEVICE_CONTEXT   *Data,
    _In_ IO_QUEUE_SETTINGS           *IoQueueSettings
)
{
    NTSTATUS                        status;
    PCOMM_QUEUE_DEVICE_CONTEXT      devContext = NULL;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };
    WDFQUEUE                        queue = { 0 };

    DEBUG_PRINT("InitCommQueue\n");

    DEBUG_STOP();
    DEBUG_PRINT("Device Addr %p\n", ControlDevice);
    DEBUG_STOP();
    DEBUG_PRINT("Device %p\n", (*ControlDevice));
    DEBUG_STOP();
    DEBUG_PRINT("Get context:\n");

    devContext = CommGetContextFromDevice(*ControlDevice);
    if (devContext == NULL)
    {
        DEBUG_PRINT("CommGetContextFromDevice context null\n");
        return STATUS_INVALID_DEVICE_STATE;
    }


    devContext->Data = Data;
    devContext->Sequence = 0;
    devContext->NotificationQueue = NULL;
    DEBUG_STOP();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

    DEBUG_STOP();

    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests = ((ULONG)-1);

    ioQueueConfig.EvtIoDefault = IoQueueSettings->IoQueueIoDefault;
    ioQueueConfig.EvtIoRead = IoQueueSettings->IoQueueIoRead;
    ioQueueConfig.EvtIoWrite = IoQueueSettings->IoQueueIoWrite;
    ioQueueConfig.EvtIoStop = IoQueueSettings->IoQueueIoStop;
    ioQueueConfig.EvtIoResume = IoQueueSettings->IoQueueIoResume;
    ioQueueConfig.EvtIoCanceledOnQueue = IoQueueSettings->IoQueueIoCanceledOnQueue;

    ioQueueConfig.EvtIoDeviceControl = IoQueueSettings->IoQueueIoDeviceControl;
    ioQueueConfig.EvtIoInternalDeviceControl = IoQueueSettings->IoQueueIoInternalDeviceControl;

    ioQueueConfig.PowerManaged = WdfFalse;

    DEBUG_STOP();

    status = WdfIoQueueCreate(*ControlDevice, &ioQueueConfig, NULL, &queue);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
        return status;
    }

    DEBUG_STOP();

    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

    DEBUG_STOP();


    status = WdfIoQueueCreate(*ControlDevice, &ioQueueConfig, NULL, &devContext->NotificationQueue);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
        return status;
    }
    DEBUG_STOP();

    WdfControlFinishInitializing(*ControlDevice);
    DEBUG_STOP();

    return status;
}

VOID
CommEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT32 bytesWritten = 0;
    BOOLEAN completeRequest = TRUE;

    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(WdfIoQueueGetDevice(Queue));
    if (!ctx)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, bytesWritten);
        return;
    }

    if()

}