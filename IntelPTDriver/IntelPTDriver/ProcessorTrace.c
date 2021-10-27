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

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Fields.TraceEn = TRUE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);
    
    if (IptError() || !IptEnabled())
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


NTSTATUS
IptInitTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Options)
        return STATUS_INVALID_PARAMETER_1;
    //if (!OutputConfiguration)
    //    return STATUS_INVALID_PARAMETER_2;

    if (gPtCapabilities->IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
    {
        Options->OutputType = PtOutputTypeToPA;
    }
    else
    {
        Options->OutputType = PtOutputTypeToPASingleRegion;
    }

    TOPA_TABLE* topaTable;
    PVOID topaTablePa;
    PVOID topaTableVa;

    // Allocate topa table, an interface for the actual pointer to the topa
    gMemoryAllocation(
        (size_t)(unsigned)gBufferSize,
        PAGE_SIZE,
        &topaTable,
        NULL
    );
    if (!topaTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    topaTable->VirtualTopaPagesAddresses = 
        (PVOID *)((unsigned long long)topaTable + sizeof(TOPA_TABLE));

    // Allocate the actual topa
    gMemoryAllocation(
        (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
        PAGE_SIZE,
        &topaTableVa,
        &topaTablePa
    );
    if (!topaTableVa || !topaTablePa)
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

        gMemoryAllocation(
            sizeof(TOPA_TABLE),
            PAGE_SIZE,
            &topaTable,
            &topaTablePa
        );

        if (!topaTable || !topaTablePa)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        gMemoryAllocation(
            PAGE_SIZE,
            PAGE_SIZE,
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
        Options->OutputMask.Fields.OutputOffset = PAGE_SIZE / 4;
        Options->OutputBase = (unsigned long long)topaTablePa;
    }
    else
    {
        status = STATUS_UNDEFINED_SCOPE;
    }

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
    INTEL_PT_OUTPUT_OPTIONS outputOptions;
    status = IptValidateConfigurationRequest(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = IptInitOutputStructure(
        &outputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (gTraceEnabled)
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
        &outputOptions
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    return status;

}