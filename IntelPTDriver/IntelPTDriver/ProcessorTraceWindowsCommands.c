#include "ProcessorTraceWindowsCommands.h"
#include "ProcessorTrace.h"
#include "ProcessorTraceWindowsControl.h"

NTSTATUS
PtwCommandTest(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    return STATUS_SUCCESS;
}

NTSTATUS
PtwCommandQueryCapabilities(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);

    NTSTATUS status;
    DEBUG_PRINT("[INFO] Query driver command...\n");

    if (OutputBufferLength != sizeof(INTEL_PT_CAPABILITIES))
        return STATUS_INVALID_PARAMETER_2;

    status = IptGetCapabilities((INTEL_PT_CAPABILITIES *)OutputBuffer);

    return status;
}

NTSTATUS
PtwCommandSetupIpt(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    return STATUS_SUCCESS;
}

NTSTATUS
PtwCommandTraceProcess(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    NTSTATUS status;
    status = PtwHookProcessCr3();

    return status;
}

NTSTATUS
PtwCommandTraceSSDT(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    NTSTATUS status;

    status = PtwHookSSDT();

    return status;
}

NTSTATUS
PtwCommandGetBuffer(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (OutputBufferLength != sizeof(COMM_BUFFER_ADDRESS))
    {
        *OutputBuffer = NULL;
        *BytesWritten = 0;
        return STATUS_INVALID_PARAMETER_2;
    }

    //PVOID address;
    COMM_BUFFER_ADDRESS* dto;
    NTSTATUS status;
    PMDL mdl;

    status = DuDequeueElement(
        gQueueHead,
        (PVOID)&dto
    );


    while (status == STATUS_NO_MORE_ENTRIES)
    {
        DEBUG_PRINT("[WARNING] No More Entries. Waitning for new daata...\n");
        KeResetEvent(&gPagesAvailableEvent);
        DEBUG_PRINT("[WARNING] Data available, proceeding...\n");

        status = KeWaitForSingleObject(
            &gPagesAvailableEvent,
            Executive,
            KernelMode,
            TRUE,
            NULL
        );
        if (!NT_SUCCESS(status))
        {
            *OutputBuffer = NULL;
            *BytesWritten = 0;
            return status;
        }

        status = DuDequeueElement(
            gQueueHead,
            (PVOID)&dto
        );
    }
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuDequeueElement returned status %X\n", status);
        *OutputBuffer = NULL;
        *BytesWritten = 0;
        return status;
    }

    memcpy(((COMM_BUFFER_ADDRESS*)OutputBuffer), dto, sizeof(COMM_BUFFER_ADDRESS));

    if (!dto->Header.Options.FirstPacket)
    {
        //mdl = dto->Payload.GenericPacket.BufferAddress;
        mdl = NULL;
        //address = MmMapLockedPagesSpecifyCache(
        //    mdl,
        //    UserMode,
        //    MmCached,
        //    NULL,
        //    FALSE,
        //    NormalPagePriority
        //);
        //if (!NT_SUCCESS(status))
        //{
        //    *OutputBuffer = NULL;
        //    *BytesWritten = 0;
        //    return status;
        //}

        ((COMM_BUFFER_ADDRESS*)OutputBuffer)->Payload.GenericPacket.BufferAddress = NULL;
        ((COMM_BUFFER_ADDRESS*)OutputBuffer)->FreeBuffer.MdlFree = mdl;
        ((COMM_BUFFER_ADDRESS*)OutputBuffer)->FreeBuffer.BaseAddressFree = NULL;
    }

    *BytesWritten = sizeof(COMM_BUFFER_ADDRESS);

    ExFreePoolWithTag(
        (PVOID)dto,
        'ffuB'
    );

    return status;
}

NTSTATUS
PtwCommandFreeBuffer(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //if (InputBufferLength != sizeof(COMM_FREE_BUFFER))
    //{
    //    DbgBreakPoint();
    //}

    //COMM_FREE_BUFFER* freeBuffer = (COMM_FREE_BUFFER*)InputBuffer;

    //MmUnmapLockedPages(
    //    freeBuffer->BaseAddressFree,
    //    freeBuffer->MdlFree
    //);

    //MmFreePagesFromMdl(
    //    freeBuffer->MdlFree
    //);

    return STATUS_SUCCESS;
}

NTSTATUS
PtwCommandDisable(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    return STATUS_SUCCESS;
}
