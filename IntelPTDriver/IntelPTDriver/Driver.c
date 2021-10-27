#include "DriverCommon.h"
#include "Device.h"
#include "Communication.h"
#include "Debug.h"
#include "ProcessorTrace.h"
#include "ProcessorTraceWindowsCommands.h"
#include "ProcessorTraceWindowsCommunication.h"
#include "ProcessorTraceWindowsControl.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD DeviceAdd;

DEVICE_SETTINGS DEFAULT_DEVICE_SETTINGS = {
    .DeviceInitSettings = {
        .NativeDeviceName = SAMPLE_DEVICE_NATIVE_NAME,
        .DeviceCharacteristics = (FILE_DEVICE_SECURE_OPEN | FILE_CHARACTERISTIC_PNP_DEVICE),
        .OverrideDriverCharacteristics = TRUE
    },
    .FileSettings = {
        .AutoForwardCleanupClose = TRUE,
        .FileCreateCallback = DeviceEvtFileCreate,
        .FileCloseCallback = DeviceEvtFileClose,
        .FileCleanupCallback = DeviceIngonreOperation
    },
    .DeviceConfigSettings = {
        .UserDeviceName = SAMPLE_DEVICE_USER_NAME,
        .CleanupCallback = NULL,
        .DestroyCallback = NULL
    }
};

IO_QUEUE_SETTINGS DEFAULT_QUEUE_SETTINGS = {
    
    .DefaultQueue = TRUE,
    .QueueType = WdfIoQueueDispatchParallel,

    .Callbacks.IoQueueIoDefault = (PFN_WDF_IO_QUEUE_IO_DEFAULT)PtwCommunicationIngonreOperation,
    .Callbacks.IoQueueIoRead = (PFN_WDF_IO_QUEUE_IO_READ)PtwCommunicationIngonreOperation,
    .Callbacks.IoQueueIoWrite = (PFN_WDF_IO_QUEUE_IO_WRITE)PtwCommunicationIngonreOperation,
    .Callbacks.IoQueueIoStop = (PFN_WDF_IO_QUEUE_IO_STOP)PtwCommunicationIngonreOperation,
    .Callbacks.IoQueueIoResume = (PFN_WDF_IO_QUEUE_IO_RESUME)PtwCommunicationIngonreOperation,
    .Callbacks.IoQueueIoCanceledOnQueue = (PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)PtwCommunicationIngonreOperation,

    .Callbacks.IoQueueIoDeviceControl = (PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL)PtwCommunicationIoControlCallback,
    .Callbacks.IoQueueIoInternalDeviceControl = (PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL)PtwCommunicationIngonreOperation
};

COMM_IO_COMMAND DEFAULT_COMMAND_CALLBACKS[] = {

    // COMM_TYPE_TEST
        PtwCommandTest,

    // COMM_TYPE_QUERY_IPT_CAPABILITIES
        PtwCommandQueryCapabilities,

    // COMM_TYPE_SETUP_IPT
        PtwCommandSetupIpt,

    // COMM_TYPE_GET_BUFFER
        PtwCommandGetBuffer,

    // COMM_TYPE_FREE_BUFFER
        PtwCommandFreeBuffer
};


VOID
DriverUnload(
    IN WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);
    DbgBreakPoint();
}

NTSTATUS
DeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Driver);
    UNREFERENCED_PARAMETER(DeviceInit);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KmdfHelloWorld: KmdfHelloWorldEvtDeviceAdd\n"));

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
)
{

    // NTSTATUS variable to record success or failure
    NTSTATUS status;
    WDFDRIVER wdfdriver;

    // Allocate the driver configuration object
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES  attributes;

    WDFDEVICE device;
    WDFQUEUE defaultQueue = {0};

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config, DeviceAdd);
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    DEBUG_STOP();
    
    config.EvtDriverUnload = DriverUnload;
    
    // Set Driver constants
    gDriverData.IoCallbacks = DEFAULT_COMMAND_CALLBACKS;


    // Create the driver object
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        &wdfdriver
    );
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WdfDriverCreate error\n"));
        return status;
    }
    DEBUG_PRINT("WdfDriverCreate OK\n");

    // Create the device object
    status = InitDevice(
        wdfdriver,
        &DEFAULT_DEVICE_SETTINGS,
        &device
    );
    if (!NT_SUCCESS(status) || !device)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] InitDevice exitted with status %X\n", status));
        return status;
    }
    DEBUG_PRINT("InitDevice OK\n");

    // Initialize the communication queue
    DEBUG_PRINT("Device Addr %p\n", device);

    // Initialize communication queue. The default, parallel one.
    status = PtwCommunicationInit(
        device,
        &DEFAULT_QUEUE_SETTINGS,
        &defaultQueue
    );
    if (!NT_SUCCESS(status))
    {
        // TODO: Uninitialize resources in case of an error
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] InitCommQueue exitted with status %X\n", status));
        return status;
    }
    DEBUG_PRINT("InitCommQueue OK\n");

    KeInitializeEvent(
        &gPagesAvailableEvent,
        NotificationEvent,
        FALSE
    );

    KeInitializeMutex(
        &gCommMutex,
        0
    );

     
    WdfControlFinishInitializing(device);

    PtwInit();

    return status;
}