#include "CommunicationCallbacks.h"
#include "Public.h"
#include "Debug.h"
#include "ProcessorTrace.h"

NTSTATUS 
CommGetRequestBuffers(
    _In_ WDFREQUEST Request,
    _In_ size_t InBufferSize,
    _In_ size_t OutBufferSize,
    _Out_ PVOID *InBuffer,
    _Out_ size_t *InBufferActualSize,
    _Out_ PVOID *OutBuffer,
    _Out_ size_t *OutBufferActualSize
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
            DEBUG_PRINT("Failed retrieveing input buffer\n");
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
            DEBUG_PRINT("Failed retrieveing output buffer\n");
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

void 
CommIoControlCallback(
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

    DEBUG_STOP();
    UNREFERENCED_PARAMETER(Queue);

    if (IoControlCode >= COMM_TYPE_MAX)
    {
        status = STATUS_INVALID_MESSAGE;
        goto cleanup;
    }
    
    status = CommGetRequestBuffers(
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

    status = gDriverData.IoCallbacks[IoControlCode](
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
CommandTest
(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    // Validate Input and Output Size
    if (InputBufferLength != sizeof(COMM_DATA_TEST))
    {
        return STATUS_INVALID_PARAMETER_1;
    }
    if (OutputBufferLength != sizeof(COMM_DATA_TEST))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    NTSTATUS status = STATUS_SUCCESS;
    COMM_DATA_TEST* inputData = (COMM_DATA_TEST*)InputBuffer;
    COMM_DATA_TEST* outputData = (COMM_DATA_TEST*)OutputBuffer;

    if (inputData->Magic == UM_TEST_MAGIC)
    {
        DEBUG_PRINT("Input value is OK. Writing output value...\n");
        outputData->Magic = KM_TEST_MAGIC;
    }
    else
    {
        DEBUG_PRINT("Input value is NOT OK. Aborting...\n");
        status = STATUS_UNSUCCESSFUL;
    }

    *BytesWritten = sizeof(COMM_DATA_TEST);
    return status;
}

NTSTATUS
CommandGetPtCapabilities
(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);

    // Validate Input and Output Size
    if (OutputBufferLength != sizeof(INTEL_PT_CAPABILITIES))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    NTSTATUS status = STATUS_SUCCESS;
    INTEL_PT_CAPABILITIES* outputData = (INTEL_PT_CAPABILITIES*)OutputBuffer;

    status = PTGetCapabilities(outputData);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("PTGetCapabilities returned status %X\n", status);
        *BytesWritten = 0;
    }
    else 
    {
        *BytesWritten = sizeof(INTEL_PT_CAPABILITIES);
    }

    return status;
}

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}