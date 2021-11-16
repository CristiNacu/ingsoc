#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"
#include "ProcessorTraceShared.h"
#include "DriverUtils.h"


NTSTATUS
IptInit(
    IptMemoryAllocationFunction MemoryAllocationFunction
);

NTSTATUS
IptInitPerCore(
    INTEL_PT_CONTROL_STRUCTURE* ControlStructure
);

NTSTATUS
IptSetup(
    INTEL_PT_CONFIGURATION* FilterConfiguration,
    INTEL_PT_CONTROL_STRUCTURE* ControlStructure
);

NTSTATUS
IptEnableTrace(
    INTEL_PT_OUTPUT_OPTIONS* cpuOptions
);

NTSTATUS
IptDisableTrace(
    INTEL_PT_OUTPUT_OPTIONS* cpuOptions
);

NTSTATUS
IptPauseTrace(
    INTEL_PT_OUTPUT_OPTIONS* cpuOptions
);

NTSTATUS
IptResumeTrace(
    INTEL_PT_OUTPUT_OPTIONS* cpuOptions
);

NTSTATUS
IptUnlinkFullTopaBuffers(

);

BOOLEAN
IptError(

);

BOOLEAN
IptEnabled(
);

void
IptUninit(
);

void
IptClearError(
);

NTSTATUS
IptGetCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
);

BOOLEAN
IptTopaPmiReason(
);

void
IptResetPmi(
);

//NTSTATUS
//PtoSwapTopaBuffer(
//    unsigned TableIndex,
//    TOPA_TABLE* TopaTable,
//    PVOID* OldBufferVa
//);

#endif // !_PROCESSOR_TRACE_H_