#include "DriverCommon.h"
#include "Device.h"
#include "Comm.h"

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
    .IoQueueIoDefault = (PFN_WDF_IO_QUEUE_IO_DEFAULT)CommIngonreOperation,
    .IoQueueIoRead = (PFN_WDF_IO_QUEUE_IO_READ)CommIngonreOperation,
    .IoQueueIoWrite = (PFN_WDF_IO_QUEUE_IO_WRITE)CommIngonreOperation,
    .IoQueueIoStop = (PFN_WDF_IO_QUEUE_IO_STOP)CommIngonreOperation,
    .IoQueueIoResume = (PFN_WDF_IO_QUEUE_IO_RESUME)CommIngonreOperation,
    .IoQueueIoCanceledOnQueue = (PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)CommIngonreOperation,
    .IoQueueIoDeviceControl = (PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL)CommIngonreOperation,
    .IoQueueIoInternalDeviceControl = (PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL)CommIngonreOperation
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
    __debugbreak();
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
    DbgBreakPoint();

    // NTSTATUS variable to record success or failure
    NTSTATUS status;
    WDFDRIVER wdfdriver;

    // Allocate the driver configuration object
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES  attributes;

    WDFDEVICE* device;
    COMM_QUEUE_DEVICE_CONTEXT ctxt;

    // Print "Hello World" for DriverEntry
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KmdfHelloWorld: DriverEntry\n"));

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config, DeviceAdd);

    config.EvtDriverUnload = DriverUnload;


    WDF_OBJECT_ATTRIBUTES_INIT(
        &attributes
    );

    attributes.EvtCleanupCallback = SerialEvtDriverContextCleanup;


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

    // Initialize the communication queue
    status = InitCommQueue(
        device,
        &ctxt,
        &DEFAULT_QUEUE_SETTINGS
    );
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] InitCommQueue exitted with status %X\n", status));
        return status;
    }


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