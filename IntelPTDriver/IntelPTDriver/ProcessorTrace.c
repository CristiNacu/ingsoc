#include "ProcessorTrace.h"
#include "ProcessorTraceShared.h"
#include "DriverUtils.h"
#include "IntelProcessorTraceDefs.h"

IptMemoryAllocationFunction gMemoryAllocation;

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

)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    DEBUG_STOP();
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
        DEBUG_PRINT("Pt TraceEnable failed %d\n", crtCpu);
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
IptDisableTrace(
)
{
    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Fields.TraceEn = FALSE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);

    if (IptError() || !IptEnabled())
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
IptPauseTrace()
{
    NTSTATUS status = STATUS_SUCCESS;
    if (gTraceEnabled)
    {
        status = IptDisableTrace();
        gTraceEnabled = TRUE;
    }
    return status;

}

NTSTATUS
IptResumeTrace()
{
    NTSTATUS status = STATUS_SUCCESS;
    if (gTraceEnabled)
    {
        status = IptEnableTrace();
    }
    return status;

}

NTSTATUS IptUnlinkFullTopaBuffers()
{
    return STATUS_NOT_IMPLEMENTED;
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
IptUnlinkFullBuffers(
    TOPA_TABLE* Table,
    PVOID OldVaAddresses[],
    unsigned *WrittenAddresses
)
{
    NTSTATUS status = STATUS_SUCCESS;
    IA32_RTIT_STATUS_STRUCTURE ptStatus;
    PtGetStatus(ptStatus);

    unsigned long long bufferCount = (ptStatus.Fields.PacketByteCnt / gBufferSize);
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

        gMemoryAllocation(
            (size_t)(unsigned)gBufferSize,
            PAGE_SIZE,
            &newBufferVa,
            &newBufferPa
        );
        if (!newBufferVa || !newBufferPa)
        {
            DEBUG_PRINT("gMemoryAllocation returned error %x\n", status);
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

NTSTATUS
IptConfigureProcessorTrace(
    INTEL_PT_CONFIGURATION* FilterConfiguration,
    INTEL_PT_OUTPUT_OPTIONS* OutputOptions
)
{
    if (gTraceEnabled)
        return STATUS_ALREADY_INITIALIZED;

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

    DEBUG_STOP();
    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);
    __writemsr(IA32_RTIT_OUTPUT_MASK_PTRS, OutputOptions->OutputMask.Raw);
    __writemsr(IA32_RTIT_OUTPUT_BASE, OutputOptions->OutputBase);
    
    //IA32_APIC_BASE_STRUCTURE apicBase;
    //apicBase.Raw = __readmsr(IA32_APIC_BASE);

    //if (apicBase.Values.EnableX2ApicMode)
    //{
    //    DEBUG_PRINT("X2 Apic mode\n");
    //    PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE lvt;
    //    unsigned long interruptVector = 0x96;
    //    NTSTATUS status;

    //    lvt.Raw = (unsigned long)__readmsr(IA32_LVT_REGISTER);

    //    if (lvt.Raw != 0)
    //    {
    //        // Use the vector that is currently configured
    //        DEBUG_PRINT("LVT already configured\n");
    //        interruptVector = lvt.Values.Vector;
    //    }
    //    else
    //    {
    //        // Todo: maybe I should make sure that I use an interrupt that is not in use by the OS right now
    //        lvt.Values.Vector = interruptVector;
    //        lvt.Values.DeliveryMode = 0;
    //        lvt.Values.Mask = 0;
    //    }

    //    PMIHANDLER newPmiHandler = PtPmiHandler;

    //    status = HalSetSystemInformation(HalProfileSourceInterruptHandler, sizeof(PVOID), (PVOID)&newPmiHandler);
    //    if (!NT_SUCCESS(status))
    //    {
    //        DEBUG_PRINT("HalSetSystemInformation returned status %X\n", status);
    //    }
    //    else
    //    {
    //        DEBUG_PRINT("HalSetSystemInformation returned SUCCESS!!!!\n");
    //    }
    //}
    //else
    //{
    //    DEBUG_PRINT("X Apic mode\n");
    //    // TODO: IMplement MMIO apic setup
    //}
   
    return STATUS_SUCCESS;
}

#define DEFAULT_NUMBER_OF_TOPA_ENTRIES 10

NTSTATUS
IptInitTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;
    TOPA_TABLE* topaTable;
    PVOID topaTablePa;
    TOPA_ENTRY* topaTableVa;
    PMDL mdl;
    PPFN_NUMBER pfnArray;

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
    DEBUG_STOP();
    // Allocate topa table, an interface for the actual pointer to the topa
    status = DuAllocateBuffer(
        sizeof(TOPA_TABLE) + (Options->TopaEntries + 1) * sizeof(PVOID),
        MmCached,
        TRUE,
        &topaTable,
        NULL
    );
    if (!NT_SUCCESS(status) || !topaTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Is this required?
    topaTable->VirtualTopaPagesAddresses =
        (PVOID*)((unsigned long long)topaTable + sizeof(TOPA_TABLE));

    // Allocate the actual topa
    status = DuAllocateBuffer(
        (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
        MmCached,
        TRUE,
        (PVOID)&topaTableVa,
        &topaTablePa
    );
    if (!NT_SUCCESS(status) || !topaTableVa || !topaTablePa)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    topaTable->TopaTableBaseVa = (TOPA_ENTRY*)topaTableVa;
    topaTable->TopaTableBasePa = (unsigned long long)topaTablePa;
    
    mdl = MmAllocatePagesForMdlEx(
        lowAddress,
        highAddress,
        lowAddress,
        Options->TopaEntries * PAGE_SIZE,
        MmCached,
        MM_ALLOCATE_FULLY_REQUIRED
    );
    pfnArray = MmGetMdlPfnArray(mdl);

    for (unsigned i = 0; i < Options->TopaEntries; i++)
    {
        topaTableVa[i].OutputRegionBasePhysicalAddress = pfnArray[i];
        topaTableVa[i].END = FALSE;
        topaTableVa[i].INT = (i % gFrequency == 0) ? TRUE : FALSE;
        topaTableVa[i].STOP = FALSE;
        topaTableVa[i].Size = Size4K;
    }

    topaTableVa[Options->TopaEntries].OutputRegionBasePhysicalAddress = ((unsigned long long)topaTablePa) >> 12;
    topaTableVa[Options->TopaEntries].STOP = FALSE;
    topaTableVa[Options->TopaEntries].INT = FALSE;
    topaTableVa[Options->TopaEntries].END = TRUE;

    Options->TopaTable = topaTable;
    // Configure the trace to start at table 0 index 0 in the current ToPA
    Options->OutputMask.Raw = 0x7F;
    Options->OutputBase = (unsigned long long)topaTablePa;
    
    return status;
}

NTSTATUS
IptSwapTopaBuffer(
    unsigned TableIndex,
    TOPA_TABLE* TopaTable,
    PVOID* OldBufferVa
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID bufferVa;
    PVOID bufferPa;

    if (!OldBufferVa)
        return STATUS_INVALID_PARAMETER_3;

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

    *OldBufferVa = TopaTable->VirtualTopaPagesAddresses[TableIndex];
    TopaTable->TopaTableBaseVa[TableIndex].OutputRegionBasePhysicalAddress = ((ULONG64)bufferPa) >> 12;
    TopaTable->VirtualTopaPagesAddresses[TableIndex] = bufferVa;

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

    Options->OutputBase = (unsigned long long)bufferPa;
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
    UNREFERENCED_PARAMETER(Options);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
IptInitPerCore(
    INTEL_PT_CONTROL_STRUCTURE *ControlStructure
)
{
    ControlStructure->IptEnabled = IptEnabled();
    if (gTraceEnabled)
    {
        DEBUG_PRINT("IntelPt is in use...\n");
    }

    // TODO: How shold i handle the case where IPT is already enabled?
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
    INTEL_PT_OUTPUT_OPTIONS *outputOptions;
    
    status = IptValidateConfigurationRequest(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    gMemoryAllocation(
        sizeof(INTEL_PT_OUTPUT_OPTIONS),
        0,
        &outputOptions,
        NULL
    );
    if (!outputOptions)
        return STATUS_INSUFFICIENT_RESOURCES;

    outputOptions->TopaEntries = 10;
    DEBUG_STOP();

    status = IptInitOutputStructure(
        outputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (ControlStructure->IptEnabled)
    {
        status = IptDisableTrace();
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("PtDisableTrace returned status %X\n", status);
            return status;
        }
    }

    status = IptConfigureProcessorTrace(
        FilterConfiguration,
        outputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    
    ULONG crtCpu = KeGetCurrentProcessorNumber();
    UNREFERENCED_PARAMETER(crtCpu);

    ControlStructure->IptHandler = (PVOID)outputOptions;

    return status;

}