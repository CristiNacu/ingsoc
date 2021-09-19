#ifndef _INTEL_PROCESSOR_TRACE_DEFS_H_
#define _INTEL_PROCESSOR_TRACE_DEFS_H_


/// Subleaf 0
typedef struct _CPUID_LEAF_14_SUBLEAF_0_EAX
{
    unsigned int MaximumValidSubleaf;                               // 31:0
} CPUID_LEAF_14_SUBLEAF_0_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EBX
{
    unsigned int Cr3FilteringSupport : 1;                           // 0
    unsigned int ConfigurablePsbAndCycleAccurateModeSupport : 1;    // 1
    unsigned int IpFilteringAndTraceStopSupport : 1;                // 2
    unsigned int MtcSupport : 1;                                    // 3
    unsigned int PtwriteSupport : 1;                                // 4
    unsigned int PowerEventTraceSupport : 1;                        // 5
    unsigned int PsbAndPmiPreservationSupport : 1;                  // 6
    unsigned int Reserved : 25;                                     // 31:7
} CPUID_LEAF_14_SUBLEAF_0_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_ECX
{
    unsigned int TopaOutputSupport : 1;                             // 0
    unsigned int TopaMultipleOutputEntriesSupport : 1;              // 1
    unsigned int SingleRangeOutputSupport : 1;                      // 2
    unsigned int OutputToTraceTransportSubsystemSupport : 1;        // 3
    unsigned int Reserved : 27;                                     // 30:4
    unsigned int IpPayloadsAreLip : 1;                              // 31
} CPUID_LEAF_14_SUBLEAF_0_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_0_EDX
{
    unsigned int Reserved;                                          // 31:0
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
    unsigned int NumberOfAddressRanges : 3;                                 // 2:0
    unsigned int Reserved : 13;                                             // 15:3    
    unsigned int BitmapOfSupportedMtcPeriodEncodings : 16;                  // 31:16
} CPUID_LEAF_14_SUBLEAF_1_EAX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EBX
{
    unsigned int BitmapOfSupportedCycleTresholdValues : 16;                 // 15:0
    unsigned int BitmapOfSupportedConfigurablePsbFrequencyEncoding : 16;    // 31:16
} CPUID_LEAF_14_SUBLEAF_1_EBX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_ECX
{
    unsigned int Reserved;                                                  // 31:0
} CPUID_LEAF_14_SUBLEAF_1_ECX;

typedef struct _CPUID_LEAF_14_SUBLEAF_1_EDX
{
    unsigned int Reserved;                                                  // 31:0
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
    unsigned long long    TopaOutputEntries;

} INTEL_PT_CAPABILITIES;


////////////////////////////////////////////////////// Filtering options
// Range filtering
typedef enum _INTEL_PT_RANGE_TYPE {

    RangeUnused,
    RangeFilterEn,
    RangeTraceStop,
    RangeMax

} INTEL_PT_RANGE_TYPE;

typedef struct _INTEL_PT_RANGE_OPTIONS {
    void* BaseAddress;
    unsigned long long AddressSize;
    INTEL_PT_RANGE_TYPE RangeType;
} INTEL_PT_RANGE_OPTIONS;

typedef struct _INTEL_PT_FILTERING_RANGE {
    unsigned short Enable;
    unsigned short NumberOfRanges;              // Will be checked to be <= CPUID_LEAF_14_SUBLEAF_1_EAX.NumberOfAddressRanges. 
    INTEL_PT_RANGE_OPTIONS RangeOptions[4];     // At most 4 address ranges are supported by any processor in production according to Intel
} INTEL_PT_FILTERING_RANGE;

// Cr3 filtering
typedef struct _INTEL_PT_FILTERING_CR3 {
    unsigned short Enable;
    void* Cr3Address;
} INTEL_PT_FILTERING_CR3;

// Cpl filtering
typedef struct _INTEL_PT_FILTERING_CPL {
    unsigned short FilterUm;
    unsigned short FilterKm;
} INTEL_PT_FILTERING_CPL;

// Aggregate all the filtering option structures
typedef struct _INTEL_PT_FILTERING_OPTIONS {
    INTEL_PT_FILTERING_CR3 FilterCr3;
    INTEL_PT_FILTERING_CPL FilterCpl;
    INTEL_PT_FILTERING_RANGE FilterRange;
} INTEL_PT_FILTERING_OPTIONS;

////////////////////////////////////////////////////// Packet generation
// CYC
typedef enum _INTEL_PT_PACKET_CYC_FREQUENCY {
    Freq0, Freq1, Freq2, Freq4,
    Freq8, Freq16, Freq32, Freq64,
    Freq128, Freq256, Freq512, Freq1024,
    Freq2048, Freq4096, Freq8192, Freq16384
} INTEL_PT_PACKET_CYC_FREQUENCY;

typedef struct _INTEL_PT_PACKET_CYC
{
    unsigned short Enable;
    INTEL_PT_PACKET_CYC_FREQUENCY Frequency;
} INTEL_PT_PACKET_CYC;


// PWR
typedef struct _INTEL_PT_PACKET_POWER_EVENTS
{
    unsigned short Enable;
} INTEL_PT_PACKET_POWER_EVENTS;


// MTC
typedef enum _INTEL_PT_PACKET_MTC_FREQUENCY {
    Art0, Art1, Art2, Art3,
    Art4, Art5, Art6, Art7,
    Art8, Art9, Art10, Art11,
    Art12, Art13, Art14, Art15
} INTEL_PT_PACKET_MTC_FREQUENCY;

typedef struct _INTEL_PT_PACKET_MTC
{
    unsigned short Enable;
    INTEL_PT_PACKET_MTC_FREQUENCY Frequency;
} INTEL_PT_PACKET_MTC;

// TSC
typedef struct _INTEL_PT_PACKET_TSC
{
    unsigned short Enable;
} INTEL_PT_PACKET_TSC;

// COFI -- should be enabled by default...
typedef struct _INTEL_PT_PACKET_COFI
{
    unsigned short Enable;
} INTEL_PT_PACKET_COFI;

// PTW
typedef struct _INTEL_PT_PACKET_PTW
{
    unsigned short Enable;
} INTEL_PT_PACKET_PTW;

// Other packet generation options
typedef enum _INTEL_PT_PACKET_PSB_FREQUENCY {
    Freq2K, Freq4K, Freq8K, Freq16K,
    Freq32K, Freq64K, Freq128K, Freq256K,
    Freq512K, Freq1M, Freq2M, Freq4M,
    Freq8M, Freq16M, Freq32M, Freq64M
} INTEL_PT_PACKET_PSB_FREQUENCY;

typedef struct _INTEL_PT_PACKET_MISC {
    unsigned short EnableFupPacketsAfterPtw;
    unsigned short DisableRetCompression;
    unsigned short InjectPsbPmiOnEnable;
    INTEL_PT_PACKET_PSB_FREQUENCY PsbFrequency;
} INTEL_PT_PACKET_MISC;

// Aggregate all the packet generation structures
typedef struct _INTEL_PT_PACKET_GENERATION_OPTIONS {
    INTEL_PT_PACKET_CYC PacketCyc;
    INTEL_PT_PACKET_POWER_EVENTS PacketPwr;
    INTEL_PT_PACKET_MTC PackteMtc;
    INTEL_PT_PACKET_TSC PacketTsc;
    INTEL_PT_PACKET_COFI PacketCofi;
    INTEL_PT_PACKET_PTW PacketPtw;
    INTEL_PT_PACKET_MISC Misc;
} INTEL_PT_PACKET_GENERATION_OPTIONS;

// Final structure for configuring IPT
typedef struct _INTEL_PT_CONFIGURATION {
    INTEL_PT_FILTERING_OPTIONS FilteringOptions;
    INTEL_PT_PACKET_GENERATION_OPTIONS PacketGenerationOptions;
} INTEL_PT_CONFIGURATION;







#endif