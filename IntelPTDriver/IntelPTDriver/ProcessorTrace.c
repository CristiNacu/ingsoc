#include "ProcessorTrace.h"
#include "ProcessorTraceControlStructure.h"
#include "DriverUtils.h"

typedef enum CPUID_INDEX {
    CpuidEax,
    CpuidEbx,
    CpuidEcx,
    CpuidEdx
} CPUID_INDEX;

TOPA_TABLE* gTopa;

#define CPUID_INTELPT_AVAILABLE_LEAF        0x7
#define CPUID_INTELPT_AVAILABLE_SUBLEAF     0x0

#define CPUID_INTELPT_CAPABILITY_LEAF       0x14

#define BIT(x)                              (1<<x)

#define INTEL_PT_BIT                        25
#define INTEL_PT_MASK                       BIT(INTEL_PT_BIT)
#define PT_FEATURE_AVAILABLE(v)             ((v) != 0)


#define IA32_RTIT_OUTPUT_BASE               0x560
#define IA32_RTIT_OUTPUT_MASK_PTRS          0x561
#define IA32_RTIT_CTL                       0x570
#define IA32_RTIT_CR3_MATCH                 0x572

#define IA32_RTIT_ADDR0_A                   0x580
#define IA32_RTIT_ADDR0_B                   0x581
#define IA32_RTIT_ADDR1_A                   0x582
#define IA32_RTIT_ADDR1_B                   0x583
#define IA32_RTIT_ADDR2_A                   0x584
#define IA32_RTIT_ADDR2_B                   0x585
#define IA32_RTIT_ADDR3_A                   0x586
#define IA32_RTIT_ADDR3_B                   0x587

#define IA32_APIC_BASE                      0x1B
#define IA32_LVT_REGISTER                   0x834

INTEL_PT_CAPABILITIES*  gPtCapabilities = NULL;
BOOLEAN                 gTraceEnabled = FALSE;
unsigned long long      gFrequency = 3;
unsigned long long      gBufferSize = PAGE_SIZE;

typedef union _PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE {

    struct {

        unsigned long Vector : 8;
        unsigned long DeliveryMode : 3;
        unsigned long Reserved0 : 1;
        unsigned long DeliveryStatus : 1;
        unsigned long Reserved1 : 3;
        unsigned long Mask : 1;
        unsigned long Raw : 15;

    } Values;

    unsigned long Raw;

} PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE;

typedef union _IA32_PERF_GLOBAL_STATUS_STRUCTURE {

    struct {
        unsigned long long OvfPMC0 : 1; // 0
        unsigned long long OvfPMC1 : 1; // 1
        unsigned long long OvfPMC2 : 1; // 2
        unsigned long long OvfPMC3 : 1; // 3
        unsigned long long Reserved0 : 28; // 31:4
        unsigned long long OvfFixedCtr0 : 1; // 32
        unsigned long long OvfFixedCtr1 : 1; // 33
        unsigned long long OvfFixedCtr2 : 1; // 34
        unsigned long long Reserved1 : 13; // 47:35
        unsigned long long OvfPerfMetrics : 1; // 48
        unsigned long long Reserved2 : 6; // 54:49
        unsigned long long TopaPMI : 1; // 55
        unsigned long long Reserved3 : 2; // 57:56
        unsigned long long LbrFrz : 1; // 58
        unsigned long long CrtFrz : 1; // 59
        unsigned long long Asci : 1; // 60
        unsigned long long OvfUncore : 1; // 61
        unsigned long long OvfBuf : 1; // 62
        unsigned long long CondChgd : 1; // 63
    } Values;
    unsigned long long Raw;

} IA32_PERF_GLOBAL_STATUS_STRUCTURE;

typedef union _IA32_APIC_BASE_STRUCTURE {

    struct {
        unsigned long long Reserved0 : 8;
        unsigned long long BSPFlag : 1;
        unsigned long long Reserved1 : 1;
        unsigned long long EnableX2ApicMode : 1;
        unsigned long long ApicGlobalEnbale : 1;
        unsigned long long ApicBase : 52;
    } Values;
    unsigned long long Raw;
} IA32_APIC_BASE_STRUCTURE;
    
#define PT_POOL_TAG                         'PTPT'
#define INTEL_PT_OUTPUT_TAG                 'IPTO'

#define PT_OPTION_IS_SUPPORTED(capability, option)  (((!capability) && (option)) ? FALSE : TRUE)

#define PT_OUTPUT_CONTIGNUOUS_BASE_MASK             0x7F
#define PT_OUTPUT_TOPA_BASE_MASK                    0xFFF





NTSTATUS
PtQueryCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
)
{
    int cpuidIntelpt[4] = { 0 };

    if (!Capabilities)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    __cpuidex(
        cpuidIntelpt,
        CPUID_INTELPT_AVAILABLE_LEAF,
        CPUID_INTELPT_AVAILABLE_SUBLEAF
    );

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CPUID(EAX:0x7 ECX:0x0).EBX = %X\n", cpuidIntelpt[CpuidEbx]));

    Capabilities->IntelPtAvailable = PT_FEATURE_AVAILABLE(cpuidIntelpt[CpuidEbx] & INTEL_PT_MASK);

    if (!Capabilities->IntelPtAvailable)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability NOT Available\n"));
        return STATUS_NOT_SUPPORTED;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability Available\n"));

    /// Gather future capabilities of Intel PT
    /// CPUID function 14H is dedicated to enumerate the resource and capability of processors that report CPUID.(EAX = 07H, ECX = 0H) : EBX[bit 25] = 1.

    __cpuidex(
        (int *)&Capabilities->IptCapabilities0,
        CPUID_INTELPT_CAPABILITY_LEAF,
        0x0
    );

    if (Capabilities->IptCapabilities0.Eax.MaximumValidSubleaf >= 1)
        __cpuidex(
            (int*)&Capabilities->IptCapabilities1,
            CPUID_INTELPT_CAPABILITY_LEAF,
            0x1
        );

    Capabilities->TopaOutputEntries = Capabilities->IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport ? 
        __readmsr(IA32_RTIT_OUTPUT_MASK_PTRS) : 0;


    return STATUS_SUCCESS;
}

NTSTATUS
PtGetCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
)
{
    NTSTATUS status = STATUS_SUCCESS;
    if (!gPtCapabilities)
    {
        status = PtInit();
        if (!NT_SUCCESS(status) || !gPtCapabilities)
        {
            return status;
        }
    }

    memcpy_s(
        Capabilities,
        sizeof(INTEL_PT_CAPABILITIES),
        gPtCapabilities,
        sizeof(INTEL_PT_CAPABILITIES)
    );

    return STATUS_SUCCESS;
}

NTSTATUS
PtEnableTrace(
)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    IA32_RTIT_STATUS_STRUCTURE ptStatus;

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Values.TraceEn = TRUE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);

    PtGetStatus(ptStatus);
    if (ptStatus.Values.Error || !ptStatus.Values.TriggerEn)
    {
        DEBUG_PRINT("Pt TraceEnable failed\n");
        DEBUG_STOP();
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        gTraceEnabled = TRUE;
        return STATUS_SUCCESS;
    }
}

NTSTATUS
PtDisableTrace(
)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    IA32_RTIT_STATUS_STRUCTURE ptStatus;

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Values.TraceEn = FALSE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);

    PtGetStatus(ptStatus);
    if (ptStatus.Values.Error || ptStatus.Values.TriggerEn)
    {
        DEBUG_PRINT("Pt TraceDisable failed\n");
        DEBUG_STOP();
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        gTraceEnabled = FALSE;
        return STATUS_SUCCESS;
    }
}

NTSTATUS
PtValidateConfigurationRequest(
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    INTEL_PT_CAPABILITIES capabilities;
    NTSTATUS status;
    status = PtGetCapabilities(
        &capabilities
    );
    if(!NT_SUCCESS(status))
        return status;

    if (!PT_OPTION_IS_SUPPORTED( capabilities.IptCapabilities0.Ebx.ConfigurablePsbAndCycleAccurateModeSupport,
        FilterConfiguration->PacketGenerationOptions.PacketCyc.Enable))
        return STATUS_NOT_SUPPORTED;

    if (!PT_OPTION_IS_SUPPORTED( capabilities.IptCapabilities0.Ebx.PtwriteSupport,
        FilterConfiguration->PacketGenerationOptions.Misc.EnableFupPacketsAfterPtw))
        return STATUS_NOT_SUPPORTED;

    if (!PT_OPTION_IS_SUPPORTED(capabilities.IptCapabilities0.Ebx.MtcSupport,
        FilterConfiguration->PacketGenerationOptions.PackteMtc.Enable))
        return STATUS_NOT_SUPPORTED;

    if (!PT_OPTION_IS_SUPPORTED(capabilities.IptCapabilities0.Ebx.PtwriteSupport,
        FilterConfiguration->PacketGenerationOptions.PacketPtw.Enable))
        return STATUS_NOT_SUPPORTED;

    if (!PT_OPTION_IS_SUPPORTED(capabilities.IptCapabilities0.Ebx.PsbAndPmiPreservationSupport,
        FilterConfiguration->PacketGenerationOptions.Misc.InjectPsbPmiOnEnable))
        return STATUS_NOT_SUPPORTED;



    if (!PT_OPTION_IS_SUPPORTED(capabilities.IptCapabilities0.Ebx.Cr3FilteringSupport,
        FilterConfiguration->FilteringOptions.FilterCr3.Enable))
        return STATUS_NOT_SUPPORTED;

    if((capabilities.IptCapabilities0.Eax.MaximumValidSubleaf < 1) && 
        FilterConfiguration->FilteringOptions.FilterRange.Enable)
        return STATUS_NOT_SUPPORTED;

    if (capabilities.IptCapabilities1.Eax.NumberOfAddressRanges < 
        FilterConfiguration->FilteringOptions.FilterRange.NumberOfRanges)
        return STATUS_NOT_SUPPORTED;

    for (int crtAddrRng = 0; crtAddrRng < FilterConfiguration->FilteringOptions.FilterRange.NumberOfRanges; crtAddrRng++)
        if(FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[crtAddrRng].RangeType >= RangeMax)
            return STATUS_NOT_SUPPORTED;


    //if (FilterConfiguration->OutputOptions.OutputType == PtOutputTypeSingleRegion
    //    && ((FilterConfiguration->OutputOptions.OutputBufferOrToPARange.BufferBaseAddress & PT_OUTPUT_CONTIGNUOUS_BASE_MASK) != 0))
    //    return STATUS_NOT_SUPPORTED;
    //else if (FilterConfiguration->OutputOptions.OutputType == PtOutputTypeToPA
    //    && ((FilterConfiguration->OutputOptions.OutputBufferOrToPARange.BufferBaseAddress & PT_OUTPUT_TOPA_BASE_MASK) != 0))
    //    return STATUS_NOT_SUPPORTED;

    //if ((FilterConfiguration->OutputOptions.OutputType == PtOutputTypeSingleRegion)
    //    && (FilterConfiguration->OutputOptions.OutputBufferOrToPARange.BufferSize < 128))
    //    return STATUS_NOT_SUPPORTED;

    // TODO: Validate frequencies with the processor capabilities

    return STATUS_SUCCESS;
}

#define MSR_IA32_PERF_GLOBAL_STATUS		0x0000038e
#define MSR_IA32_PERF_GLOBAL_OVF_CTRL   0x00000390

NTSTATUS
PtUnlinkFullBuffers(
    TOPA_TABLE* Table,
    PVOID OldVaAddresses[],
    unsigned *WrittenAddresses
)
{
    NTSTATUS status = STATUS_SUCCESS;
    IA32_RTIT_STATUS_STRUCTURE ptStatus;
    PtGetStatus(ptStatus);

    unsigned long long bufferCount = (ptStatus.Values.PacketByteCnt / gBufferSize);
    if (bufferCount > gFrequency)
    {
        // Means we have more than one ISR issued
        //DEBUG_STOP();
    }

    unsigned crtBufferCount = Table->CurrentBufferOffset;
    unsigned crtOldVaAddressesCount = 0;
    // Unlink only our current frequency

    do
    {
        if (Table->TopaTableBaseVa[crtBufferCount].END)
            break;

        PVOID newBufferVa;
        PVOID newBufferPa;
        PVOID oldBufferVa;

        oldBufferVa = Table->VirtualTopaPagesAddresses[crtBufferCount];

        status = DuAllocateBuffer(
            (size_t)gBufferSize,
            MmCached,
            TRUE,
            &newBufferVa,
            &newBufferPa
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("DuAllocateBuffer returned error %x\n", status);
            DEBUG_STOP();
        }

        OldVaAddresses[crtOldVaAddressesCount] = oldBufferVa;

        // Save the page virtual address so that it can be mapped in user space
        Table->VirtualTopaPagesAddresses[crtBufferCount] = newBufferVa;
        // Set the topa entry to point at the current page
        Table->TopaTableBaseVa[crtBufferCount].OutputRegionBasePhysicalAddress = ((unsigned long long)newBufferPa) >> 12;
        
        crtBufferCount++;
        crtOldVaAddressesCount++;
    } while (
        crtBufferCount == 0 ||
        (!Table->TopaTableBaseVa[crtBufferCount].END &&
            !Table->TopaTableBaseVa[crtBufferCount - 1].INT)
        );

    if (Table->TopaTableBaseVa[crtBufferCount].END)
    {
        //DEBUG_PRINT("Wrapped around buffer\n");
        crtBufferCount = 0;
    }

    *WrittenAddresses = crtOldVaAddressesCount;
    Table->CurrentBufferOffset = crtBufferCount;

    return status;
}

VOID
PtDpc(
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

    //DEBUG_STOP();
    //DEBUG_PRINT("DPC ON PROC %lld\n", (unsigned long long)SystemArgument1);

    //while (!gTopa->TopaTableBaseVa[i].END)
    //{
    //    for (unsigned long j = 0; j < PAGE_SIZE && ((char*)gTopa->VirtualTopaPagesAddresses[i])[j]; j++)
    //    {
    //        DEBUG_PRINT("%x ", ((char*)gTopa->VirtualTopaPagesAddresses[i])[j]);
    //    }
    //    DEBUG_PRINT("\n");
    //    i++;
    //}

    PVOID *oldVaAddresses = ExAllocatePoolWithTag(NonPagedPool, sizeof(PVOID) * (gFrequency + 1), PT_POOL_TAG);
    unsigned WrittenAddresses = 0;
   
    PtUnlinkFullBuffers(
        gTopa,
        oldVaAddresses,
        &WrittenAddresses
    );

    DuEnqueueElements(
        gQueueHead,
        WrittenAddresses,
        oldVaAddresses
    );

    DEBUG_PRINT("Elements queued ok\n");
    //DEBUG_PRINT("Unlinked addresses: ");
    //for (i = 0; i < WrittenAddresses; i++)
    //{
    //    //DEBUG_PRINT("%p ", oldVaAddresses[i]);
    //    DuFreeBuffer(oldVaAddresses[i], gBufferSize, MmCached);
    //}

    KeSetEvent(&gPagesAvailableEvent, 0, FALSE);

    ExFreePoolWithTag(oldVaAddresses, PT_POOL_TAG);

    //DEBUG_PRINT("\n");

    PtEnableTrace();
}

VOID PtPmiHandler(PKTRAP_FRAME pTrapFrame)
{
    UNREFERENCED_PARAMETER(pTrapFrame);
    IA32_PERF_GLOBAL_STATUS_STRUCTURE perf;
    PKDPC pProcDpc;

    perf.Raw = __readmsr(MSR_IA32_PERF_GLOBAL_STATUS);
    if (!perf.Values.TopaPMI)
        return;

    PtDisableTrace();
    //DEBUG_STOP();

    IA32_PERF_GLOBAL_STATUS_STRUCTURE ctlperf = {0};
    ctlperf.Values.TopaPMI = 1;
    __writemsr(MSR_IA32_PERF_GLOBAL_OVF_CTRL, ctlperf.Raw);

   /* PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE lvt;
    lvt.Raw = (unsigned long)__readmsr(IA32_LVT_REGISTER);
    lvt.Values.Mask = 0;
    __writemsr(IA32_LVT_REGISTER, lvt.Raw);*/
    pProcDpc = (PKDPC)ExAllocatePoolWithTag(
        NonPagedPool, 
        sizeof(KDPC), 
        PT_POOL_TAG
    );

    KeInitializeDpc(
        pProcDpc,
        PtDpc,
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

    //DEBUG_PRINT(">> Received PT Interrupt\n");

    return;
}

typedef VOID(*PMIHANDLER)(PKTRAP_FRAME TrapFrame);

NTSTATUS
PtConfigureProcessorTrace(
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    if (gTraceEnabled)
        return STATUS_ALREADY_INITIALIZED;

    if (FilterConfiguration->OutputOptions.OutputType == PtOutputTypeTraceTransportSubsystem)
        return STATUS_NOT_SUPPORTED;

    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow = {0};

    ia32RtitCtlMsrShadow.Values.CycEn                  = FilterConfiguration->PacketGenerationOptions.PacketCyc.Enable;
    ia32RtitCtlMsrShadow.Values.OS                     = FilterConfiguration->FilteringOptions.FilterCpl.FilterKm;
    ia32RtitCtlMsrShadow.Values.User                   = FilterConfiguration->FilteringOptions.FilterCpl.FilterUm;
    ia32RtitCtlMsrShadow.Values.PwrEvtEn               = FilterConfiguration->PacketGenerationOptions.PacketPwr.Enable;
    ia32RtitCtlMsrShadow.Values.FUPonPTW               = FilterConfiguration->PacketGenerationOptions.Misc.EnableFupPacketsAfterPtw;
    ia32RtitCtlMsrShadow.Values.Cr3Filter              = FilterConfiguration->FilteringOptions.FilterCr3.Enable;
    ia32RtitCtlMsrShadow.Values.MtcEn                  = FilterConfiguration->PacketGenerationOptions.PackteMtc.Enable;
    ia32RtitCtlMsrShadow.Values.TscEn                  = FilterConfiguration->PacketGenerationOptions.PacketTsc.Enable;
    ia32RtitCtlMsrShadow.Values.DisRETC                = FilterConfiguration->PacketGenerationOptions.Misc.DisableRetCompression;
    ia32RtitCtlMsrShadow.Values.PTWEn                  = FilterConfiguration->PacketGenerationOptions.PacketPtw.Enable;
    ia32RtitCtlMsrShadow.Values.BranchEn               = FilterConfiguration->PacketGenerationOptions.PacketCofi.Enable;

    ia32RtitCtlMsrShadow.Values.MtcFreq                = FilterConfiguration->PacketGenerationOptions.PackteMtc.Frequency;
    ia32RtitCtlMsrShadow.Values.CycThresh              = FilterConfiguration->PacketGenerationOptions.PacketCyc.Frequency;
    ia32RtitCtlMsrShadow.Values.PSBFreq                = FilterConfiguration->PacketGenerationOptions.Misc.PsbFrequency;

    ia32RtitCtlMsrShadow.Values.Addr0Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].RangeType;
    ia32RtitCtlMsrShadow.Values.Addr1Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].RangeType;
    ia32RtitCtlMsrShadow.Values.Addr2Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].RangeType;
    ia32RtitCtlMsrShadow.Values.Addr3Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].RangeType;

    ia32RtitCtlMsrShadow.Values.InjectPsbPmiOnEnable   = FilterConfiguration->PacketGenerationOptions.Misc.InjectPsbPmiOnEnable;

    ia32RtitCtlMsrShadow.Values.FabricEn               = 0; // PtOutputTypeTraceTransportSubsystem is not supported
    ia32RtitCtlMsrShadow.Values.ToPA                   = FilterConfiguration->OutputOptions.OutputType == PtOutputTypeSingleRegion ? 0 : 1;

    if (FilterConfiguration->FilteringOptions.FilterCr3.Enable)
        __writemsr(IA32_RTIT_CR3_MATCH, (unsigned long long)FilterConfiguration->FilteringOptions.FilterCr3.Cr3Address);

    if (FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].RangeType != RangeUnused)
    {
        __writemsr(IA32_RTIT_ADDR0_A, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].BaseAddress);
        __writemsr(IA32_RTIT_ADDR0_B, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].EndAddress);
    }

    if (FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].RangeType != RangeUnused)
    {
        __writemsr(IA32_RTIT_ADDR1_A, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].BaseAddress);
        __writemsr(IA32_RTIT_ADDR1_B, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].EndAddress);
    }

    if (FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].RangeType != RangeUnused)
    {
        __writemsr(IA32_RTIT_ADDR2_A, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].BaseAddress);
        __writemsr(IA32_RTIT_ADDR2_B, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].EndAddress);
    }

    if (FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].RangeType != RangeUnused)
    {
        __writemsr(IA32_RTIT_ADDR3_A, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].BaseAddress);
        __writemsr(IA32_RTIT_ADDR3_B, (unsigned long long)FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].EndAddress);
    }

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);
    __writemsr(IA32_RTIT_OUTPUT_MASK_PTRS, FilterConfiguration->OutputOptions.OutputMask.Raw);
    __writemsr(IA32_RTIT_OUTPUT_BASE, FilterConfiguration->OutputOptions.OutputBase);

    IA32_APIC_BASE_STRUCTURE apicBase;
    apicBase.Raw = __readmsr(IA32_APIC_BASE);

    if (apicBase.Values.EnableX2ApicMode)
    {
        DEBUG_PRINT("X2 Apic mode\n");
        PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE lvt;
        unsigned long interruptVector = 0x96;
        NTSTATUS status;

        lvt.Raw = (unsigned long)__readmsr(IA32_LVT_REGISTER);

        if (lvt.Raw != 0)
        {
            // Use the vector that is currently configured
            DEBUG_PRINT("LVT already configured\n");
            interruptVector = lvt.Values.Vector;
        }
        else
        {
            // Todo: maybe I should make sure that I use an interrupt that is not in use by the OS right now
            lvt.Values.Vector = interruptVector;
            lvt.Values.DeliveryMode = 0;
            lvt.Values.Mask = 0;
        }

        PMIHANDLER newPmiHandler = PtPmiHandler;

        status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PVOID), (PVOID)&newPmiHandler);
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("HalSetSystemInformation returned status %X\n", status);
        }
        else
        {
            DEBUG_PRINT("HalSetSystemInformation returned SUCCESS!!!!\n");
        }
    }
    else
    {
        DEBUG_PRINT("X Apic mode\n");
        // TODO: IMplement MMIO apic setup
    }
   
    return STATUS_SUCCESS;
}


NTSTATUS
PtoInitTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Options)
        return STATUS_INVALID_PARAMETER_1;
    //if (!OutputConfiguration)
    //    return STATUS_INVALID_PARAMETER_2;

    if (gPtCapabilities->IptCapabilities0.Ecx.TopaOutputSupport)
    {
        if (gPtCapabilities->IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
        {
            Options->OutputType = PtOutputTypeToPA;
        }
        else
        {
            Options->OutputType = PtOutputTypeToPASingleRegion;
        }
    }
    else
    {
        // TODO: find a better way to report errors
        return STATUS_NOT_SUPPORTED;
    }

    TOPA_TABLE* topaTable;
    PVOID topaTablePa;
    PVOID topaTableVa;

    // Allocate topa table, an interface for the actual pointer to the topa
    topaTable = (TOPA_TABLE*)ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(TOPA_TABLE),
        INTEL_PT_OUTPUT_TAG
    );
    if (!topaTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate the topa virtual address pages array
    topaTable->VirtualTopaPagesAddresses = (PVOID *)ExAllocatePoolWithTag(
        NonPagedPool,
        (Options->TopaEntries + 1) * sizeof(PVOID),
        INTEL_PT_OUTPUT_TAG
    );

    // Allocate the actual topa
    status = DuAllocateBuffer(
        (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
        MmCached,
        TRUE,
        &topaTableVa,
        &topaTablePa
    );
    if (!NT_SUCCESS(status) || !topaTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    topaTable->TopaTableBaseVa = (TOPA_ENTRY*)topaTableVa;
    topaTable->TopaTableBasePa = (unsigned long long)topaTablePa;

    if (Options->OutputType == PtOutputTypeToPA)
    {
        // If the hardware cannot support that many entries, error 
        if (Options->TopaEntries > gPtCapabilities->TopaOutputEntries)
        {
            return STATUS_TOO_MANY_LINKS;
        }

        // Allocate a page for every topa entry
        for (unsigned i = 0; i < Options->TopaEntries; i++)
        {
            PVOID bufferVa;
            PVOID bufferPa;

            status = DuAllocateBuffer(
                PAGE_SIZE,
                MmCached,
                TRUE,
                &bufferVa,
                &bufferPa
            );
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            // Save the page virtual address so that it can be mapped in user space
            topaTable->VirtualTopaPagesAddresses[i] = bufferVa;
            // Set the topa entry to point at the current page
            topaTable->TopaTableBaseVa[i].OutputRegionBasePhysicalAddress = ((unsigned long long)bufferPa) >> 12;
            topaTable->TopaTableBaseVa[i].Size = Size4K;
            if (i % gFrequency == 0 && i)
            {
                topaTable->TopaTableBaseVa[i].INT = TRUE;
            }
        }

        // Last entry will point back to the topa table -> circular buffer
        topaTable->TopaTableBaseVa[Options->TopaEntries].OutputRegionBasePhysicalAddress = ((unsigned long long)topaTablePa) >> 12;
        topaTable->TopaTableBaseVa[Options->TopaEntries].END = TRUE;
        topaTable->TopaTableBaseVa[Options->TopaEntries].STOP = FALSE;
        topaTable->TopaTableBaseVa[Options->TopaEntries].INT = FALSE;

        Options->TopaTable = topaTable;
        // Configure the trace to start at table 0 index 0 in the current ToPA
        Options->OutputMask.Raw = 0x7F;
        Options->OutputBase = (unsigned long long)topaTablePa;

        gTopa = topaTable;

    }
    else if (Options->OutputType == PtOutputTypeToPASingleRegion)
    {
        PVOID bufferVa;
        PVOID bufferPa;

        status = DuAllocateBuffer(
            sizeof(TOPA_TABLE),
            MmCached,
            TRUE,
            &topaTable,
            &topaTablePa
        );

        if (!NT_SUCCESS(status) || !topaTable)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = DuAllocateBuffer(
            PAGE_SIZE,
            MmCached,
            TRUE,
            &bufferVa,
            &bufferPa
        );
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        topaTable->TopaTableBaseVa[0].OutputRegionBasePhysicalAddress = ((ULONG64)bufferPa) >> 12;
        topaTable->TopaTableBaseVa[1].OutputRegionBasePhysicalAddress = ((unsigned long long)topaTablePa) >> 12;
        topaTable->TopaTableBaseVa[1].INT = TRUE;

        Options->TopaTable = topaTable;

        // Leave some space at the beginning of the buffer in case the interrupt doesn't fire exactly when the buffer is filled
        Options->OutputMask.Values.OutputOffset = PAGE_SIZE / 4;
        Options->OutputBase = (unsigned long long)topaTablePa;
    }
    else
    {
        status = STATUS_UNDEFINED_SCOPE;
    }

    return status;
}

NTSTATUS
PtoSwapTopaBuffer(
    unsigned TableIndex,
    TOPA_TABLE* TopaTable,
    PVOID* OldBufferVa
)
{
    NTSTATUS status;
    PVOID bufferVa;
    PVOID bufferPa;

    if (!OldBufferVa)
        return STATUS_INVALID_PARAMETER_3;

    status = DuAllocateBuffer(
        PAGE_SIZE,
        MmCached,
        TRUE,
        &bufferVa,
        &bufferPa
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *OldBufferVa = TopaTable->VirtualTopaPagesAddresses[TableIndex];
    TopaTable->TopaTableBaseVa[TableIndex].OutputRegionBasePhysicalAddress = ((ULONG64)bufferPa) >> 12;
    TopaTable->VirtualTopaPagesAddresses[TableIndex] = bufferVa;

    return status;
}


NTSTATUS
PtoInitSingleRegionOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status;
    PVOID bufferVa;
    PVOID bufferPa;

    status = DuAllocateBuffer(
        PAGE_SIZE,
        MmCached,
        TRUE,
        &bufferVa,
        &bufferPa
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Options->OutputBase = (unsigned long long)bufferPa;
    Options->OutputMask.Raw = 0;
    Options->OutputMask.Values.MaskOrTableOffset = PAGE_SIZE - 1;

    return status;
}


NTSTATUS
PtoInitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    if (Options->OutputType == PtOutputTypeToPASingleRegion || Options->OutputType == PtOutputTypeToPA)
    {
        return PtoInitTopaOutput(
            Options
        );
    }
    else if (Options->OutputType == PtOutputTypeSingleRegion)
    {
        return PtoInitSingleRegionOutput(
            Options
        );
    }

    else
    {
        return STATUS_NOT_SUPPORTED;
    }

}

NTSTATUS
PtoUninitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    UNREFERENCED_PARAMETER(Options);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PtInit(
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!gPtCapabilities)
    {
        gPtCapabilities = (INTEL_PT_CAPABILITIES*)ExAllocatePoolWithTag(
            PagedPool,
            sizeof(INTEL_PT_CAPABILITIES),
            PT_POOL_TAG
        );
        if (!gPtCapabilities)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = PtQueryCapabilities(
            gPtCapabilities
        );

        if (!NT_SUCCESS(status))
        {
            ExFreePoolWithTag(
                gPtCapabilities,
                PT_POOL_TAG
            );
            gPtCapabilities = NULL;
        }

        IA32_RTIT_STATUS_STRUCTURE ptStatus;

        PtGetStatus(ptStatus);

        gTraceEnabled = (ptStatus.Values.TriggerEn != 0);
        if (gTraceEnabled)
        {
            DEBUG_PRINT("IntelPt is in use...\n");
        }
    }
    return status;
}

void
PtUninit(
)
{
    if (gPtCapabilities)
    {
        ExFreePoolWithTag(
            gPtCapabilities,
            PT_POOL_TAG
        );
    }
}

NTSTATUS 
PtSetup(
    INTEL_PT_CONFIGURATION* FilterConfiguration,
    PVOID *UserQueueVa
)
{
    NTSTATUS status;

    status = PtValidateConfigurationRequest(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PtoInitOutputStructure(
        &FilterConfiguration->OutputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (gTraceEnabled)
    {
        status = PtDisableTrace();
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("PtDisableTrace returned status %X\n");
            return status;
        }
    }

    status = PtConfigureProcessorTrace(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DuQueueInit(
        &gQueueHead,
        TRUE
    );

    KeInitializeEvent(
        &gPagesAvailableEvent,
        NotificationEvent,
        FALSE
    );

    //PMDL mdl;
    //PVOID userQueueVa;

    //status = DuMapBufferInUserspace(
    //    gUserQueue,
    //    sizeof(LIST_ENTRY),
    //    &mdl,
    //    &userQueueVa
    //);
    //if (!NT_SUCCESS(status))
    //{
    //    DEBUG_PRINT("Failed to map queue to userspace\n");
    //    DEBUG_STOP();
    //}

    //status = DuMapBufferInUserspace(
    //    gUserQueue->Queue,
    //    queueSize * sizeof(PVOID),
    //    &mdl,
    //    NULL
    //);
    //if (!NT_SUCCESS(status))
    //{
    //    DEBUG_PRINT("Failed to map queue queue to userspace\n");
    //    DEBUG_STOP();
    //}

    //*UserQueueVa = userQueueVa;
    
    * UserQueueVa = NULL;

    return status;

}