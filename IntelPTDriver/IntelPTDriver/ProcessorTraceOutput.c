#include "ProcessorTraceOutput.h"
#include "IntelProcessorTraceDefs.h"
#include "DriverUtils.h"


// 
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

#define INTEL_PT_OUTPUT_TAG 'IPTO'

typedef struct _TOPA_TABLE {

    PVOID *PhysicalAddresses;
    TOPA_ENTRY Entries[];

} TOPA_TABLE;

typedef struct _OUTPUT_CONTROL_STRUCTURE {

    INTEL_PT_OUTPUT_TYPE OutputType;
    void* OutputBaseAddress;
    IA32_RTIT_OUTPUT_MASK_STRUCTURE OutputMaskAddress;
    
} OUTPUT_CONTROL_STRUCTURE;


INTEL_PT_CAPABILITIES gCapabilities;
OUTPUT_CONTROL_STRUCTURE gOutputConfiguration;

NTSTATUS
PtoInit(
    INTEL_PT_CAPABILITIES* Capabilities
)
{

    memcpy_s(
        &gCapabilities,
        sizeof(gCapabilities),
        Capabilities,
        sizeof(INTEL_PT_CAPABILITIES)
    );

    return STATUS_SUCCESS;

}

NTSTATUS
PtoCreateTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options,
    unsigned long long *TopaBaseAddress
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (gCapabilities.IptCapabilities0.Ecx.TopaOutputSupport)
    {
        if (gCapabilities.IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
        {
            gOutputConfiguration.OutputType = PtOutputTypeToPA;
        }
        else
        {
            gOutputConfiguration.OutputType = PtOutputTypeToPASingleRegion;
        }
    }

    if (gOutputConfiguration.OutputType == PtOutputTypeToPA)
    {
        if (Options->TopaEntries > gCapabilities.TopaOutputEntries)
        {
            return STATUS_TOO_MANY_LINKS;
        }

        TOPA_TABLE *topaTable = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(TOPA_TABLE) + (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        topaTable->PhysicalAddresses = (PVOID *)ExAllocatePoolWithTag(
            NonPagedPool,
            (Options->TopaEntries) * sizeof(PVOID),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable->PhysicalAddresses)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PHYSICAL_ADDRESS topaTablePa = MmGetPhysicalAddress(
            topaTable
        );
        
        unsigned frequency = 4;

        for (unsigned i = 0; i < Options->TopaEntries; i++)
        {
            PVOID bufferVa;
            PVOID bufferPa;

            status = DuAllocateBuffer(
                PAGE_SIZE,
                MmCached,
                TRUE,
                &bufferVa,
                &bufferPa
            );
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            topaTable->PhysicalAddresses[i] = bufferVa;
            topaTable->Entries[i].OutputRegionBasePhysicalAddress = bufferPa;
            if (i % frequency == 0)
                topaTable->Entries[i].INT = TRUE;

        }

        topaTable->Entries[Options->TopaEntries].END = TRUE;
        topaTable->Entries[Options->TopaEntries].OutputRegionBasePhysicalAddress = topaTablePa.QuadPart;
        *TopaBaseAddress = topaTable;
    }
    else if (gOutputConfiguration.OutputType == PtOutputTypeSingleRegion)
    {
        PVOID bufferVa;
        PVOID bufferPa;

        TOPA_TABLE* topaTable = ExAllocatePoolWithTag(
            NonPagedPool,
            2 * sizeof(TOPA_ENTRY),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PHYSICAL_ADDRESS topaTablePa = MmGetPhysicalAddress(
            topaTable
        );


        status = DuAllocateBuffer(
            PAGE_SIZE,
            MmCached,
            TRUE,
            &bufferVa,
            &bufferPa
        );
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        topaTable->Entries[0].OutputRegionBasePhysicalAddress = bufferPa;
        topaTable->Entries[1].OutputRegionBasePhysicalAddress = topaTablePa.QuadPart;
        topaTable->Entries[1].INT = TRUE;

    }
    else
    {
        return STATUS_UNDEFINED_SCOPE;
    }

    return status;
}

NTSTATUS
PtoSwapTopaBuffer(
    unsigned TableIndex,
    TOPA_TABLE *TopaTable,
    PVOID* OldBufferVa
)
{
    NTSTATUS status;
    PVOID bufferVa;
    PVOID bufferPa;
    
    if (!OldBufferVa)
        return STATUS_INVALID_PARAMETER_3;

    status = DuAllocateBuffer(
        PAGE_SIZE,
        MmCached,
        TRUE,
        &bufferVa,
        &bufferPa
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *OldBufferVa = TopaTable->PhysicalAddresses[TableIndex];
    TopaTable->Entries[TableIndex].OutputRegionBasePhysicalAddress = bufferPa;
    TopaTable->PhysicalAddresses[TableIndex] = bufferVa;

    return status;
}


NTSTATUS
PtoCreateSingleRegionOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{

}


NTSTATUS
PtoCreateOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    if (Options->OutputType == PtOutputTypeSingleRegion)
    {

    }
    else if (Options->OutputType == PtOutputTypeToPA)
    {

    }

    else
    {
        return STATUS_NOT_SUPPORTED;
    }

}
