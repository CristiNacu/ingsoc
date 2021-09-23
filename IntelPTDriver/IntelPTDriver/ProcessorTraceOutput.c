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
PtoCreateTopaOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options,
    PROCESSOR_TRACE_CONTROL_STRUCTURE* OutputConfiguration
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Options)
        return STATUS_INVALID_PARAMETER_1;
    if (!OutputConfiguration)
        return STATUS_INVALID_PARAMETER_2;

    if (gCapabilities.IptCapabilities0.Ecx.TopaOutputSupport)
    {
        if (gCapabilities.IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport)
        {
            OutputConfiguration->OutputType = PtOutputTypeToPA;
        }
        else
        {
            OutputConfiguration->OutputType = PtOutputTypeToPASingleRegion;
        }
    }
    else
    {
        // TODO: find a better way to report errors
        return STATUS_NOT_SUPPORTED;
    }

    if (OutputConfiguration->OutputType == PtOutputTypeToPA)
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

        topaTable->TopaTableBasePa = (PVOID)topaTablePa.QuadPart;
        
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

        OutputConfiguration->TopaTable = topaTable;
        // Configure the trace to start at table 0 index 0 in the current ToPA
        OutputConfiguration->OutputMask.Raw = 0;
        OutputConfiguration->OutputBase = (PVOID)topaTablePa.QuadPart;

    }
    else if (OutputConfiguration->OutputType == PtOutputTypeSingleRegion)
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

        OutputConfiguration->TopaTable = topaTable;

        // Leave some space at the beginning of the buffer in case the interrupt doesn't fire exactly when the buffer is filled
        OutputConfiguration->OutputMask.Values.OutputOffset = PAGE_SIZE / 4;
        OutputConfiguration->OutputBase = (PVOID)topaTablePa.QuadPart;
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
PtoCreateSingleRegionOutput(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    UNREFERENCED_PARAMETER(Options);
    return STATUS_SUCCESS;
}


NTSTATUS
PtoCreateOutputStructure(
    INTEL_PT_OUTPUT_OPTIONS* Options
)
{
    PROCESSOR_TRACE_CONTROL_STRUCTURE outputConfiguration;

    if (Options->OutputType == PtOutputTypeSingleRegion)
    {
        return PtoCreateTopaOutput(
            Options,
            &outputConfiguration
        );
    }
    else if (Options->OutputType == PtOutputTypeToPA)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    else
    {
        return STATUS_NOT_SUPPORTED;
    }
    
}
