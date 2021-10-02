#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"
#include "IntelProcessorTraceDefs.h"

#define IA32_RTIT_STATUS                    0x571
#define PtGetStatus(ia32_rtit_ctl_structure) (ia32_rtit_ctl_structure.Raw = __readmsr(IA32_RTIT_STATUS))


// As described in Intel Manual Volume 4 Chapter 2 Table 2-2 pg 4630
typedef union _IA32_RTIT_STATUS_STRUCTURE {
    struct {
        unsigned long long FilterEn : 1;        // 0
        unsigned long long ContextEn : 1;        // 1  
        unsigned long long TriggerEn : 1;        // 2
        unsigned long long Reserved0 : 1;        // 3
        unsigned long long Error : 1;        // 4
        unsigned long long Stopped : 1;        // 5
        unsigned long long SendPSB : 1;        // 6
        unsigned long long PendToPAPMI : 1;        // 7
        unsigned long long ReservedZero0 : 24;       // 31:8
        unsigned long long PacketByteCnt : 17;       // 48:32
        unsigned long long Reserved1 : 15;       // 10
    } Values;
    unsigned long long Raw;
} IA32_RTIT_STATUS_STRUCTURE;

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


NTSTATUS
PtoSwapTopaBuffer(
    unsigned TableIndex,
    TOPA_TABLE* TopaTable,
    PVOID* OldBufferVa
);

#endif // !_PROCESSOR_TRACE_H_