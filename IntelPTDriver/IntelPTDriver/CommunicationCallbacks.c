#include "CommunicationCallbacks.h"
#include "Public.h"
#include "Debug.h"
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

    DEBUG_STOP();
    UNREFERENCED_PARAMETER(Queue);

    switch (IoControlCode)
    {
    case COMM_TEST:
    {
        DEBUG_STOP();

        COMM_PROT_TEST *testOutBuffer = NULL;
        COMM_PROT_TEST *testInBuffer = NULL;
        size_t inBufferSize;
        size_t outBufferSize;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(COMM_PROT_TEST),
            &testInBuffer,
            &inBufferSize
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("Failed retrieveing input buffer\n");
            goto cleanup;
        }

        status = WdfRequestRetrieveOutputBuffer(
            Request,
            sizeof(COMM_PROT_TEST),
            &testOutBuffer,
            &outBufferSize
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("Failed retrieveing output buffer\n");
            goto cleanup;
        }

        if (testInBuffer->Magic == UM_TEST_MAGIC)
        {
            DEBUG_PRINT("Input value is OK. Writing output value...\n");
            testOutBuffer->Magic = KM_TEST_MAGIC;
        }
        else
        {
            DEBUG_PRINT("Input value is NOT OK. Aborting...\n");
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
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

cleanup:
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