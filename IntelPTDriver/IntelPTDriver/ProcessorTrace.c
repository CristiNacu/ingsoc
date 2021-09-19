#include "ProcessorTrace.h"

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

// As described in Intel Manual Volume 4 Chapter 2 Table 2-2 pg 4630
typedef struct _IA32_RTIT_CTL_STRUCTURE {

    unsigned long long TraceEn                  : 1;        // 0
    unsigned long long CycEn                    : 1;        // 1  
    unsigned long long OS                       : 1;        // 2
    unsigned long long User                     : 1;        // 3
    unsigned long long PwrEvtEn                 : 1;        // 4
    unsigned long long FUPonPTW                 : 1;        // 5
    unsigned long long FabricEn                 : 1;        // 6
    unsigned long long Cr3Filter                : 1;        // 7
    unsigned long long ToPA                     : 1;        // 8
    unsigned long long MtcEn                    : 1;        // 9
    unsigned long long TscEn                    : 1;        // 10
    unsigned long long DisRETC                  : 1;        // 11
    unsigned long long PTWEn                    : 1;        // 12
    unsigned long long BranchEn                 : 1;        // 13
    unsigned long long MtcFreq                  : 4;        // 17:14
    unsigned long long ReservedZero0            : 1;        // 18
    unsigned long long CycThresh                : 4;        // 22:19
    unsigned long long ReservedZero1            : 1;        // 23
    unsigned long long PSBFreq                  : 4;        // 27:24
    unsigned long long ReservedZero2            : 4;        // 31:28
    unsigned long long Addr0Cfg                 : 4;        // 35:32
    unsigned long long Addr1Cfg                 : 4;        // 39:36
    unsigned long long Addr2Cfg                 : 4;        // 43:40
    unsigned long long Addr3Cfg                 : 4;        // 47:44
    unsigned long long ReservedZero3            : 8;        // 55:48
    unsigned long long InjectPsbPmiOnEnable     : 1;        // 56
    unsigned long long ReservedZero4            : 7;        // 63:57

} IA32_RTIT_CTL_STRUCTURE;

// As described in Intel Manual Volume 4 Chapter 2 Table 2-2 pg 4630
typedef struct _IA32_RTIT_STATUS_STRUCTURE {

    unsigned long long FilterEn                 : 1;        // 0
    unsigned long long ContextEn                : 1;        // 1  
    unsigned long long TriggerEn                : 1;        // 2
    unsigned long long Reserved0                : 1;        // 3
    unsigned long long Error                    : 1;        // 4
    unsigned long long Stopped                  : 1;        // 5
    unsigned long long SendPSB                  : 1;        // 6
    unsigned long long PendToPAPMI              : 1;        // 7
    unsigned long long ReservedZero0            : 24;       // 31:8
    unsigned long long PacketByteCnt            : 17;       // 48:32
    unsigned long long Reserved1                : 15;       // 10
} IA32_RTIT_STATUS_STRUCTURE;



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
    }
    return status;
}

NTSTATUS
PTGetCapabilities(
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
    unsigned long long ia32RtitStatusMsrShadow = __readmsr(IA32_RTIT_STATUS);
    memcpy_s(
        Status,
        sizeof(IA32_RTIT_STATUS_STRUCTURE),
        &ia32RtitStatusMsrShadow,
        sizeof(unsigned long long)
    );

    return STATUS_SUCCESS;
}

NTSTATUS
PtEnableTrace(
)
{
    unsigned long long ia32RtitCtlMsrShadow = __readmsr(IA32_RTIT_CTL);
    IA32_RTIT_CTL_STRUCTURE* ia32RtitCtlMsrShadowPtr = 
        (IA32_RTIT_CTL_STRUCTURE*) &ia32RtitCtlMsrShadow;

    ia32RtitCtlMsrShadowPtr->TraceEn = TRUE;

    __writemsr(IA32_RTIT_CTL, ia32RtitCtlMsrShadow);

    gTraceEnabled = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
PtDisableTrace(
)
{
    unsigned long long ia32RtitMsrShadow = __readmsr(IA32_RTIT_CTL);
    IA32_RTIT_CTL_STRUCTURE* ia32RtitMsrShadowPtr = 
        (IA32_RTIT_CTL_STRUCTURE*) &ia32RtitMsrShadow;

    ia32RtitMsrShadowPtr->TraceEn = FALSE;

    __writemsr(IA32_RTIT_CTL, ia32RtitMsrShadow);

    gTraceEnabled = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS
PtValidateConfigurationRequest(
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    INTEL_PT_CAPABILITIES capabilities;
    NTSTATUS status;
    status = PTGetCapabilities(
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

    // TODO: Validate frequencies with the processor capabilities

    return STATUS_SUCCESS;
}

NTSTATUS
PtConfigureProcessorTrace(
    INTEL_PT_CONFIGURATION* FilterConfiguration
)
{
    if (gTraceEnabled)
        return STATUS_ALREADY_INITIALIZED;

    IA32_RTIT_CTL_STRUCTURE ia32RtitMsrShadow = {0};

    ia32RtitMsrShadow.CycEn                 = FilterConfiguration->PacketGenerationOptions.PacketCyc.Enable;
    ia32RtitMsrShadow.OS                    = FilterConfiguration->FilteringOptions.FilterCpl.FilterKm;
    ia32RtitMsrShadow.User                  = FilterConfiguration->FilteringOptions.FilterCpl.FilterUm;
    ia32RtitMsrShadow.PwrEvtEn              = FilterConfiguration->PacketGenerationOptions.PacketPwr.Enable;
    ia32RtitMsrShadow.FUPonPTW              = FilterConfiguration->PacketGenerationOptions.Misc.EnableFupPacketsAfterPtw;
    ia32RtitMsrShadow.Cr3Filter             = FilterConfiguration->FilteringOptions.FilterCr3.Enable;
    ia32RtitMsrShadow.MtcEn                 = FilterConfiguration->PacketGenerationOptions.PackteMtc.Enable;
    ia32RtitMsrShadow.TscEn                 = FilterConfiguration->PacketGenerationOptions.PacketTsc.Enable;
    ia32RtitMsrShadow.DisRETC               = FilterConfiguration->PacketGenerationOptions.Misc.DisableRetCompression;
    ia32RtitMsrShadow.PTWEn                 = FilterConfiguration->PacketGenerationOptions.PacketPtw.Enable;
    ia32RtitMsrShadow.BranchEn              = FilterConfiguration->PacketGenerationOptions.PacketCofi.Enable;

    ia32RtitMsrShadow.MtcFreq               = FilterConfiguration->PacketGenerationOptions.PackteMtc.Frequency;
    ia32RtitMsrShadow.CycThresh             = FilterConfiguration->PacketGenerationOptions.PacketCyc.Frequency;
    ia32RtitMsrShadow.PSBFreq               = FilterConfiguration->PacketGenerationOptions.Misc.PsbFrequency;

    ia32RtitMsrShadow.Addr0Cfg              = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[0].RangeType;
    ia32RtitMsrShadow.Addr1Cfg              = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[1].RangeType;
    ia32RtitMsrShadow.Addr2Cfg              = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[2].RangeType;
    ia32RtitMsrShadow.Addr3Cfg              = FilterConfiguration->FilteringOptions.FilterRange.RangeOptions[3].RangeType;

    ia32RtitMsrShadow.InjectPsbPmiOnEnable  = FilterConfiguration->PacketGenerationOptions.Misc.InjectPsbPmiOnEnable;

    // TODO: Configure output

    return STATUS_SUCCESS;
}

NTSTATUS 
PtSetup(
    INTEL_PT_CONFIGURATION* FilterConfiguration
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

    status = PtConfigureProcessorTrace(
        FilterConfiguration
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;

}