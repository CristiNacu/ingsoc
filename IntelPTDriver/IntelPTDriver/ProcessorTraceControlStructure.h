#ifndef _PROCESSOR_TRACE_CONTROL_STRUCTURE_
#define _PROCESSOR_TRACE_CONTROL_STRUCTURE_
#include "DriverCommon.h"
#include "IntelProcessorTraceDefs.h"

typedef struct _TOPA_TABLE {
    PVOID* PhysicalAddresses;
    TOPA_ENTRY* TopaTableBaseVa;
    PVOID TopaTableBasePa;
} TOPA_TABLE;

typedef struct _PROCESSOR_TRACE_CONTROL_STRUCTURE{
    INTEL_PT_OUTPUT_TYPE OutputType;
    IA32_RTIT_CTL_STRUCTURE Control;
    PVOID OutputBase;
    IA32_RTIT_OUTPUT_MASK_STRUCTURE OutputMask;
    TOPA_TABLE *TopaTable;
} PROCESSOR_TRACE_CONTROL_STRUCTURE;


#endif