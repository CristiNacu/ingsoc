#include "DriverCommon.h"
#include "Device.h"
#include "Communication.h"
#include "Debug.h"
#include "CommunicationCallbacks.h"

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

    .Callbacks.IoQueueIoDefault = (PFN_WDF_IO_QUEUE_IO_DEFAULT)CommIngonreOperation,
    .Callbacks.IoQueueIoRead = (PFN_WDF_IO_QUEUE_IO_READ)CommIngonreOperation,
    .Callbacks.IoQueueIoWrite = (PFN_WDF_IO_QUEUE_IO_WRITE)CommIngonreOperation,
    .Callbacks.IoQueueIoStop = (PFN_WDF_IO_QUEUE_IO_STOP)CommIngonreOperation,
    .Callbacks.IoQueueIoResume = (PFN_WDF_IO_QUEUE_IO_RESUME)CommIngonreOperation,
    .Callbacks.IoQueueIoCanceledOnQueue = (PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)CommIngonreOperation,

    .Callbacks.IoQueueIoDeviceControl = (PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL)CommIoControlCallback,
    .Callbacks.IoQueueIoInternalDeviceControl = (PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL)CommIngonreOperation
};

COMM_COMMUNICATION_DESCRIPTOR DEFAULT_COMMAND_CALLBACKS[] = {

    // COMM_TYPE_TEST                       0x0
    {
        CommandTest,
        sizeof(COMM_DATA_TEST),
        sizeof(COMM_DATA_TEST)
    },

    // COMM_TYPE_QUERY_IPT_CAPABILITIES     0x1
    {
        NULL,
        0,
        0
    },

    // COMM_TYPE_SETUP_IPT                  0x2
    {
        NULL,
        0,
        0
    }
};


VOID
DriverUnload(
    IN WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);
    DbgBreakPoint();
}

VOID
SerialEvtDriverContextCleanup(
    _In_
    WDFOBJECT Object
)
{
    UNREFERENCED_PARAMETER(Object);
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
    WDFQUEUE defaultQueue;

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config, DeviceAdd);
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    config.EvtDriverUnload = DriverUnload;
    attributes.EvtCleanupCallback = SerialEvtDriverContextCleanup;
    
    // Set Driver constants
    gDriverData.IoCallbacks = DEFAULT_COMMAND_CALLBACKS;


    // Create the driver object
    DEBUG_STOP();
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
    DEBUG_STOP();
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
    DEBUG_STOP();
    DEBUG_PRINT("Device Addr %p\n", device);

    // Initialize communication queue. The default, parallel one.
    status = InitCommQueue(
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

    WdfControlFinishInitializing(device);


    //typedef struct _INTEL_PT_CAPABILITIES
    //{
    //
    //    DWORD IntelPtAvailable : 1;
    //    DWORD Cr3FilteringSupport : 1;
    //    DWORD ConfigurablePsbAndCycleAccurateModeSupport : 1;
    //    DWORD IpFilteringAndTraceStopSupport : 1;
    //    DWORD MtcSupport : 1;
    //    DWORD PtwriteSupport : 1;
    //    DWORD PowerEventTraceSupport : 1;
    //    DWORD PsbAndPmiPreservationSupport : 1;
    //    DWORD TopaOutputSupport : 1;
    //    DWORD TopaMultipleOutputSupport : 1;
    //
    //} INTEL_PT_CAPABILITIES;


    //BOOLEAN IntelPtPresent = FALSE;

    //DbgBreakPoint();
    //int cpuidIntelpt[4] = { 0 };
    //__cpuidex(cpuidIntelpt, 0x7, 0x0);
    //KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CPUID(EAX:0x7 ECX:0x0).EBX = %X\n", cpuidIntelpt[1]));
    //if (cpuidIntelpt[1] & (1 << 25))
    //{
    //    IntelPtPresent = TRUE;
    //    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability Available\n"));
    //}
    //else
    //{
    //    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability NOT Available\n"));
    //}

    //if (IntelPtPresent)
    //{
    //    /// Gather future capabilities of Intel PT

    //    __cpuidex(cpuidIntelpt, 0x14, 0x0);
    //    cpuidIntelpt[]

    //}
    //DbgBreakPoint();

    return status;
}