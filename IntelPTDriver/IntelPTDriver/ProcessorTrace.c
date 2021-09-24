#include "ProcessorTrace.h"
#include "ProcessorTraceControlStructure.h"

typedef enum CPUID_INDEX {
    CpuidEax,
    CpuidEbx,
    CpuidEcx,
    CpuidEdx
} CPUID_INDEX;

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
#define IA32_RTIT_STATUS                    0x571
#define IA32_RTIT_CR3_MATCH                 0x572

#define IA32_RTIT_ADDR0_A                   0x580
#define IA32_RTIT_ADDR0_B                   0x581
#define IA32_RTIT_ADDR1_A                   0x582
#define IA32_RTIT_ADDR1_B                   0x583
#define IA32_RTIT_ADDR2_A                   0x584
#define IA32_RTIT_ADDR2_B                   0x585
#define IA32_RTIT_ADDR3_A                   0x586
#define IA32_RTIT_ADDR3_B                   0x587

#define PT_POOL_TAG                         'PTPT'

#define PT_OPTION_IS_SUPPORTED(capability, option)  (((!capability) && (option)) ? FALSE : TRUE)

#define PT_OUTPUT_CONTIGNUOUS_BASE_MASK             0x7F
#define PT_OUTPUT_TOPA_BASE_MASK                    0xFFF


INTEL_PT_CAPABILITIES   *gPtCapabilities = NULL;
BOOLEAN                 gTraceEnabled = FALSE;

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
PtGetStatus(
    IA32_RTIT_STATUS_STRUCTURE *Status
)
{
    Status->Raw = __readmsr(IA32_RTIT_STATUS);
    return STATUS_SUCCESS;
}

NTSTATUS
PtEnableTrace(
)
{
    DEBUG_STOP();

    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    IA32_RTIT_STATUS_STRUCTURE status;

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Values.TraceEn = TRUE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);

    PtGetStatus(&status);
    if (status.Values.Error || !status.Values.TriggerEn)
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
    DEBUG_STOP();

    IA32_RTIT_CTL_STRUCTURE ia32RtitCtlMsrShadow;
    IA32_RTIT_STATUS_STRUCTURE status;

    ia32RtitCtlMsrShadow.Raw = __readmsr(IA32_RTIT_CTL);
    ia32RtitCtlMsrShadow.Values.TraceEn = FALSE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow.Raw);

    PtGetStatus(&status);
    if (status.Values.Error || status.Values.TriggerEn)
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

//unsigned long long
//PtGenerateOutputMask(
//    INTEL_PT_OUTPUT_BUFFER* Options
//)
//{
//
//    // TODO: update this for ToPA
//    DEBUG_STOP();
//    IA32_RTIT_OUTPUT_MASK_STRUCTURE result;
//
//    // TODO: I assume the user always allocates & sends a buffer size power of 2. Is this ok? Think it trough. 
//    // If it is should check this in the validation process else i'll surely fuck up.
//    // Also result.Values.MaskOrTableOffset & Options->BufferBaseAddress must equal 0, if not generates runtime error... To be considered
//    result.Values.MaskOrTableOffset = Options->BufferSize - 1;
//    result.Values.OutputOffset = 0;
//
//    return result.Raw;
//}

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
    __writemsr(IA32_RTIT_OUTPUT_BASE, FilterConfiguration->OutputOptions.TopaTable->TopaTableBasePa);


    return STATUS_SUCCESS;
}

NTSTATUS
PtInit(
)
{
    NTSTATUS status = STATUS_SUCCESS;

    DEBUG_STOP();

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

        status = PtGetStatus(
            &ptStatus
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("PtGetStatus failed sith status %X\n", status);
            return status;
        }

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
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    NTSTATUS status;
    DEBUG_STOP();
    status = PtValidateConfigurationRequest(
        FilterConfiguration
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

    return status;

}