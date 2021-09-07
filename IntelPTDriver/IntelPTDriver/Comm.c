#include "Comm.h"
#include "Debug.h"
VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}

NTSTATUS
InitCommQueue(
    _In_ WDFDEVICE                   *ControlDevice,
    _In_ COMM_QUEUE_DEVICE_CONTEXT   *Data,
    _In_ IO_QUEUE_SETTINGS           *IoQueueSettings
)
{
    NTSTATUS                        status;
    PCOMM_QUEUE_DEVICE_CONTEXT      devContext = NULL;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };
    WDFQUEUE                        queue = { 0 };

    DEBUG_PRINT("InitCommQueue\n");

    DEBUG_STOP();
    DEBUG_PRINT("Device Addr %p\n", ControlDevice);
    DEBUG_STOP();
    DEBUG_PRINT("Device %p\n", (*ControlDevice));
    DEBUG_STOP();
    DEBUG_PRINT("Get context:\n");

    devContext = CommGetContextFromDevice(*ControlDevice);
    if (devContext == NULL)
    {
        DEBUG_PRINT("CommGetContextFromDevice context null\n");
        return STATUS_INVALID_DEVICE_STATE;
    }


    devContext->Data = Data;
    devContext->Sequence = 0;
    devContext->NotificationQueue = NULL;
    DEBUG_STOP();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

    DEBUG_STOP();

    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests = ((ULONG)-1);

    ioQueueConfig.EvtIoDefault = IoQueueSettings->IoQueueIoDefault;
    ioQueueConfig.EvtIoRead = IoQueueSettings->IoQueueIoRead;
    ioQueueConfig.EvtIoWrite = IoQueueSettings->IoQueueIoWrite;
    ioQueueConfig.EvtIoStop = IoQueueSettings->IoQueueIoStop;
    ioQueueConfig.EvtIoResume = IoQueueSettings->IoQueueIoResume;
    ioQueueConfig.EvtIoCanceledOnQueue = IoQueueSettings->IoQueueIoCanceledOnQueue;

    ioQueueConfig.EvtIoDeviceControl = IoQueueSettings->IoQueueIoDeviceControl;
    ioQueueConfig.EvtIoInternalDeviceControl = IoQueueSettings->IoQueueIoInternalDeviceControl;

    ioQueueConfig.PowerManaged = WdfFalse;

    DEBUG_STOP();

    status = WdfIoQueueCreate(*ControlDevice, &ioQueueConfig, NULL, &queue);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
        return status;
    }

    DEBUG_STOP();

    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

    DEBUG_STOP();


    status = WdfIoQueueCreate(*ControlDevice, &ioQueueConfig, NULL, &devContext->NotificationQueue);
    if (!NT_SUCCESS(status))
    {
        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
        return status;
    }
    DEBUG_STOP();

    WdfControlFinishInitializing(*ControlDevice);
    DEBUG_STOP();

    return status;
}

/*
VOID
CommEvtIoRead(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t      Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Length);
    DbgBreakPoint();
}

VOID
CommEvtIoWrite(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Length);
    DbgBreakPoint();
}

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}

NTSTATUS
QueueInitialize(
    WDFDRIVER Driver
)
{
    NTSTATUS                        status;
    WDFQUEUE                        queue = { 0 };
    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };
    WDF_FILEOBJECT_CONFIG           fileConfig = { 0 };
    WDF_OBJECT_ATTRIBUTES           objAttributes;
    WDFDEVICE                       controlDevice = NULL;
    PCOMM_QUEUE_DEVICE_CONTEXT      devContext = NULL;
    PWDFDEVICE_INIT                 deviceInit = NULL;
    UNICODE_STRING                  deviceName = { 0 };


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objAttributes, COMM_QUEUE_DEVICE_CONTEXT);

    objAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    objAttributes.EvtCleanupCallback = CommIngonreOperation;
    objAttributes.EvtDestroyCallback = CommIngonreOperation;

    deviceInit = WdfControlDeviceInitAllocate(Driver, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
    if (deviceInit == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&deviceName, Data->Configuration.NativeDeviceName);
    status = WdfDeviceInitAssignName(deviceInit, &deviceName);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    status = WdfDeviceInitAssignSDDLString(deviceInit, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    WdfDeviceInitSetCharacteristics(deviceInit, (FILE_DEVICE_SECURE_OPEN | FILE_CHARACTERISTIC_PNP_DEVICE), FALSE);

    // Stop device INIT !

    // FileConfig init!

    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
        CommEvtDeviceFileCreate,
        CommEvtFileClose,
        CommpIngonreOperation
    );

    fileConfig.AutoForwardCleanupClose = WdfTrue;

    WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);


    status = WdfDeviceCreate(&deviceInit, &objAttributes, &controlDevice);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    RtlInitUnicodeString(&deviceName, Data->Configuration.UserDeviceName);
    status = WdfDeviceCreateSymbolicLink(controlDevice, &deviceName);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    Data->CommDevice = controlDevice;

    devContext = CommGetContextFromDevice(controlDevice);
    if (devContext == NULL)
    {
        WdfObjectDelete(Data->CommDevice);
        return STATUS_INVALID_DEVICE_STATE;
    }

    devContext->Data = Data;
    devContext->Sequence = 0;
    devContext->NotificationQueue = NULL;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests = ((ULONG)-1);

    ioQueueConfig.EvtIoDefault = (PFN_WDF_IO_QUEUE_IO_DEFAULT)CommpOperationNotSupported;
    ioQueueConfig.EvtIoRead = (PFN_WDF_IO_QUEUE_IO_READ)CommpOperationNotSupported;
    ioQueueConfig.EvtIoWrite = (PFN_WDF_IO_QUEUE_IO_WRITE)CommpOperationNotSupported;
    ioQueueConfig.EvtIoStop = (PFN_WDF_IO_QUEUE_IO_STOP)CommpOperationNotSupported;
    ioQueueConfig.EvtIoResume = (PFN_WDF_IO_QUEUE_IO_RESUME)CommpOperationNotSupported;
    ioQueueConfig.EvtIoCanceledOnQueue = (PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)CommpOperationNotSupported;

    ioQueueConfig.EvtIoDeviceControl = CommEvtIoDeviceControl;
    ioQueueConfig.EvtIoInternalDeviceControl = CommEvtIoInternalDeviceControl;

    ioQueueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(controlDevice, &ioQueueConfig, NULL, &queue);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(controlDevice, &ioQueueConfig, NULL, &devContext->NotificationQueue);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    WdfControlFinishInitializing(controlDevice);

    return status;
}*/