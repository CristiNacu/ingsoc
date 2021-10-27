#include "ProcessorTraceWindowsCommunication.h"
#include "Debug.h"

static BOOLEAN gDefaultQueueInitialized = FALSE;

NTSTATUS
PtwCommunicationGetRequestBuffers(
    _In_ WDFREQUEST Request,
    _In_ size_t InBufferSize,
    _In_ size_t OutBufferSize,
    _Out_ PVOID* InBuffer,
    _Out_ size_t* InBufferActualSize,
    _Out_ PVOID* OutBuffer,
    _Out_ size_t* OutBufferActualSize
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (InBufferSize)
    {
        status = WdfRequestRetrieveInputBuffer(
            Request,
            InBufferSize,
            InBuffer,
            InBufferActualSize
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("Failed retrieveing input buffer, status %X\n", status);
            return status;
        }
    }
    else
    {
        *InBufferActualSize = 0;
        InBuffer = NULL;
    }

    if (OutBufferSize)
    {
        status = WdfRequestRetrieveOutputBuffer(
            Request,
            OutBufferSize,
            OutBuffer,
            OutBufferActualSize
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("Failed retrieveing output buffer, status %X\n", status);
            return status;
        }
    }
    else
    {
        *OutBufferActualSize = 0;
        OutBuffer = NULL;
    }

    return status;
}

VOID
PtwCommunicationIoControlCallback(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    PVOID inBuffer, outBuffer;
    size_t inBufferSize, outBufferSize;
    UINT32 bytesWritten = 0;
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(Queue);

    status = PtwCommunicationGetRequestBuffers(
        Request,
        InputBufferLength,
        OutputBufferLength,
        &inBuffer,
        &inBufferSize,
        &outBuffer,
        &outBufferSize
    );
    if (!NT_SUCCESS(status))
    {
        // Todo: log error
        goto cleanup;
    }

    unsigned function;
    if (IoControlCode == COMM_TYPE_TEST)
        function = 0;
    else if (IoControlCode == COMM_TYPE_QUERY_IPT_CAPABILITIES)
        function = 1;
    else if (IoControlCode == COMM_TYPE_SETUP_IPT)
        function = 2;
    else if (IoControlCode == COMM_TYPE_GET_BUFFER)
        function = 3;
    else if (IoControlCode == COMM_TYPE_FREE_BUFFER)
        function = 4;
    else
    {
        status = STATUS_NOT_IMPLEMENTED;
        goto cleanup;
    }

    status = gDriverData.IoCallbacks[function](
        inBufferSize,
        outBufferSize,
        inBuffer,
        outBuffer,
        &bytesWritten
        );
    if (!NT_SUCCESS(status))
    {
        // Todo: log error
        goto cleanup;
    }

cleanup:
    WdfRequestCompleteWithInformation(Request, status, bytesWritten);
    return;
}

NTSTATUS
PtwCommunicationInit(
    _In_ WDFDEVICE                  ControlDevice,
    _In_ IO_QUEUE_SETTINGS*         IoQueueSettings,
    _Inout_ WDFQUEUE*               Queue
)
{
    NTSTATUS                        status;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };

    DEBUG_PRINT("InitCommQueue\n");

    if (IoQueueSettings->DefaultQueue && !gDefaultQueueInitialized)
    {
        gDefaultQueueInitialized = TRUE;
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    }
    else if (IoQueueSettings->DefaultQueue && gDefaultQueueInitialized)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] Only one default queue alowed\n"));
        return STATUS_ALREADY_INITIALIZED;
    }
    else
    {
        WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
    }

    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests = ((ULONG)-1);

    ioQueueConfig.EvtIoDefault = IoQueueSettings->Callbacks.IoQueueIoDefault;
    ioQueueConfig.EvtIoRead = IoQueueSettings->Callbacks.IoQueueIoRead;
    ioQueueConfig.EvtIoWrite = IoQueueSettings->Callbacks.IoQueueIoWrite;
    ioQueueConfig.EvtIoStop = IoQueueSettings->Callbacks.IoQueueIoStop;
    ioQueueConfig.EvtIoResume = IoQueueSettings->Callbacks.IoQueueIoResume;
    ioQueueConfig.EvtIoCanceledOnQueue = IoQueueSettings->Callbacks.IoQueueIoCanceledOnQueue;

    ioQueueConfig.EvtIoDeviceControl = IoQueueSettings->Callbacks.IoQueueIoDeviceControl;
    ioQueueConfig.EvtIoInternalDeviceControl = IoQueueSettings->Callbacks.IoQueueIoInternalDeviceControl;

    ioQueueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(ControlDevice, &ioQueueConfig, NULL, Queue);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
        return status;
    }

    return status;

}

NTSTATUS
PtwCommunicationUninit(
    _In_ WDFQUEUE Queue
)
{
    WdfIoQueueStopAndPurge(
        Queue,
        NULL,
        NULL
    );

    return STATUS_SUCCESS;
}