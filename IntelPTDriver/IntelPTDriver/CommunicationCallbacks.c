#include "CommunicationCallbacks.h"
#include "Public.h"

NTSTATUS
CommHandleRetrieveInputOutputBuffers(
    WDFREQUEST Request,
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID *InputBuffer,
    PVOID *OutputBuffer
)
{
    size_t bufferSize = 0;

    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        InputBufferLength,
        InputBuffer,
        &bufferSize
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(
        Request,
        OutputBufferLength,
        OutputBuffer,
        &bufferSize
    );
    if (!NT_SUCCESS(status))
    {
        *OutputBuffer = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CommHandleIoRequest(
    ULONG       RequestType,
    WDFREQUEST  Request,
    size_t      OutputBufferLength,
    size_t      InputBufferLength
)
{
    NTSTATUS status;

    PVOID  inputBuffer = NULL;
    PVOID  outputBuffer = NULL;

    status = CommHandleRetrieveInputOutputBuffers(
        Request,
        InputBufferLength,
        OutputBufferLength,
        &inputBuffer,
        &outputBuffer
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return gDriverData.IoCallbacks[RequestType](
        InputBufferLength,
        OutputBufferLength,
        &inputBuffer,
        &outputBuffer
    );
}


void 
CommIoControlCallback(
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

    __debugbreak();

    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(WdfIoQueueGetDevice(Queue));
    if (!ctx)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, bytesWritten);
        return;
    }


    switch (IoControlCode)
    {
    case COMM_INITIALIZE_COMMUNICATION:
    {
        status = CommHandleIoRequest(
            IoControlCode,
            Request,
            OutputBufferLength,
            InputBufferLength
        );
    }
    break;
    case COMM_STOP_COMMUNICATION:
    {
        status = CommHandleIoRequest(
            IoControlCode,
            Request,
            OutputBufferLength,
            InputBufferLength
        );
    }
    break;
    case COMM_UPDATE_COMMUNICATION:
    {
        status = CommHandleIoRequest(
            IoControlCode,
            Request,
            OutputBufferLength,
            InputBufferLength
        );
    }
    break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        bytesWritten = 0;
        break;
    }

    if (completeRequest)
    {
        WdfRequestCompleteWithInformation(Request, status, bytesWritten);
    }

    return;
}

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}