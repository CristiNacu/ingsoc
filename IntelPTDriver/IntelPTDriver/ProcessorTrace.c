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
#define PT_FEATURE_AVAILABLE(v) ((v) != 0)

INTEL_PT_CAPABILITIES *gPtCapabilities = NULL;
#define PT_POOL_TAG 'PTPT'

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
PtSetup(
)
{

    return STATUS_SUCCESS;
}
