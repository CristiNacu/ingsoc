#include "ProcessorTraceWindowsControl.h"
#include "ProcessorTraceShared.h"
#include "ProcessorTrace.h"
//#include "IntelProcessorTraceDefs.h"

#define PUBLIC
#define PRIVATE
#define PTW_POOL_TAG                         'PTWP'

KIPI_BROADCAST_WORKER PtwIpiPerCoreInit;
KIPI_BROADCAST_WORKER PtwIpiPerCoreSetup;
KIPI_BROADCAST_WORKER PtwIpcPerCoreDisable;

void IptPageAllocation(
    unsigned Size,
    unsigned Alignment,
    void* VirtualAddress,
    void* PhysicalAddress
);

void
PtwHookImageLoad(
    PUNICODE_STRING FullImageName,
    HANDLE ProcessId,
    PIMAGE_INFO ImageInfo
);

typedef VOID(*PMIHANDLER)(PKTRAP_FRAME TrapFrame);
BOOLEAN gFirstPage = TRUE;
WCHAR* ExecutableName = L"TracedApp.exe";
HANDLE gProcessId = 0;
volatile long gExecutedCoreCount = 0;

PRIVATE
NTSTATUS
PtwExecuteAndWaitPerCore(
    KIPI_BROADCAST_WORKER PtwBroadcastRoutine,
    PVOID Context
)
{
    //PKDPC pProcDpc;
    LONG numberOfCores = (LONG)KeQueryActiveProcessorCount(NULL);

    // TODO: Is sending an IPI a good implementation????
    KeIpiGenericCall(PtwBroadcastRoutine, (ULONG_PTR)Context);

    // TODO: Shoud i force an interrupt on all cores to process the DPC queues?

    while (gExecutedCoreCount != numberOfCores);

    gExecutedCoreCount = 0;

    return STATUS_SUCCESS;
}

PUBLIC
NTSTATUS 
PtwInit()
{
    NTSTATUS status;
    status = IptInit(
        IptPageAllocation
    );

    return PtwExecuteAndWaitPerCore(PtwIpiPerCoreInit, NULL);
}

PUBLIC
void 
PtwUninit()
{
    IptUninit();
}

PRIVATE
NTSTATUS 
PtwSetup(
    INTEL_PT_CONFIGURATION *Config
)
{
    return PtwExecuteAndWaitPerCore(PtwIpiPerCoreSetup, Config);
}

PUBLIC
NTSTATUS 
PtwDisable()
{
    return PtwExecuteAndWaitPerCore(PtwIpcPerCoreDisable, NULL);
}

PUBLIC
NTSTATUS 
PtwHookProcess()
{
    NTSTATUS status;

    status = PsSetLoadImageNotifyRoutine(
        PtwHookImageLoad
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

    if (CreateInfo)
        return;

    if (!gProcessId)
        return;

    if (ProcessId != gProcessId)
        return;

    DEBUG_STOP();

    status = PsRemoveCreateThreadNotifyRoutine(
        PtwHookThreadCreation
    );

    PtwDisable();
    gProcessId = 0;

    return;
}

PRIVATE
void
PtwHookImageLoad(
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
    //unsigned WrittenAddresses = 0;
    //PVOID* oldVaAddresses;

    status = IptUnlinkFullTopaBuffers();
    IptResetPmi();


    //oldVaAddresses = ExAllocatePoolWithTag(
    //        NonPagedPool, 
    //        sizeof(PVOID) * (gFrequency + 1), 
    //        PT_POOL_TAG
    //    );
    //if (!oldVaAddresses)
    //{
    //    DEBUG_PRINT("could not allocate oldVaAddresses buffer\n");
    //    return;
    //}

    //status = IptUnlinkFullTopaBuffers(
    //    gTopa,
    //    oldVaAddresses,
    //    &WrittenAddresses
    //);
    //if (!NT_SUCCESS(status))
    //{
    //    DEBUG_PRINT("IptUnlinkFullTopaBuffers error %X\n", status);
    //    return;
    //}

    //if (gFirstPage)
    //{
    //    gFirstPage = FALSE;
    //    DEBUG_PRINT("FIRST PAGE BUFFER VA %p\n", oldVaAddresses[0]);
    //}

    //status = DuEnqueueElements(
    //    gQueueHead,
    //    WrittenAddresses,
    //    oldVaAddresses
    //);
    //if (!NT_SUCCESS(status))
    //{
    //    DEBUG_PRINT("DuEnqueueElements error %X\n", status);
    //    return;
    //}

    //KeSetEvent(&gPagesAvailableEvent, 0, FALSE);
    //ExFreePoolWithTag(oldVaAddresses, PT_POOL_TAG);

    DEBUG_STOP();
    IptResetPmi();
    IptResumeTrace();
}

PRIVATE
VOID 
IptPmiHandler(
    PKTRAP_FRAME pTrapFrame
)
{
    UNREFERENCED_PARAMETER(pTrapFrame);
    PKDPC pProcDpc;

    IptPauseTrace();

    if (!IptTopaPmiReason())
    {
        IptResumeTrace();
        return;
    }


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

    return;
}


// Ipi routines:
PRIVATE
ULONG_PTR
PtwIpiPerCoreInit(
    _In_ ULONG_PTR Argument
)
{
    UNREFERENCED_PARAMETER(Argument);
    
    INTEL_PT_CONTROL_STRUCTURE IptControlStructure;
    NTSTATUS status;
    
    status = IptInitPerCore(
        &IptControlStructure
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("IptInitPerCore %X\n", status);
    }

    return 0;
}

PRIVATE
void IptPageAllocation(
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
ULONG_PTR
PtwIpiPerCoreSetup(
    _In_ ULONG_PTR Argument
)
{
    INTEL_PT_CONFIGURATION* config = (INTEL_PT_CONFIGURATION*)Argument;
    //INTEL_PT_CONTROL_STRUCTURE controlStructure;
    NTSTATUS status;

    ULONG currentProcessorNumber = KeGetCurrentProcessorNumberEx(NULL);

    status = IptSetup(
        config,
        &gDriverData.IptPerCoreControl[currentProcessorNumber]
    );
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("IptSetup returned status %X\n", status);
    }

    IptEnableTrace();

    InterlockedIncrement(&gExecutedCoreCount);
    return 0;
}

PRIVATE
ULONG_PTR
PtwIpcPerCoreDisable(
    _In_ ULONG_PTR Argument
)
{
    UNREFERENCED_PARAMETER(Argument);
    IptDisableTrace();
    InterlockedIncrement(&gExecutedCoreCount);
    return 0;
}
