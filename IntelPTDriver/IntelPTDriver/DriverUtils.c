#include "DriverUtils.h"

NTSTATUS 
DuAllocateBuffer(
    size_t BufferSizeInBytes,
    MEMORY_CACHING_TYPE CachingType,
    BOOLEAN ZeroBuffer,
    PVOID* BufferVa,
    PVOID* BufferPa
)
{
    PHYSICAL_ADDRESS minAddr = { .QuadPart = 0x0000000001000000 };
    PHYSICAL_ADDRESS maxAddr = { .QuadPart = 0x0000001000000000 };
    PHYSICAL_ADDRESS zero = { .QuadPart = 0 };

    PVOID buffVa = MmAllocateContiguousMemorySpecifyCache(
        BufferSizeInBytes,
        minAddr,
        maxAddr,
        zero,
        CachingType
    );
    if (buffVa == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PHYSICAL_ADDRESS buffPa = MmGetPhysicalAddress(
        buffVa
    );

    if(ZeroBuffer)
        RtlFillMemory(buffVa, BufferSizeInBytes, 0);

    if (BufferVa)
        *BufferVa = buffVa;

    if (BufferPa)
        *BufferPa = (PVOID)buffPa.QuadPart;

    return STATUS_SUCCESS;
}

NTSTATUS
DuMapBufferInUserspace(
    PVOID BufferKernelAddress,
    size_t BufferSizeInBytes,
    PMDL* Mdl,
    PVOID *BufferUserAddress
)
{
    PMDL mdl = IoAllocateMdl(
        BufferKernelAddress,
        (ULONG)BufferSizeInBytes,
        FALSE,
        FALSE,
        NULL
    );
    if (mdl == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(mdl);

    PVOID buffVaU = MmMapLockedPagesSpecifyCache(
        mdl,
        UserMode,
        MmCached,
        NULL,
        FALSE,
        0
    );
    if (buffVaU == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Mdl)
        *Mdl = mdl;
    if (BufferUserAddress)
        *BufferUserAddress = buffVaU;
    return STATUS_SUCCESS;
}

NTSTATUS
DuFreeUserspaceMapping(
    PMDL* Mdl
)
{
    UNREFERENCED_PARAMETER(Mdl);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DuFreeBuffer(
    PVOID* BufferAddress,
    size_t BufferSizeInBytes,
    MEMORY_CACHING_TYPE CachingType
)
{
    UNREFERENCED_PARAMETER(BufferAddress);
    UNREFERENCED_PARAMETER(BufferSizeInBytes);
    UNREFERENCED_PARAMETER(CachingType);
    return STATUS_NOT_IMPLEMENTED;
}