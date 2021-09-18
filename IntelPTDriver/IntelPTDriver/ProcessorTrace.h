#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"
#include "IntelProcessorTraceDefs.h"

NTSTATUS
PtInit(
);

NTSTATUS
PTGetCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
);

NTSTATUS
PtSetup(

);


#endif // !_PROCESSOR_TRACE_H_