#ifndef _PROCESSOR_TRACE_INTERFACE_H_
#define _PROCESSOR_TRACE_INTERFACE_H_
#include "DriverCommon.h"
#include "ProcessorTraceControlStructure.h"

NTSTATUS
PtoInit(
    INTEL_PT_CAPABILITIES* Capabilities
);

NTSTATUS
PtoSwapTopaBuffer(
    unsigned TableIndex,
    TOPA_TABLE* TopaTable,
    PVOID* OldBufferVa
);

NTSTATUS
PtoInitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
);

NTSTATUS
PtoUninitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
);



#endif