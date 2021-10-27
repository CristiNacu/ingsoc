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
);

NTSTATUS
IptDisableTrace(
);

NTSTATUS
IptPauseTrace(
);

NTSTATUS
IptResumeTrace(
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