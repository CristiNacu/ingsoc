#include "ProcessorTrace.h"
#include "ProcessorTraceShared.h"
#include "DriverUtils.h"
#include "IntelProcessorTraceDefs.h"

IptMemoryAllocationFunction gMemoryAllocation;

#define IPT_POOL_TAG         'IPTP'
#define DEFAULT_NUMBER_OF_TOPA_ENTRIES 10
#define IptFlush() _mm_sfence();

NTSTATUS
IptAllocateNewTopaBuffer(
    unsigned                    EntriesCount,
    TOPA_ENTRY* TopaTableVa,
    PVOID                       NextTopaPa,
    PMDL* Mdl
);

BOOLEAN
IptTopaPmiReason(
)
{
    IA32_PERF_GLOBAL_STATUS_STRUCTURE perf;
    perf.Raw = __readmsr(MSR_IA32_PERF_GLOBAL_STATUS);
    if (!perf.Values.TopaPMI)
        return FALSE;
    return TRUE;
}

void
IptResetPmi(
)
{
    IA32_PERF_GLOBAL_STATUS_STRUCTURE ctlperf = { 0 };
    ctlperf.Values.TopaPMI = 1;
    __writemsr(MSR_IA32_PERF_GLOBAL_OVF_CTRL, ctlperf.Raw);
}

NTSTATUS
IptQueryCapabilities(
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
IptGetCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
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
    }

    memcpy_s(
        Capabilities,
        sizeof(INTEL_PT_CAPABILITIES),
        gPtCapabilities,
        sizeof(INTEL_PT_CAPABILITIES)
    );

    return status;
}

NTSTATUS
IptEnableTrace(
    INTEL_PT_OUTPUT_OPTIONS* CpuOptions
)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;

    DEBUG_PRINT("Enabling trace on cpu %d\n", KeGetCurrentProcessorNumber());

    if (IptError())
    {
        IptClearError();
    }

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Fields.TraceEn = TRUE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);
    
    if (IptError() || !IptEnabled())
    {
        ULONG crtCpu = KeGetCurrentProcessorNumber();
        DEBUG_PRINT("Pt TraceEnable failed on cpu %d\n", crtCpu);
        DEBUG_STOP();
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        DEBUG_PRINT("Enabled trace on cpu %d\n", KeGetCurrentProcessorNumber());
    }

    CpuOptions->RunningStatus = IPT_STATUS_ENABLED;
    return STATUS_SUCCESS;
}

void
IptClearTraceEn(
)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Fields.TraceEn = FALSE;
    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);
}

NTSTATUS
IptDisableTrace(
    PMDL *Mdl,
    INTEL_PT_OUTPUT_OPTIONS* CpuOptions
)
{
    DEBUG_PRINT("Disabling trace on cpu %d\n", KeGetCurrentProcessorNumber());
    IptClearTraceEn();
    DEBUG_PRINT("Disabled trace on cpu %d\n", KeGetCurrentProcessorNumber());
    IptFlush();



    if(Mdl)
        *Mdl = CpuOptions->TopaMdl;
    
    CpuOptions->RunningStatus = IPT_STATUS_DISABLED;

    // TODO: Restore the old IPT state when disabling execution


    return STATUS_SUCCESS;
}

NTSTATUS
IptPauseTrace(
    INTEL_PT_OUTPUT_OPTIONS* CpuOptions
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (IptError())
    {
        DEBUG_STOP();

        if (!IptEnabled())
        {
            DEBUG_PRINT("IPT DISABLED! Walter Fuck?!\n");
        }
    }

    if (IptEnabled())
        IptClearTraceEn();

    CpuOptions->RunningStatus = IPT_STATUS_PAUSED;

    return status;

}

NTSTATUS
IptResumeTrace(
    INTEL_PT_OUTPUT_OPTIONS* CpuOptions
)
{
    NTSTATUS status = STATUS_SUCCESS;
    if (!IptEnabled())
    {
        status = IptEnableTrace(CpuOptions);
    }
    return status;

}

NTSTATUS 
IptUnlinkFullTopaBuffers(
    PMDL* Mdl,
    INTEL_PT_OUTPUT_OPTIONS* CpuOptions,
    BOOLEAN AllocateNewTopa
)
{
    if (CpuOptions->RunningStatus == IPT_STATUS_ENABLED)
        return STATUS_CANNOT_MAKE;

    DEBUG_PRINT(">>> UNLINKING ON AP %d\n", KeGetCurrentProcessorNumber());
    NTSTATUS status = STATUS_SUCCESS;
    PMDL mdl;

    if (!Mdl)
        return STATUS_INVALID_PARAMETER_1;

    if (!CpuOptions)
        return STATUS_INVALID_PARAMETER_2;


    mdl = CpuOptions->TopaMdl;
    *Mdl = CpuOptions->TopaMdl;
    if (AllocateNewTopa)
    {
        status = IptAllocateNewTopaBuffer(
            CpuOptions->TopaEntries,
            CpuOptions->TopaTableVa,
            CpuOptions->TopaTablePa,
            &(PMDL)CpuOptions->TopaMdl
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("IptAlocateNewTopaBuffer failed with status %X\n", status);
        }

        IA32_RTIT_STATUS_STRUCTURE ptStatus;
        PtGetStatus(ptStatus);

        __writemsr(IA32_RTIT_OUTPUT_MASK_PTRS, CpuOptions->OutputMask.Raw);
        __writemsr(IA32_RTIT_OUTPUT_BASE, (unsigned long long)CpuOptions->TopaTablePa);
    }
    return status;
}

BOOLEAN IptError()
{
    IA32_RTIT_STATUS_STRUCTURE ptStatus;
    PtGetStatus(ptStatus);
    return !!ptStatus.Fields.Error;
}

void IptClearError()
{
    IA32_RTIT_STATUS_STRUCTURE ptStatus;
    PtGetStatus(ptStatus);
    ptStatus.Fields.Error = FALSE;
    PtSetStatus(ptStatus.Raw);
}

BOOLEAN IptEnabled()
{
    IA32_RTIT_STATUS_STRUCTURE ptStatus;
    PtGetStatus(ptStatus);
    return !!ptStatus.Fields.TriggerEn;
}

NTSTATUS
IptValidateConfigurationRequest(
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    INTEL_PT_CAPABILITIES capabilities;
    NTSTATUS status;
    status = IptGetCapabilities(
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

    return STATUS_SUCCESS;
}

NTSTATUS
IptConfigureProcessorTrace(
    INTEL_PT_CONFIGURATION* FilterConfiguration,
    INTEL_PT_OUTPUT_OPTIONS* OutputOptions
)
{
    //if (FilterConfiguration->OutputOptions.OutputType == PtOutputTypeTraceTransportSubsystem)
    //    return STATUS_NOT_SUPPORTED;

    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow = {0};

    ia32RtitCtlMsrShadow.Fields.CycEn                  = FilterConfiguration->PacketGenerationOptions.PacketCyc.Enable;
    ia32RtitCtlMsrShadow.Fields.OS                     = FilterConfiguration->FilteringOptions.FilterCpl.FilterKm;
    ia32RtitCtlMsrShadow.Fields.User                   = FilterConfiguration->FilteringOptions.FilterCpl.FilterUm;
    ia32RtitCtlMsrShadow.Fields.PwrEvtEn               = FilterConfiguration->PacketGenerationOptions.PacketPwr.Enable;
    ia32RtitCtlMsrShadow.Fields.FUPonPTW               = FilterConfiguration->PacketGenerationOptions.Misc.EnableFupPacketsAfterPtw;
    ia32RtitCtlMsrShadow.Fields.Cr3Filter              = FilterConfiguration->FilteringOptions.FilterCr3.Enable;
    ia32RtitCtlMsrShadow.Fields.MtcEn                  = FilterConfiguration->PacketGenerationOptions.PackteMtc.Enable;
    ia32RtitCtlMsrShadow.Fields.TscEn                  = FilterConfiguration->PacketGenerationOptions.PacketTsc.Enable;
    ia32RtitCtlMsrShadow.Fields.DisRETC                = FilterConfiguration->PacketGenerationOptions.Misc.DisableRetCompression;
    ia32RtitCtlMsrShadow.Fields.PTWEn                  = FilterConfiguration->PacketGenerationOptions.PacketPtw.Enable;
    ia32RtitCtlMsrShadow.Fields.BranchEn               = FilterConfiguration->PacketGenerationOptions.PacketCofi.Enable;

    ia32RtitCtlMsrShadow.Fields.MtcFreq                = FilterConfiguration->PacketGenerationOptions.PackteMtc.Frequency;
    ia32RtitCtlMsrShadow.Fields.CycThresh              = FilterConfiguration->PacketGenerationOptions.PacketCyc.Frequency;
    ia32RtitCtlMsrShadow.Fields.PSBFreq                = FilterConfiguration->PacketGenerationOptions.Misc.PsbFrequency;

    ia32RtitCtlMsrShadow.Fields.Addr0Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].RangeType;
    ia32RtitCtlMsrShadow.Fields.Addr1Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].RangeType;
    ia32RtitCtlMsrShadow.Fields.Addr2Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].RangeType;
    ia32RtitCtlMsrShadow.Fields.Addr3Cfg               = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].RangeType;

    ia32RtitCtlMsrShadow.Fields.InjectPsbPmiOnEnable   = FilterConfiguration->PacketGenerationOptions.Misc.InjectPsbPmiOnEnable;

    ia32RtitCtlMsrShadow.Fields.FabricEn               = 0; // PtOutputTypeTraceTransportSubsystem is not supported
    ia32RtitCtlMsrShadow.Fields.ToPA                   = 1;/*FilterConfiguration->OutputOptions.OutputType == PtOutputTypeSingleRegion ? 0 : 1;*/

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
    __writemsr(IA32_RTIT_OUTPUT_MASK_PTRS, OutputOptions->OutputMask.Raw);
    __writemsr(IA32_RTIT_OUTPUT_BASE, (unsigned long long)OutputOptions->TopaTablePa);
   
    return STATUS_SUCCESS;
}

NTSTATUS
IptAllocateNewTopaBuffer(
    unsigned                    EntriesCount,
    TOPA_ENTRY*                 TopaTableVa,
    PVOID                       NextTopaPa,
    PMDL*                       Mdl
)
{
    PMDL mdl;
    PPFN_NUMBER pfnArray;
    PHYSICAL_ADDRESS lowAddress = { .QuadPart = 0 };
    PHYSICAL_ADDRESS highAddress = { .QuadPart = -1L };

    if (!Mdl)
        return STATUS_INVALID_PARAMETER_4;

    mdl = MmAllocatePagesForMdlEx(
        lowAddress,
        highAddress,
        lowAddress,
        EntriesCount * PAGE_SIZE,
        MmCached,
        MM_ALLOCATE_FULLY_REQUIRED
    );
    if (mdl == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES; // mood 
    }

    char* buff = (char*)MmMapLockedPagesSpecifyCache(
        mdl,
        KernelMode,
        MmCached,
        NULL,
        FALSE,
        NormalPagePriority
    );

    RtlFillBytes(
        buff,
        EntriesCount * PAGE_SIZE,
        0xFF
    );

    MmUnmapLockedPages(
        buff,
        mdl
    );

    pfnArray = MmGetMdlPfnArray(mdl);


    for (unsigned i = 0; i < EntriesCount; i++)
    {
        TopaTableVa[i].OutputRegionBasePhysicalAddress = pfnArray[i];
        TopaTableVa[i].END = FALSE;
        TopaTableVa[i].INT = (i == (EntriesCount - 1)) ? TRUE : FALSE;
        TopaTableVa[i].STOP = FALSE;
        TopaTableVa[i].Size = Size4K;
    }

    TopaTableVa[EntriesCount].OutputRegionBasePhysicalAddress = ((unsigned long long)NextTopaPa) >> 12;
    TopaTableVa[EntriesCount].STOP = FALSE;
    TopaTableVa[EntriesCount].INT = FALSE;
    TopaTableVa[EntriesCount].END = TRUE;
    *Mdl = mdl;

    return STATUS_SUCCESS;
}

NTSTATUS
IptInitTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;
    TOPA_ENTRY* topaTableVa;


    PHYSICAL_ADDRESS lowAddress = { .QuadPart = 0 };
    PHYSICAL_ADDRESS highAddress = { .QuadPart = -1L };
    
    if (!Options)
        return STATUS_INVALID_PARAMETER_1;

    if (gPtCapabilities->IptCapabilities0.Ecx.TopaOutputSupport)
    {
        if (gPtCapabilities->IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
        {
            Options->OutputType = PtOutputTypeToPA;
            Options->TopaEntries = DEFAULT_NUMBER_OF_TOPA_ENTRIES;
        }
        else
        {
            Options->OutputType = PtOutputTypeToPASingleRegion;
            Options->TopaEntries = 1;
        }
    }
    else
    {
        // TODO: find a better way to report errors
        return STATUS_NOT_SUPPORTED;
    }

    // If the hardware cannot support that many entries, error 
    if (Options->TopaEntries > gPtCapabilities->TopaOutputEntries)
    {
        return STATUS_TOO_MANY_LINKS;
    }

    // Allocate the actual topa
    topaTableVa = MmAllocateContiguousMemorySpecifyCache(
        (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
        lowAddress,
        highAddress,
        lowAddress,
        MmCached
    );
    if (!topaTableVa)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlFillMemory(
        topaTableVa,
        (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
        0
    );

    Options->TopaTablePa = (PVOID)MmGetPhysicalAddress(topaTableVa).QuadPart;
    Options->TopaTableVa = (PVOID)topaTableVa;

    status = IptAllocateNewTopaBuffer(
        Options->TopaEntries,
        topaTableVa,
        Options->TopaTablePa,
        &(PMDL)Options->TopaMdl
    );
    if (!NT_SUCCESS(status))
        return status;

    // Configure the trace to start at table 0 index 0 in the current ToPA
    Options->OutputMask.Raw = 0x7F;
    Options->RunningStatus = IPT_STATUS_INITIALIZED;

    return status;
}


NTSTATUS
IptInitSingleRegionOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID bufferVa;
    PVOID bufferPa;

    gMemoryAllocation(
        PAGE_SIZE,
        PAGE_SIZE,
        &bufferVa,
        &bufferPa
    );
    if (!bufferVa || !bufferPa)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Options->TopaTablePa = bufferPa;
    Options->TopaTableVa = bufferVa;
    Options->OutputMask.Raw = 0;
    Options->OutputMask.Fields.MaskOrTableOffset = PAGE_SIZE - 1;

    return status;
}


NTSTATUS
IptInitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    if (gPtCapabilities->IptCapabilities0.Ecx.TopaOutputSupport)
    {
        return IptInitTopaOutput(
            Options
        );
    }
    else
    {
        return IptInitSingleRegionOutput(
            Options
        );
    }
}

NTSTATUS
IptUninitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    if (Options->OutputType == PtOutputTypeToPA)
    {
        MmFreePagesFromMdl(
            Options->TopaMdl
        );

        ExFreePoolWithTag(
            Options->TopaTablePa,
            IPT_POOL_TAG
        );
    }
    else
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    ExFreePoolWithTag(
        Options,
        IPT_POOL_TAG
    );
    return STATUS_SUCCESS;
}

NTSTATUS
IptInitPerCore(
    INTEL_PT_CONTROL_STRUCTURE *ControlStructure
)
{
    INTEL_PT_OUTPUT_OPTIONS* outputOptions;
    NTSTATUS status;

    ControlStructure->IptEnabled = IptEnabled();
    if (ControlStructure->IptEnabled)
    {
        DEBUG_PRINT("IntelPt is already in use...\n");
    }

    outputOptions = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(INTEL_PT_OUTPUT_OPTIONS),
        IPT_POOL_TAG
    );
    if (!outputOptions)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    outputOptions->TopaEntries = 10;

    status = IptInitOutputStructure(
        outputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (IptEnabled())
    {
        status = IptDisableTrace(NULL, outputOptions);
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("PtDisableTrace returned status %X\n", status);
            return status;
        }
        // TODO: If ipt is already in use, implement a mechanism to maintain the old state and revert it when IPT is disabled
    }

    ControlStructure->OutputOptions = outputOptions;

    return STATUS_SUCCESS;
}

NTSTATUS
IptInit(
    IptMemoryAllocationFunction MemoryAllocationFunction
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!MemoryAllocationFunction)
        return STATUS_INVALID_PARAMETER_1;

    gMemoryAllocation = MemoryAllocationFunction;

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

        status = IptQueryCapabilities(
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
    }

    return status;
}

void
IptUninit(
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
IptSetup(
    INTEL_PT_CONFIGURATION* FilterConfiguration,
    INTEL_PT_CONTROL_STRUCTURE *ControlStructure
)
{
    UNREFERENCED_PARAMETER(ControlStructure);

    NTSTATUS status;
    

    status = IptValidateConfigurationRequest(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = IptConfigureProcessorTrace(
        FilterConfiguration,
        ControlStructure->OutputOptions
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    ULONG crtCpu = KeGetCurrentProcessorNumber();
    UNREFERENCED_PARAMETER(crtCpu);

    return status;

}