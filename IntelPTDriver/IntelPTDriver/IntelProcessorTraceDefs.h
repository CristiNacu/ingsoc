#ifndef _INTEL_PROCESSOR_TRACE_DEFS_H_
#define _INTEL_PROCESSOR_TRACE_DEFS_H_

// As described in Intel Manual Volume 4 Chapter 2 Table 2-2 pg 4630
typedef union _IA32_RTIT_CTL_STRUCTURE {

    struct {
        unsigned long long TraceEn : 1;        // 0
        unsigned long long CycEn : 1;        // 1  
        unsigned long long OS : 1;        // 2
        unsigned long long User : 1;        // 3
        unsigned long long PwrEvtEn : 1;        // 4
        unsigned long long FUPonPTW : 1;        // 5
        unsigned long long FabricEn : 1;        // 6
        unsigned long long Cr3Filter : 1;        // 7
        unsigned long long ToPA : 1;        // 8
        unsigned long long MtcEn : 1;        // 9
        unsigned long long TscEn : 1;        // 10
        unsigned long long DisRETC : 1;        // 11
        unsigned long long PTWEn : 1;        // 12
        unsigned long long BranchEn : 1;        // 13
        unsigned long long MtcFreq : 4;        // 17:14
        unsigned long long ReservedZero0 : 1;        // 18
        unsigned long long CycThresh : 4;        // 22:19
        unsigned long long ReservedZero1 : 1;        // 23
        unsigned long long PSBFreq : 4;        // 27:24
        unsigned long long ReservedZero2 : 4;        // 31:28
        unsigned long long Addr0Cfg : 4;        // 35:32
        unsigned long long Addr1Cfg : 4;        // 39:36
        unsigned long long Addr2Cfg : 4;        // 43:40
        unsigned long long Addr3Cfg : 4;        // 47:44
        unsigned long long ReservedZero3 : 8;        // 55:48
        unsigned long long InjectPsbPmiOnEnable : 1;        // 56
        unsigned long long ReservedZero4 : 7;        // 63:57
    } Fields;
    unsigned long long Raw;
} IA32_RTIT_CTL_STRUCTURE;

//////////////////////////////////////////////////// Output 

typedef enum _TOPA_ENTRY_SIZE_ENCODINGS {
    Size4K, Size8K, Size16K, Size32K,
    Size64K, Size128K, Size256K, Size512K,
    Size1M, Size2M, Size4M, Size8M,
    Size16M, Size32M, Size64M, Size128M
} TOPA_ENTRY_SIZE_ENCODINGS;

typedef struct _TOPA_ENTRY {
    unsigned long long END : 1;                                 // 0
    unsigned long long ReservedZero0 : 1;                       // 1
    unsigned long long INT : 1;                                 // 2
    unsigned long long ReservedZero1 : 1;                       // 3
    unsigned long long STOP : 1;                                // 4
    unsigned long long ReservedZero5 : 1;                       // 5
    unsigned long long Size : 4;                                // 9:6
    unsigned long long ReservedZero6 : 2;                       // 11:10
    unsigned long long OutputRegionBasePhysicalAddress : 52;    // MAXPHYADDR-1:12
} TOPA_ENTRY;

typedef union _IA32_RTIT_OUTPUT_MASK_STRUCTURE {
    struct {
        // REMINDER: Lowest mask available is 128 -> last 7 bits are ALWAYS 1
        unsigned long long MaskOrTableOffset : 32;      // 31:0
        unsigned long long OutputOffset : 32;           // 64:32
    } Fields;
    unsigned long long Raw;
} IA32_RTIT_OUTPUT_MASK_STRUCTURE;

typedef enum _INTEL_PT_OUTPUT_TYPE {
    PtOutputTypeSingleRegion,
    PtOutputTypeToPA,
    PtOutputTypeToPASingleRegion,
    PtOutputTypeTraceTransportSubsystem,    // Unsupported?
} INTEL_PT_OUTPUT_TYPE;

typedef struct _TOPA_TABLE {
    unsigned CurrentBufferOffset;
    unsigned NumberOfTopaEntries;
    PVOID* VirtualTopaPagesAddresses;
    unsigned long long TopaTableBasePa;
    TOPA_ENTRY* TopaTableBaseVa;
} TOPA_TABLE;

typedef struct _INTEL_PT_OUTPUT_OPTIONS {
    INTEL_PT_OUTPUT_TYPE OutputType;
    unsigned TopaEntries;
    unsigned EntrySize;
    unsigned long long OutputBase;
    IA32_RTIT_OUTPUT_MASK_STRUCTURE OutputMask;
    TOPA_TABLE* TopaTable;
} INTEL_PT_OUTPUT_OPTIONS;

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
    } Fields;
    unsigned long long Raw;
} IA32_RTIT_STATUS_STRUCTURE;


typedef enum CPUID_INDEX {
    CpuidEax,
    CpuidEbx,
    CpuidEcx,
    CpuidEdx
} CPUID_INDEX;


typedef union _PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE {

    struct {

        unsigned long Vector : 8;
        unsigned long DeliveryMode : 3;
        unsigned long Reserved0 : 1;
        unsigned long DeliveryStatus : 1;
        unsigned long Reserved1 : 3;
        unsigned long Mask : 1;
        unsigned long Raw : 15;

    } Values;

    unsigned long Raw;

} PERFORMANCE_MONITOR_COUNTER_LVT_STRUCTURE;

typedef union _IA32_PERF_GLOBAL_STATUS_STRUCTURE {

    struct {
        unsigned long long OvfPMC0 : 1; // 0
        unsigned long long OvfPMC1 : 1; // 1
        unsigned long long OvfPMC2 : 1; // 2
        unsigned long long OvfPMC3 : 1; // 3
        unsigned long long Reserved0 : 28; // 31:4
        unsigned long long OvfFixedCtr0 : 1; // 32
        unsigned long long OvfFixedCtr1 : 1; // 33
        unsigned long long OvfFixedCtr2 : 1; // 34
        unsigned long long Reserved1 : 13; // 47:35
        unsigned long long OvfPerfMetrics : 1; // 48
        unsigned long long Reserved2 : 6; // 54:49
        unsigned long long TopaPMI : 1; // 55
        unsigned long long Reserved3 : 2; // 57:56
        unsigned long long LbrFrz : 1; // 58
        unsigned long long CrtFrz : 1; // 59
        unsigned long long Asci : 1; // 60
        unsigned long long OvfUncore : 1; // 61
        unsigned long long OvfBuf : 1; // 62
        unsigned long long CondChgd : 1; // 63
    } Values;
    unsigned long long Raw;

} IA32_PERF_GLOBAL_STATUS_STRUCTURE;

typedef union _IA32_APIC_BASE_STRUCTURE {

    struct {
        unsigned long long Reserved0 : 8;
        unsigned long long BSPFlag : 1;
        unsigned long long Reserved1 : 1;
        unsigned long long EnableX2ApicMode : 1;
        unsigned long long ApicGlobalEnbale : 1;
        unsigned long long ApicBase : 52;
    } Values;
    unsigned long long Raw;
} IA32_APIC_BASE_STRUCTURE;

#define PT_POOL_TAG                         'PTPT'
#define INTEL_PT_OUTPUT_TAG                 'IPTO'

#define PT_OPTION_IS_SUPPORTED(capability, option)  (((!capability) && (option)) ? FALSE : TRUE)

#define PT_OUTPUT_CONTIGNUOUS_BASE_MASK             0x7F
#define PT_OUTPUT_TOPA_BASE_MASK                    0xFFF

#define CPUID_INTELPT_AVAILABLE_LEAF        0x7
#define CPUID_INTELPT_AVAILABLE_SUBLEAF     0x0

#define CPUID_INTELPT_CAPABILITY_LEAF       0x14

#define BIT(x)                              (1<<x)

#define INTEL_PT_BIT                        25
#define INTEL_PT_MASK                       BIT(INTEL_PT_BIT)
#define PT_FEATURE_AVAILABLE(v)             ((v) != 0)


#define IA32_RTIT_OUTPUT_BASE               0x560
#define IA32_RTIT_OUTPUT_MASK_PTRS          0x561
#define IA32_RTIT_CTL                       0x570
#define IA32_RTIT_CR3_MATCH                 0x572

#define IA32_RTIT_ADDR0_A                   0x580
#define IA32_RTIT_ADDR0_B                   0x581
#define IA32_RTIT_ADDR1_A                   0x582
#define IA32_RTIT_ADDR1_B                   0x583
#define IA32_RTIT_ADDR2_A                   0x584
#define IA32_RTIT_ADDR2_B                   0x585
#define IA32_RTIT_ADDR3_A                   0x586
#define IA32_RTIT_ADDR3_B                   0x587

#define IA32_APIC_BASE                      0x1B
#define IA32_LVT_REGISTER                   0x834
#define MSR_IA32_PERF_GLOBAL_STATUS		    0x0000038e
#define MSR_IA32_PERF_GLOBAL_OVF_CTRL       0x00000390

#define IA32_RTIT_STATUS                    0x571
#define PtGetStatus(ia32_rtit_ctl_structure) (ia32_rtit_ctl_structure.Raw = __readmsr(IA32_RTIT_STATUS))
#define PtSetStatus(value) __writemsr(IA32_RTIT_STATUS, value)

INTEL_PT_CAPABILITIES*  gPtCapabilities = NULL;
unsigned long long      gFrequency = 3;

#endif