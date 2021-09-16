#include "ProcessorTrace.h"

typedef enum CPUID_INDEX {
    CpuidEax,
    CpuidEbx,
    CpuidEcx,
    CpuidEdx
} CPUID_INDEX;

#define CPUID_INTELPT_AVAILABLE_LEAF 0x7
#define CPUID_INTELPT_AVAILABLE_SUBLEAF 0x0

#define BIT(x) (1<<x)
#define INTEL_PT_BIT 25
#define INTEL_PT_MASK BIT(INTEL_PT_BIT)

NTSTATUS
PtQueryCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
)
{
    int cpuidIntelpt[4] = { 0 };
    __cpuidex(
        cpuidIntelpt,
        CPUID_INTELPT_AVAILABLE_LEAF,
        CPUID_INTELPT_AVAILABLE_SUBLEAF
    );
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CPUID(EAX:0x7 ECX:0x0).EBX = %X\n", cpuidIntelpt[CpuidEbx]));
    if (cpuidIntelpt[CpuidEbx] & INTEL_PT_MASK)
    {
        Capabilities->IntelPtAvailable = TRUE;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability Available\n"));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability NOT Available\n"));
    }

    if (Capabilities->IntelPtAvailable)
    {
        /// Gather future capabilities of Intel PT

        //__cpuidex(cpuidIntelpt, 0x14, 0x0);
        //cpuidIntelpt[]

    }




    return STATUS_SUCCESS;
}

NTSTATUS
PtSetup(
)
{

    return STATUS_SUCCESS;
}
