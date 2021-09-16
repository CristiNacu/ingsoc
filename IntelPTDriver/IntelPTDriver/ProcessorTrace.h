#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"

typedef struct _INTEL_PT_CAPABILITIES
{
    UINT32 IntelPtAvailable : 1;
    UINT32 Cr3FilteringSupport : 1;
    UINT32 ConfigurablePsbAndCycleAccurateModeSupport : 1;
    UINT32 IpFilteringAndTraceStopSupport : 1;
    UINT32 MtcSupport : 1;
    UINT32 PtwriteSupport : 1;
    UINT32 PowerEventTraceSupport : 1;
    UINT32 PsbAndPmiPreservationSupport : 1;
    UINT32 TopaOutputSupport : 1;
    UINT32 TopaMultipleOutputSupport : 1;
} INTEL_PT_CAPABILITIES;


NTSTATUS
PtQueryCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
);

NTSTATUS
PtSetup(

);


#endif // !_PROCESSOR_TRACE_H_