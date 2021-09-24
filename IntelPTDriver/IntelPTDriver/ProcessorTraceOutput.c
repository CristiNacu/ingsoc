#include "ProcessorTraceOutput.h"
#include "IntelProcessorTraceDefs.h"
#include "DriverUtils.h"

#define INTEL_PT_OUTPUT_TAG 'IPTO'

INTEL_PT_CAPABILITIES gCapabilities;

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
PtoInitTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Options)
        return STATUS_INVALID_PARAMETER_1;
    //if (!OutputConfiguration)
    //    return STATUS_INVALID_PARAMETER_2;

    if (gCapabilities.IptCapabilities0.Ecx.TopaOutputSupport)
    {
        if (gCapabilities.IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
        {
            Options->OutputType = PtOutputTypeToPA;
        }
        else
        {
            Options->OutputType = PtOutputTypeToPASingleRegion;
        }
    }
    else
    {
        // TODO: find a better way to report errors
        return STATUS_NOT_SUPPORTED;
    }

    if (Options->OutputType == PtOutputTypeToPA)
    {
        if (Options->TopaEntries > gCapabilities.TopaOutputEntries)
        {
            return STATUS_TOO_MANY_LINKS;
        }

        TOPA_TABLE *topaTable = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(TOPA_TABLE),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        topaTable->PhysicalAddresses = (PVOID)ExAllocatePoolWithTag(
            NonPagedPool,
            (Options->TopaEntries + 1) * sizeof(PVOID),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable->PhysicalAddresses)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        topaTable->TopaTableBaseVa = (PVOID)ExAllocatePoolWithTag(
            NonPagedPool,
            (Options->TopaEntries + 1) * sizeof(TOPA_ENTRY),
            INTEL_PT_OUTPUT_TAG
        );
        if (!topaTable->TopaTableBaseVa)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PHYSICAL_ADDRESS topaTablePa = MmGetPhysicalAddress(
            topaTable
        );

        topaTable->TopaTableBasePa = topaTablePa.QuadPart;
        
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
            topaTable->TopaTableBaseVa[i].OutputRegionBasePhysicalAddress = (ULONG64)bufferPa;
            if (i % frequency == 0)
                topaTable->TopaTableBaseVa[i].INT = TRUE;

        }

        topaTable->TopaTableBaseVa[Options->TopaEntries].END = TRUE;
        topaTable->TopaTableBaseVa[Options->TopaEntries].OutputRegionBasePhysicalAddress = topaTablePa.QuadPart;

        Options->TopaTable = topaTable;
        // Configure the trace to start at table 0 index 0 in the current ToPA
        Options->OutputMask.Raw = 0;
        Options->OutputBase = topaTablePa.QuadPart;

    }
    else if (Options->OutputType == PtOutputTypeSingleRegion)
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

        topaTable->TopaTableBaseVa[0].OutputRegionBasePhysicalAddress = (ULONG64)bufferPa;
        topaTable->TopaTableBaseVa[1].OutputRegionBasePhysicalAddress = topaTablePa.QuadPart;
        topaTable->TopaTableBaseVa[1].INT = TRUE;

        Options->TopaTable = topaTable;

        // Leave some space at the beginning of the buffer in case the interrupt doesn't fire exactly when the buffer is filled
        Options->OutputMask.Values.OutputOffset = PAGE_SIZE / 4;
        Options->OutputBase = topaTablePa.QuadPart;
    }
    else
    {
        status =  STATUS_UNDEFINED_SCOPE;
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
    TopaTable->TopaTableBaseVa[TableIndex].OutputRegionBasePhysicalAddress = (ULONG64)bufferPa;
    TopaTable->PhysicalAddresses[TableIndex] = bufferVa;

    return status;
}


NTSTATUS
PtoInitSingleRegionOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    UNREFERENCED_PARAMETER(Options);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
PtoInitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    if (Options->OutputType == PtOutputTypeSingleRegion)
    {
        return PtoInitTopaOutput(
            Options
        );
    }
    else if (Options->OutputType == PtOutputTypeToPA)
    {
        return PtoInitSingleRegionOutput(
            Options
        );
    }

    else
    {
        return STATUS_NOT_SUPPORTED;
    }
    
}

NTSTATUS
PtoUninitOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    UNREFERENCED_PARAMETER(Options);
    return STATUS_NOT_IMPLEMENTED;
}
