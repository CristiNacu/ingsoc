#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"
#include "IntelProcessorTraceDefs.h"

NTSTATUS
PtInit(
);

NTSTATUS
PtGetCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
);

NTSTATUS
PtSetup(
    INTEL_PT_CONFIGURATION* FilterConfiguration
);

NTSTATUS
PtEnableTrace(
);

NTSTATUS
PtDisableTrace(
);

#endif // !_PROCESSOR_TRACE_H_