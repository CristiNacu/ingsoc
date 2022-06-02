#include "ProcessorTraceWindowsControl.h"
#include "ProcessorTraceShared.h"
#include "ProcessorTrace.h"
//#include "IntelProcessorTraceDefs.h"

#define PUBLIC
#define PRIVATE
#define PTW_POOL_TAG                         'PTWP'

typedef 
void (GENERIC_PER_CORE_SYNC_ROUTINE)(
    PVOID _In_ Context
);


typedef enum _PER_CORE_SYNC_EXECUTION_LEVEL {
    IptPerCoreExecutionLevelIpi,
    IptPerCoreExecutionLevelDpc
} PER_CORE_SYNC_EXECUTION_LEVEL;

typedef struct _PER_CORE_SYNC_STRUCTURE {
    volatile SHORT IpiCounter;
    volatile SHORT DpcCounter;
    PER_CORE_SYNC_EXECUTION_LEVEL ExecutionLevel;
    PVOID Context;
    GENERIC_PER_CORE_SYNC_ROUTINE *Function;
} PER_CORE_SYNC_STRUCTURE;

GENERIC_PER_CORE_SYNC_ROUTINE PtwIpiPerCoreInit;
GENERIC_PER_CORE_SYNC_ROUTINE PtwIpiPerCoreSetup;
GENERIC_PER_CORE_SYNC_ROUTINE PtwDpcPerCoreDisable;

PRIVATE
VOID
IptPmiHandler(
    PKTRAP_FRAME pTrapFrame
);

PRIVATE
VOID
IptGenericDpcFunction(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    PER_CORE_SYNC_STRUCTURE* syncStructure = (PER_CORE_SYNC_STRUCTURE*)DeferredContext;

    ULONG procNumber = KeGetCurrentProcessorNumber();
    DEBUG_PRINT("Executig method on CPU %d LEVEL DPC\n", procNumber);

    if (syncStructure->ExecutionLevel == IptPerCoreExecutionLevelDpc)
        syncStructure->Function(syncStructure->Context);

    InterlockedIncrement16(&syncStructure->DpcCounter);

}

void 
IptPageAllocation(
    unsigned Size,
    unsigned Alignment,
    void* VirtualAddress,
    void* PhysicalAddress
);

void
PtwHookImageLoadCr3(
    PUNICODE_STRING FullImageName,
    HANDLE ProcessId,
    PIMAGE_INFO ImageInfo
);

void
PtwHookImageLoadCodeBase(
    PUNICODE_STRING FullImageName,
    HANDLE ProcessId,
    PIMAGE_INFO ImageInfo
);

typedef VOID(*PMIHANDLER)(PKTRAP_FRAME TrapFrame);
BOOLEAN gFirstPage = TRUE;
WCHAR* ExecutableName = L"TracedApp.exe";
HANDLE gProcessId = 0;
BOOLEAN gThreadHoodked = FALSE;
INTEL_PT_CONTROL_STRUCTURE* gIptPerCoreControl;


ULONG_PTR
PerCoreDispatcherRoutine(
    _In_ ULONG_PTR Argument
)
{
    PER_CORE_SYNC_STRUCTURE* syncStructure = (PER_CORE_SYNC_STRUCTURE*)Argument;
   
    ULONG procNumber = KeGetCurrentProcessorNumber();
    DEBUG_PRINT("Executig method on CPU %d LEVEL IPI\n", procNumber);

    if(syncStructure->ExecutionLevel == IptPerCoreExecutionLevelIpi)
        syncStructure->Function(syncStructure->Context);
    
    InterlockedIncrement16(&syncStructure->IpiCounter);
    return 0;
}

PRIVATE
NTSTATUS
PtwExecuteAndWaitPerCore(
    GENERIC_PER_CORE_SYNC_ROUTINE* PtwPerCoreRoutine,
    PER_CORE_SYNC_EXECUTION_LEVEL ExecutionLevel,
    PVOID Context
)
{
    PKDPC pProcDpc;
    LONG numberOfCores = (LONG)KeQueryActiveProcessorCount(NULL);

    PER_CORE_SYNC_STRUCTURE syncStructure = {
        .IpiCounter = 0,
        .DpcCounter = 0,
        .ExecutionLevel = ExecutionLevel,
        .Context = Context,
        .Function = PtwPerCoreRoutine
    };

    if (ExecutionLevel == IptPerCoreExecutionLevelDpc)
    {
        for (LONG i = 0; i < numberOfCores; i++)
        {
            pProcDpc = (PKDPC)ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(KDPC),
                PTW_POOL_TAG
            );
            if (pProcDpc == NULL)
            {
                DEBUG_PRINT("pProcDpc was allocated NULL. Exitting\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeDpc(
                pProcDpc,
                IptGenericDpcFunction,
                &syncStructure
            );

            // Set DPC for all processors
            KeSetTargetProcessorDpc(
                pProcDpc,
                (CCHAR)i
            );

            BOOLEAN status = KeInsertQueueDpc(
                pProcDpc,
                NULL,
                NULL
            );
            if (!status)
            {
                return STATUS_ACCESS_DENIED;
            }

            for (unsigned kk = 0; kk < 0xffff; kk++);
        }
    }
    // TODO: Is sending an IPI a good implementation????
    KeIpiGenericCall(PerCoreDispatcherRoutine, (ULONG_PTR)&syncStructure);

    // TODO: Shoud i force an interrupt on all cores to process the DPC queues?
    if (ExecutionLevel == IptPerCoreExecutionLevelDpc)
    {
        while (syncStructure.DpcCounter != numberOfCores);
        DEBUG_PRINT("Execution DPC finished successfully\n");
    }
    else
        while (syncStructure.IpiCounter != numberOfCores);

    return STATUS_SUCCESS;
}

PUBLIC
NTSTATUS 
PtwInit()
{
    NTSTATUS status;
    
    
    gIptPerCoreControl = ExAllocatePoolWithTag(
        NonPagedPool, 
        ((unsigned long)KeQueryActiveProcessorCount(NULL)) * sizeof(INTEL_PT_CONTROL_STRUCTURE), 
        PTW_POOL_TAG
    );
    if (!gIptPerCoreControl)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = IptInit(
        IptPageAllocation
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PtwExecuteAndWaitPerCore(PtwIpiPerCoreInit, IptPerCoreExecutionLevelDpc, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = DuQueueInit(
        &gQueueHead,
        TRUE
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_STOP();
        return status;
    }


    return status;
}

PUBLIC
void 
PtwUninit()
{
    IptUninit();
}

typedef VOID(*PMIHANDLER)(PKTRAP_FRAME TrapFrame);

PRIVATE
NTSTATUS 
PtwSetup(
    INTEL_PT_CONFIGURATION *Config
)
{
    PMIHANDLER newPmiHandler;
    NTSTATUS status;

    DEBUG_PRINT(">>> BA %p EA %p\n",
        Config->FilteringOptions.FilterRange.RangeOptions[0].BaseAddress,
        Config->FilteringOptions.FilterRange.RangeOptions[0].EndAddress);
    
    newPmiHandler = IptPmiHandler;
    status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PVOID), (PVOID)&newPmiHandler);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("HalSetSystemInformation returned status %X\n", status);
    }
    else
    {
        DEBUG_PRINT("HalSetSystemInformation returned SUCCESS!!!!\n");
    }


    return PtwExecuteAndWaitPerCore(PtwIpiPerCoreSetup, IptPerCoreExecutionLevelDpc, Config);
}

PUBLIC
NTSTATUS 
PtwDisable()
{
    return PtwExecuteAndWaitPerCore(PtwDpcPerCoreDisable, IptPerCoreExecutionLevelDpc, NULL);
}

PUBLIC
NTSTATUS 
PtwHookProcessCr3()
{
    NTSTATUS status;

    status = PsSetLoadImageNotifyRoutine(
        PtwHookImageLoadCr3
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("PsSetLoadImageNotifyRoutine error %X\n", status);
    }

    return status;
}

PUBLIC
NTSTATUS
PtwHookProcessCodeBase()
{
    NTSTATUS status;

    status = PsSetLoadImageNotifyRoutine(
        PtwHookImageLoadCodeBase
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("PsSetLoadImageNotifyRoutine error %X\n", status);
    }

    return status;
}

PUBLIC
NTSTATUS 
PtwHookSSDT()
{
    return STATUS_SUCCESS;
}

PVOID gImageBasePaStart = 0;
PVOID gImageBasePaEnd = 0;

PRIVATE
void
PtwHookThreadCreation(
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

    if (gThreadHoodked)
        return;

    gThreadHoodked = TRUE;


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
                .Enable = gImageBasePaStart != 0 ? TRUE : FALSE,
                .NumberOfRanges = gImageBasePaStart != 0 ? 0 : 1,
                .RangeOptions[0] = {
                    .BaseAddress = gImageBasePaStart,
                    .EndAddress = gImageBasePaEnd,
                    .RangeType = gImageBasePaStart != 0 ? RangeFilterEn : RangeUnused
            }
        }
        },
        .PacketGenerationOptions = {
            .PacketCofi.Enable = TRUE,
            .PacketCyc = {0},
            .PacketPtw = {0},
            .PacketPwr = {0},
            .PacketTsc.Enable = TRUE,
            .PackteMtc = {0},
            .Misc = {
                .PsbFrequency = Freq4K,
                //.InjectPsbPmiOnEnable = TRUE,
                .DisableRetCompression = TRUE
            },
        }
    };

    status = PtwSetup(
        &filterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    DEBUG_PRINT("STARTED TRACING\n");

    return;

}

PRIVATE
void
PtwHookProcessExit(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    NTSTATUS status;
    PMIHANDLER oldHandler;

    if (CreateInfo)
        return;

    if (!gProcessId)
        return;

    if (ProcessId != gProcessId)
        return;

    PtwDisable();

    status = PsRemoveCreateThreadNotifyRoutine(
        PtwHookThreadCreation
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("PsRemoveCreateThreadNotifyRoutine returned status %X\n", status);
    }

    //status = PsSetCreateProcessNotifyRoutineEx(
    //    PtwHookProcessExit,
    //    TRUE
    //);
    //if (!NT_SUCCESS(status))
    //{
    //    DEBUG_STOP();
    //    DEBUG_PRINT("PsSetCreateProcessNotifyRoutineEx returned status %X\n", status);
    //}

    //DEBUG_STOP();
    /// TODO: place this code somewhere

    oldHandler = 0;
    status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PVOID), (PVOID)&oldHandler);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("HalSetSystemInformation returned status %X\n", status);
    }
    else
    {
        DEBUG_PRINT("HalSetSystemInformation returned SUCCESS!!!!\n");
    }


    gProcessId = 0;
    gThreadHoodked = FALSE;

    return;
}

PRIVATE
void
PtwHookImageLoadCr3(
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

    DuIncreaseSequenceId();
    gDriverData.PacketIdCounter = 0;

    gProcessId = ProcessId;
    PVOID imageBasePhysicalStartAddress = (PVOID)MmGetPhysicalAddress(ImageInfo->ImageBase).QuadPart;
    PVOID imageBasePhysicalEndAddress = (PVOID)((unsigned long long)imageBasePhysicalStartAddress + ImageInfo->ImageSize);

    DEBUG_PRINT(">>>>>>>>>>>>\nProcess Image Base VA %X\nProcess Image Base Start PA %X\nImage Base End PA %X\nProcess Image Size %d\n<<<<<<<<<<<\n",
        ImageInfo->ImageBase,
        imageBasePhysicalStartAddress,
        imageBasePhysicalEndAddress,
        ImageInfo->ImageSize);

    status = PsSetCreateThreadNotifyRoutineEx(
        PsCreateThreadNotifyNonSystem,
        (PVOID)PtwHookThreadCreation
    );
    if (!NT_SUCCESS(status)) 
    {
        return;
    }

    status = PsSetCreateProcessNotifyRoutineEx(
        PtwHookProcessExit,
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

PRIVATE
void
PtwHookImageLoadCodeBase(
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
        
    gProcessId = ProcessId;

    status = PsSetCreateThreadNotifyRoutineEx(
        PsCreateThreadNotifyNonSystem,
        (PVOID)PtwHookThreadCreation
    );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    DuIncreaseSequenceId();
    gDriverData.PacketIdCounter = 0;

    PVOID imageBasePhysicalStartAddress = (PVOID)MmGetPhysicalAddress(ImageInfo->ImageBase).QuadPart;
    PVOID imageBasePhysicalEndAddress = (PVOID)((unsigned long long)imageBasePhysicalStartAddress + ImageInfo->ImageSize);

    DEBUG_PRINT(">>>>>>>>>>>>\nProcess Image Base VA %p\nProcess Image Base Start PA %p\nImage Base End PA %p\nProcess Image Size %lld\n<<<<<<<<<<<\n",
        ImageInfo->ImageBase,
        imageBasePhysicalStartAddress,
        imageBasePhysicalEndAddress,
        ImageInfo->ImageSize);

    gImageBasePaStart = ImageInfo->ImageBase;
    gImageBasePaEnd = (PVOID)((char *)ImageInfo->ImageBase + ImageInfo->ImageSize);

    COMM_BUFFER_ADDRESS* dto =
        (COMM_BUFFER_ADDRESS*)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(COMM_BUFFER_ADDRESS),
            'ffuB'
        );
    if (!dto)
    {
        return;
    }

    dto->Header.HeaderSize = sizeof(PACKET_HEADER_INFORMATION);
    dto->Header.Options.FirstPacket = TRUE;
    dto->Header.Options.LastPacket = FALSE;
    dto->Header.CpuId = KeGetCurrentProcessorNumber();
    dto->Header.SequenceId = DuGetSequenceId();
    dto->Header.PacketId = DuGetPacketId();

    dto->Payload.FirstPacket.ImageBaseAddress = gImageBasePaStart;
    dto->Payload.FirstPacket.ProcessorFrequency = gDriverData.ProcessorFrequency;
    dto->Payload.FirstPacket.ImageSize = (unsigned long)ImageInfo->ImageSize;


    DuEnqueueElement(
        gQueueHead,
        (PVOID)dto
    );

    status = PsSetCreateThreadNotifyRoutineEx(
        PsCreateThreadNotifyNonSystem,
        (PVOID)PtwHookThreadCreation
    );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    status = PsSetCreateProcessNotifyRoutineEx(
        PtwHookProcessExit,
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

PRIVATE
VOID
IptDpc(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NTSTATUS status;
    PMDL mdl;
    ULONG bufferSize;

    DEBUG_PRINT(">>> DPC ON AP %d\n", KeGetCurrentProcessorNumber());

    status = IptUnlinkFullTopaBuffers(
        &mdl,
        &bufferSize,
        gIptPerCoreControl[KeGetCurrentProcessorNumber()].OutputOptions,
        TRUE
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("IptUnlinkFullTopaBuffers error %X\n", status);
        return;
    }

    COMM_BUFFER_ADDRESS* dto =
        (COMM_BUFFER_ADDRESS*)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(COMM_BUFFER_ADDRESS),
            'ffuB'
        );
    
    if (!dto)
    {
        return;
    }


    dto->Header.HeaderSize = sizeof(PACKET_HEADER_INFORMATION);
    dto->Header.SequenceId = DuGetSequenceId();
    dto->Header.PacketId = DuGetPacketId();
    dto->Header.Options.LastPacket = FALSE;
    dto->Header.Options.FirstPacket = FALSE;
    dto->Header.CpuId = KeGetCurrentProcessorNumber();

    dto->Payload.GenericPacket.BufferSize = bufferSize;
    dto->Payload.GenericPacket.BufferAddress = mdl;


    status = DuEnqueueElement(
        gQueueHead,
        (PVOID)dto
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuEnqueueElements error %X\n", status);
    }
    else
    {
        KeSetEvent(&gPagesAvailableEvent, 0, FALSE);
    }

    IptResetPmi();
    IptResumeTrace(
        gIptPerCoreControl[KeGetCurrentProcessorNumber()].OutputOptions
    );
}

PRIVATE
VOID 
IptPmiHandler(
    PKTRAP_FRAME pTrapFrame
)
{
    PKDPC pProcDpc;
    DEBUG_PRINT(">>> PMI ON AP %d\n", KeGetCurrentProcessorNumber());
    UNREFERENCED_PARAMETER(pTrapFrame);

    if (!IptTopaPmiReason())
    {
        IptResetPmi();
        return;
    }

    DEBUG_PRINT(">>> PAUSING TRACE ON AP %d\n", KeGetCurrentProcessorNumber());
    IptPauseTrace(gIptPerCoreControl[KeGetCurrentProcessorNumber()].OutputOptions);
    
    pProcDpc = (PKDPC)ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(KDPC),
        PTW_POOL_TAG
    );
    if (pProcDpc == NULL)
    {
        DEBUG_PRINT("pProcDpc was allocated NULL. Exitting\n");
        return;
    }

    KeInitializeDpc(
        pProcDpc,
        IptDpc,
        NULL
    );

    KeSetTargetProcessorDpc(
        pProcDpc,
        (CCHAR)KeGetCurrentProcessorNumber()
    );

    KeInsertQueueDpc(
        pProcDpc,
        (PVOID)KeGetCurrentProcessorNumber(),
        NULL
    );
}


// Ipi routines:
PRIVATE
void
PtwIpiPerCoreInit(
    _In_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);
    
    NTSTATUS status;
    ULONG procnumber = KeGetCurrentProcessorNumber();

    status = IptInitPerCore(
        &gIptPerCoreControl[procnumber]
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("IptInitPerCore %X\n", status);
    }

    return;
}

PRIVATE
void 
IptPageAllocation(
    unsigned Size,
    unsigned Alignment,
    void* VirtualAddress,
    void* PhysicalAddress
)
{
    UNREFERENCED_PARAMETER(Alignment);

    NTSTATUS status;
    status = DuAllocateBuffer(
        Size,
        MmCached,
        TRUE,
        VirtualAddress,
        PhysicalAddress
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuAllocateBuffer returned status %X\n", status);
    }
    return;
}

void
IptPageFree(
    void* VirtualAddress,
    void* PhysicalAddress
)
{
    UNREFERENCED_PARAMETER(PhysicalAddress);
    DuFreeBuffer(
        VirtualAddress
    );
}


PRIVATE
void
PtwIpiPerCoreSetup(
    _In_ PVOID Context
)
{
    INTEL_PT_CONFIGURATION* config = (INTEL_PT_CONFIGURATION*)Context;
    //INTEL_PT_CONTROL_STRUCTURE controlStructure;
    NTSTATUS status;


    ULONG currentProcessorNumber = KeGetCurrentProcessorNumber();

    status = IptSetup(
        config,
        &gIptPerCoreControl[currentProcessorNumber]
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_STOP();
        DEBUG_PRINT("IptSetup returned status %X\n", status);
    }

    IptEnableTrace(
        gIptPerCoreControl[currentProcessorNumber].OutputOptions
    );

    return;
}

PRIVATE
void
PtwDpcPerCoreDisable(
    _In_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);
    PMDL mdl;
    NTSTATUS status;
    ULONG bufferSize;
    IptDisableTrace(
        &mdl,
        &bufferSize,
        gIptPerCoreControl[KeGetCurrentProcessorNumber()].OutputOptions
    );

    COMM_BUFFER_ADDRESS* dto =
        (COMM_BUFFER_ADDRESS*)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(COMM_BUFFER_ADDRESS),
            'ffuB'
        );
    if (!dto)
    {
        return;
    }

    dto->Header.HeaderSize = sizeof(PACKET_HEADER_INFORMATION);
    dto->Header.Options.FirstPacket = FALSE;
    dto->Header.Options.LastPacket = TRUE;
    dto->Header.CpuId = KeGetCurrentProcessorNumber();
    dto->Header.PacketId = DuGetPacketId();
    dto->Header.SequenceId = DuGetSequenceId();

    dto->Payload.GenericPacket.BufferSize = bufferSize;
    dto->Payload.GenericPacket.BufferAddress = mdl;

    status = DuEnqueueElement(
        gQueueHead,
        (PVOID)dto
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("DuEnqueueElements error %X\n", status);
    }
    else
    {
        KeSetEvent(&gPagesAvailableEvent, 0, FALSE);
    }

    return;
}
