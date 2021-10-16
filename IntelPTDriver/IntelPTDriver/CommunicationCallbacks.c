#include "CommunicationCallbacks.h"
#include "Public.h"
#include "Debug.h"
#include "ProcessorTrace.h"
#include "DriverUtils.h"

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

    UNREFERENCED_PARAMETER(Queue);

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


WCHAR* ExecutableName = L"TracedApp.exe";
HANDLE gProcessId = 0;

void
InterceptThreadCreate(
    _In_ HANDLE ParentId,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN Create
)
{
    UNREFERENCED_PARAMETER(ParentId);
    NTSTATUS status;

    if (gProcessId != ProcessId && ParentId != gProcessId)
        return;

    if (!Create)
        return; // Destul de rau

    DEBUG_STOP();


    unsigned long long  cr3 = __readcr3() & (~(PAGE_SIZE - 1));
    DEBUG_PRINT("Thread CR3 %X\n", cr3);

    INTEL_PT_CONFIGURATION filterConfiguration = {
        .FilteringOptions = {
            .FilterCpl = {
                .FilterKm = FALSE,
                .FilterUm = TRUE
            },
            .FilterCr3 = {
                .Enable = TRUE,
                .Cr3Address = (PVOID)cr3
            },
            .FilterRange = {
                .Enable = FALSE,
                .NumberOfRanges = 0
            }
        },
        .PacketGenerationOptions = {
            .Misc = {0},
            .PacketCofi.Enable = TRUE,
            .PacketCyc = {0},
            .PacketPtw = {0},
            .PacketPwr = {0},
            .PacketTsc = {0},
            .PackteMtc = {0}
        },
        .OutputOptions = {
            .TopaEntries = 70,
            .OutputType = PtOutputTypeToPA
        }
    };

    status = PtSetup(
        &filterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    PtEnableTrace();
    DEBUG_PRINT("STARTED TRACING\n");

    return;
    
}

void
InterceptProcessExit(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    NTSTATUS status;

    if (CreateInfo)
        return;

    if (!gProcessId)
        return;

    if (ProcessId != gProcessId)
        return;

    DEBUG_STOP();

    status = PsRemoveCreateThreadNotifyRoutine(
        InterceptThreadCreate
    );

    PtDisableTrace();
    gProcessId = 0;


    return;
}

void LoadImageNotifyRoutine(
    PUNICODE_STRING FullImageName,
    HANDLE ProcessId,
    PIMAGE_INFO ImageInfo
)
{
    UNREFERENCED_PARAMETER(ImageInfo);

    NTSTATUS status;
    wchar_t* found = wcsstr(
        FullImageName->Buffer,
        ExecutableName
    );

    if (found == NULL)
    {
        goto cleanup;
    }

    DEBUG_STOP();
    gProcessId = ProcessId;

    status = PsSetCreateThreadNotifyRoutineEx(
        PsCreateThreadNotifyNonSystem,
        (PVOID)InterceptThreadCreate
    );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    status = PsSetCreateProcessNotifyRoutineEx(
        InterceptProcessExit,
        FALSE
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("Failed to hook process creation. Status %X\n", status);
        return;
    }

cleanup:
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

    status = PtGetCapabilities(outputData);
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

NTSTATUS
CommandTestIptSetup
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
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (OutputBufferLength != sizeof(COMM_DATA_SETUP_IPT))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBuffer);

    //COMM_DATA_SETUP_IPT* data = (COMM_DATA_SETUP_IPT*)OutputBuffer;
    //PVOID bufferVaKm;
    //PVOID bufferPa;
    //PVOID bufferVaUm;
    //IA32_RTIT_STATUS_STRUCTURE statusPt;
    NTSTATUS status;
    //PMDL mdl;

    //DEBUG_STOP();

    status = DuQueueInit(
        &gQueueHead,
        TRUE
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuQueueInit returned status %X\n", status);
    }

    status = PsSetLoadImageNotifyRoutine(
        LoadImageNotifyRoutine
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("Failed to hook process creation. Status %X\n", status);
        return status;
    }


    //if (!NT_SUCCESS(status))
    //{
    //    return status;
    //}

    //PtDisableTrace();
    //if (!NT_SUCCESS(status))
    //{
    //    return status;
    //}

    return status;
}

#define PT_COMM_MEM_TAG 'PCMT'

typedef struct _USER_MAPPING_INTERNAL_STRUCTURE {

    PVOID BaseKAddr;
    PVOID BaseUAddr;
    PMDL  Mdl;

} USER_MAPPING_INTERNAL_STRUCTURE;

#define MAX_USER_MAPPINGS 10000
USER_MAPPING_INTERNAL_STRUCTURE* gUserBufferMappings[MAX_USER_MAPPINGS] = {0};
unsigned long long gCurrentId = 0;
BOOLEAN gFirstPageToUser = TRUE;
BOOLEAN gFirstPageToFree = TRUE;

NTSTATUS
CommandSendBuffers(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(InputBuffer);

    if (OutputBufferLength != sizeof(COMM_BUFFER_ADDRESS))
    {
        *OutputBuffer = NULL;
        *BytesWritten = 0;
        return STATUS_INVALID_PARAMETER_2;
    }

    NTSTATUS status;
    PVOID data;
    PMDL mdl;
    PVOID userAddress;
    unsigned long long crtIdx;    
    
    status = DuDequeueElement(
        gQueueHead,
        &data
    );
    
    while (status == STATUS_NO_MORE_ENTRIES)
    {
        KeResetEvent(&gPagesAvailableEvent);
        KIRQL irql = KeGetCurrentIrql();
        DEBUG_PRINT("irql %d\n", irql);
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
            &data
        );
    }
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuDequeueElement returned status %X\n", status);
        *OutputBuffer = NULL;
        *BytesWritten = 0;
        return status;
    }

    if (gFirstPageToUser)
    {
        DEBUG_PRINT("PAGE TO USER %p\n", data);
        gFirstPageToUser = FALSE;
    }

    status = DuMapBufferInUserspace(
        data,
        PAGE_SIZE,
        &mdl,
        &userAddress
    );
    if (!NT_SUCCESS(status))
    {
        *OutputBuffer = NULL;
        *BytesWritten = 0;
        return status;
    }

    USER_MAPPING_INTERNAL_STRUCTURE* mapping = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(USER_MAPPING_INTERNAL_STRUCTURE),
        PT_COMM_MEM_TAG
    );
    if (!mapping)
    {
        DEBUG_PRINT("ExAllocatePoolWithTag failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mapping->BaseKAddr = data;
    mapping->BaseUAddr = userAddress;
    mapping->Mdl = mdl;

    status = KeWaitForSingleObject(
        &gCommMutex,
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
    crtIdx = gCurrentId;
    while (gUserBufferMappings[crtIdx])
    {
        crtIdx = (crtIdx + 1) % MAX_USER_MAPPINGS;
        if (crtIdx == gCurrentId)
        {
            *OutputBuffer = NULL;
            *BytesWritten = 0;
            return STATUS_NO_MORE_ENTRIES;
        }
    }
    gUserBufferMappings[crtIdx] = mapping;

    KeReleaseMutex(
        &gCommMutex,
        FALSE
    );

    ((COMM_BUFFER_ADDRESS*)OutputBuffer)->BufferAddress = userAddress;
    ((COMM_BUFFER_ADDRESS*)OutputBuffer)->PageId = crtIdx;       // TODO: Generate page id, transform array into linked list
    *BytesWritten = sizeof(COMM_BUFFER_ADDRESS);

    return status;

}

NTSTATUS
CommandFreeBuffer(
    size_t InputBufferLength,
    size_t OutputBufferLength,
    PVOID* InputBuffer,
    PVOID* OutputBuffer,
    UINT32* BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(BytesWritten);

    if (InputBufferLength != sizeof(unsigned long long))
        return STATUS_INVALID_PARAMETER_1;

    unsigned long long index = *(unsigned long long*)InputBuffer;

    USER_MAPPING_INTERNAL_STRUCTURE *element = gUserBufferMappings[index];
    gUserBufferMappings[index] = NULL;

    if (gFirstPageToUser)
    {
        DEBUG_PRINT("PAGE TO FREE %p\n", element->BaseKAddr);
        gFirstPageToUser = FALSE;
    }

    DuFreeUserspaceMapping(
        element->BaseUAddr,
        element->Mdl
    );

    DuFreeBuffer(element->BaseKAddr);
    
    return STATUS_NOT_IMPLEMENTED;
}

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}