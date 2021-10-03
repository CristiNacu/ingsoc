#include "DriverUtils.h"

#define PT_QUEUE_TAG 'PTQT'

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


typedef struct _INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE {

    LIST_ENTRY *ListEntry;
    PVOID Data;

} INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE;

typedef struct _QUEUE_INTERLOCKED_HED_STRUCT {
    QUEUE_HEAD_STRUCTURE QueueHead;
    KSPIN_LOCK QueueLock;
} QUEUE_INTERLOCKED_HED_STRUCT;

VOID
DuQueueUninit(
    QUEUE_HEAD_STRUCTURE* QueueHead
)
{
    if (!QueueHead)
        return;

    if (QueueHead->ListEntry)
        ExFreePoolWithTag(
            QueueHead->ListEntry,
            PT_QUEUE_TAG
        );

    ExFreePoolWithTag(
        QueueHead,
        PT_QUEUE_TAG
    );

}

NTSTATUS 
DuQueueInit(
    QUEUE_HEAD_STRUCTURE **QueueHead,
    BOOLEAN Interlocked
)
{
    if (!QueueHead)
        return STATUS_INVALID_PARAMETER_1;

    NTSTATUS status = STATUS_SUCCESS;

    QUEUE_HEAD_STRUCTURE* queueHead = ExAllocatePoolWithTag(
        PagedPool,
        Interlocked? sizeof(QUEUE_INTERLOCKED_HED_STRUCT) : sizeof(QUEUE_HEAD_STRUCTURE),
        PT_QUEUE_TAG
    );
    if (!queueHead)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    queueHead->ListEntry = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(LIST_ENTRY),
        PT_QUEUE_TAG
    );
    if (!queueHead->ListEntry)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    queueHead->Interlocked = Interlocked;

    InitializeListHead(
        queueHead->ListEntry
    );
    
    if (Interlocked)
        KeInitializeSpinLock(
            &((QUEUE_INTERLOCKED_HED_STRUCT*)queueHead)->QueueLock
        );

cleanup:
    if (!NT_SUCCESS(status))
    {
        DuQueueUninit(
            queueHead
        );

        *QueueHead = NULL;
    }
    else
    {
        *QueueHead = queueHead;
    }

    return status;
}

NTSTATUS
DuEnqueueElement(
    QUEUE_HEAD_STRUCTURE *QueueHead,
    PVOID Data
)
{
    if(!QueueHead)
        return STATUS_INVALID_PARAMETER_1;

    INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE* element = ExAllocatePoolWithTag(
        KeGetCurrentIrql() == DISPATCH_LEVEL ? NonPagedPool : PagedPool,
        sizeof(INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE),
        PT_QUEUE_TAG
    );
    if (!element)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    element->Data = Data;

    if (!QueueHead->Interlocked)
    {
        InsertTailList(
            QueueHead->ListEntry,
            (PLIST_ENTRY)element
        );
    }
    else {
        ExInterlockedInsertTailList(
            QueueHead->ListEntry,
            (PLIST_ENTRY)element,
            &((QUEUE_INTERLOCKED_HED_STRUCT*)QueueHead)->QueueLock
        );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DuDequeueElement(
    QUEUE_HEAD_STRUCTURE* QueueHead,
    PVOID* Data
)
{
    if (!QueueHead)
        return STATUS_INVALID_PARAMETER_1;
    if (!Data)
        return STATUS_INVALID_PARAMETER_2;

    PLIST_ENTRY element;
    if (!QueueHead->Interlocked)
    {
        element = RemoveHeadList(
            QueueHead->ListEntry
        );
    }
    else
    {
        element = ExInterlockedRemoveHeadList(
            QueueHead->ListEntry,
            &((QUEUE_INTERLOCKED_HED_STRUCT*)QueueHead)->QueueLock
        );
    }

    if (element == NULL || element == QueueHead->ListEntry)
    {
        *Data = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE* queueElement =
        (INTERNAL_QUEUE_WORK_ELEMENT_STRUCTURE*) element;

    *Data = queueElement->Data;

    ExFreePoolWithTag(
        queueElement,
        PT_QUEUE_TAG
    );

    return STATUS_SUCCESS;
}

NTSTATUS
DuEnqueueElements(
    QUEUE_HEAD_STRUCTURE* QueueHead,
    unsigned NumberOfElements,
    PVOID Elements[]
)
{
    NTSTATUS status = STATUS_SUCCESS;
    for (unsigned i = 0; i < NumberOfElements; i++)
    {
        status = DuEnqueueElement(
            QueueHead,
            Elements[i]
        );
        if (!NT_SUCCESS(status))
        {
            DEBUG_PRINT("DuEnqueueElement returned status %X\n", status);
            return status;
        }
    }

    return status;
}