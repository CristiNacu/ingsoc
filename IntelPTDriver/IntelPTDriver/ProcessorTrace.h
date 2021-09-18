#ifndef _PROCESSOR_TRACE_H_
#define _PROCESSOR_TRACE_H_
#include "DriverCommon.h"

/// Subleaf 0
typedef struct _CPUID_LEAF_14_SUBLEAF_0_EAX
{
    UINT32 MaximumValidSubleaf;
} CPUID_LEAF_14_SUBLEAF_0_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EBX
{
    UINT32 Cr3FilteringSupport : 1;
    UINT32 ConfigurablePsbAndCycleAccurateModeSupport : 1;
    UINT32 IpFilteringAndTraceStopSupport : 1;
    UINT32 MtcSupport : 1;
    UINT32 PtwriteSupport : 1;
    UINT32 PowerEventTraceSupport : 1;
    UINT32 PsbAndPmiPreservationSupport : 1;
    UINT32 Reserved : 25;
} CPUID_LEAF_14_SUBLEAF_0_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_ECX
{
    UINT32 TopaOutputSupport : 1;
    UINT32 TopaMultipleOutputEntriesSupport : 1;
    UINT32 SingleRangeOutputSupport : 1;
    UINT32 OutputToTraceTransportSubsystemSupport : 1;
    UINT32 Reserved : 27;
    UINT32 IpPayloadsAreLip : 1;
} CPUID_LEAF_14_SUBLEAF_0_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EDX
{
    UINT32 Reserved;
} CPUID_LEAF_14_SUBLEAF_0_EDX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0
{
    CPUID_LEAF_14_SUBLEAF_0_EAX Eax;
    CPUID_LEAF_14_SUBLEAF_0_EBX Ebx;
    CPUID_LEAF_14_SUBLEAF_0_ECX Ecx;
    CPUID_LEAF_14_SUBLEAF_0_EDX Edx;
} CPUID_LEAF_14_SUBLEAF_0;

//ASSERT(sizeof(int[4]) == sizeof(CPUID_LEAF_14_SUBLEAF_0));


///  Subleaf 1
typedef struct _CPUID_LEAF_14_SUBLEAF_1_EAX
{
    UINT32 NumberOfAddressRanges : 3;
    UINT32 Reserved : 13;
    UINT32 BitmapOfSupportedMtcPeriodEncodings : 16;
} CPUID_LEAF_14_SUBLEAF_1_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EBX
{
    UINT32 BitmapOfSupportedCycleTresholdValues : 16;
    UINT32 BitmapOfSupportedConfigurablePsbFrequencyEncoding : 16;
} CPUID_LEAF_14_SUBLEAF_1_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_ECX
{
    UINT32 Reserved;
} CPUID_LEAF_14_SUBLEAF_1_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EDX
{
    UINT32 Reserved;
} CPUID_LEAF_14_SUBLEAF_1_EDX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1
{
    CPUID_LEAF_14_SUBLEAF_1_EAX Eax;
    CPUID_LEAF_14_SUBLEAF_1_EBX Ebx;
    CPUID_LEAF_14_SUBLEAF_1_ECX Ecx;
    CPUID_LEAF_14_SUBLEAF_1_EDX Edx;
} CPUID_LEAF_14_SUBLEAF_1;

//ASSERT(sizeof(int[4]) == sizeof(CPUID_LEAF_14_SUBLEAF_1));


typedef struct _INTEL_PT_CAPABILITIES
{
    BOOLEAN IntelPtAvailable;
    CPUID_LEAF_14_SUBLEAF_0 IptCapabilities0;
    CPUID_LEAF_14_SUBLEAF_1 IptCapabilities1;

} INTEL_PT_CAPABILITIES;


NTSTATUS
PtQueryCapabilities(
    INTEL_PT_CAPABILITIES* Capabilities
);

NTSTATUS
PtSetup(

);


#endif // !_PROCESSOR_TRACE_H_