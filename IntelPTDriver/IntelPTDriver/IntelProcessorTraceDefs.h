#ifndef _INTEL_PROCESSOR_TRACE_DEFS_H_
#define _INTEL_PROCESSOR_TRACE_DEFS_H_


/// Subleaf 0
typedef struct _CPUID_LEAF_14_SUBLEAF_0_EAX
{
    unsigned int MaximumValidSubleaf;
} CPUID_LEAF_14_SUBLEAF_0_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EBX
{
    unsigned int Cr3FilteringSupport : 1;
    unsigned int ConfigurablePsbAndCycleAccurateModeSupport : 1;
    unsigned int IpFilteringAndTraceStopSupport : 1;
    unsigned int MtcSupport : 1;
    unsigned int PtwriteSupport : 1;
    unsigned int PowerEventTraceSupport : 1;
    unsigned int PsbAndPmiPreservationSupport : 1;
    unsigned int Reserved : 25;
} CPUID_LEAF_14_SUBLEAF_0_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_ECX
{
    unsigned int TopaOutputSupport : 1;
    unsigned int TopaMultipleOutputEntriesSupport : 1;
    unsigned int SingleRangeOutputSupport : 1;
    unsigned int OutputToTraceTransportSubsystemSupport : 1;
    unsigned int Reserved : 27;
    unsigned int IpPayloadsAreLip : 1;
} CPUID_LEAF_14_SUBLEAF_0_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EDX
{
    unsigned int Reserved;
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
    unsigned int NumberOfAddressRanges : 3;
    unsigned int Reserved : 13;
    unsigned int BitmapOfSupportedMtcPeriodEncodings : 16;
} CPUID_LEAF_14_SUBLEAF_1_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EBX
{
    unsigned int BitmapOfSupportedCycleTresholdValues : 16;
    unsigned int BitmapOfSupportedConfigurablePsbFrequencyEncoding : 16;
} CPUID_LEAF_14_SUBLEAF_1_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_ECX
{
    unsigned int Reserved;
} CPUID_LEAF_14_SUBLEAF_1_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EDX
{
    unsigned int Reserved;
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
    unsigned short IntelPtAvailable;
    CPUID_LEAF_14_SUBLEAF_0 IptCapabilities0;
    CPUID_LEAF_14_SUBLEAF_1 IptCapabilities1;

} INTEL_PT_CAPABILITIES;

#endif