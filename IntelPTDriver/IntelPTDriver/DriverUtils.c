#include "DriverUtils.h"

NTSTATUS
DuAllocatePtBuffer(
    size_t BufferSizeInBytes,
    MEMORY_CACHING_TYPE CachingType,
    PVOID *BufferVaKm,
    PVOID *BufferVaUm,
    PVOID *BufferPa
)
{
    PHYSICAL_ADDRESS minAddr = { .QuadPart = 0x0000000001000000 };
    PHYSICAL_ADDRESS maxAddr = { .QuadPart = 0x0000001000000000 };
    PHYSICAL_ADDRESS zero = { .QuadPart = 0 };

    PVOID buffVaK = MmAllocateContiguousMemorySpecifyCache(
        BufferSizeInBytes,
        minAddr,
        maxAddr,
        zero,
        CachingType
    );
    if (buffVaK == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    PHYSICAL_ADDRESS buffPa = MmGetPhysicalAddress(
        buffVaK
    );

    PMDL mdl = IoAllocateMdl(
        buffVaK,
        PAGE_SIZE,
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

    RtlFillMemory(buffVaK, PAGE_SIZE, 0);

    if (BufferVaKm)
        *BufferVaKm = buffVaK;
    if (BufferPa)
        *BufferPa = (PVOID)buffPa.QuadPart;
    if (BufferVaUm)
        *BufferVaUm = buffVaU;

    return STATUS_SUCCESS;

}