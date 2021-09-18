#include "ProcessorTrace.h"

typedef enum CPUID_INDEX {
    CpuidEax,
    CpuidEbx,
    CpuidEcx,
    CpuidEdx
} CPUID_INDEX;

#define CPUID_INTELPT_AVAILABLE_LEAF 0x7
#define CPUID_INTELPT_AVAILABLE_SUBLEAF 0x0

#define CPUID_INTELPT_CAPABILITY_LEAF 0x14

#define BIT(x) (1<<x)

#define INTEL_PT_BIT 25
#define INTEL_PT_MASK BIT(INTEL_PT_BIT)
#define PT_FEATURE_AVAILABLE(v) (!!v)

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
        return STATUS_NOT_SUPPORTED;
    }

    if (cpuidIntelpt[CpuidEbx] & INTEL_PT_MASK)
    {
        Capabilities->IntelPtAvailable = TRUE;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability Available\n"));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability NOT Available\n"));
    }

    /// Gather future capabilities of Intel PT
    /// CPUID function 14H is dedicated to enumerate the resource and capability of processors that report CPUID.(EAX = 07H, ECX = 0H) : EBX[bit 25] = 1.

    __cpuidex(
        (int *)&Capabilities->IptCapabilities0,
        CPUID_INTELPT_CAPABILITY_LEAF,
        0x0
    );

    if (Capabilities->IptCapabilities0.Eax.MaximumValidSubleaf >= 1)
    {
        __cpuidex(
            (int*)&Capabilities->IptCapabilities1,
            CPUID_INTELPT_CAPABILITY_LEAF,
            0x1
        );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PtSetup(
)
{

    return STATUS_SUCCESS;
}
