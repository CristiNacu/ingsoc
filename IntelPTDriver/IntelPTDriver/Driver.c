#include <ntddk.h>
#include <wdf.h>
#include <wdfobject.h>
#include <intrin.h>
#include "Public.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KmdfHelloWorldEvtDeviceAdd;

VOID
KmdfHelloWorldDriverUnload(
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


    // Print "Hello World" for DriverEntry
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KmdfHelloWorld: DriverEntry\n"));

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config,
        KmdfHelloWorldEvtDeviceAdd
    );

    config.EvtDriverUnload = KmdfHelloWorldDriverUnload;


    WDF_OBJECT_ATTRIBUTES_INIT(
        &attributes
    );

    attributes.EvtCleanupCallback = SerialEvtDriverContextCleanup;


    // Finally, create the driver object
    status = WdfDriverCreate(DriverObject,
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

    //DbgBreakPoint();
    //int cpuidIntelpt[4] = { 0 };
    //__cpuidex(cpuidIntelpt, 0x7, 0x0);
    //KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CPUID(EAX:0x7 ECX:0x0).EBX = %X\n", cpuidIntelpt[1]));
    //if (cpuidIntelpt[1] & (1 << 25))
    //{
    //    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability Available\n"));
    //}
    //else
    //{
    //    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IntelPT Capability NOT Available\n"));
    //}
    //DbgBreakPoint();

    return status;
}

//////////////////////////////////////////////////////////////////////////// IO QUEUE ////////////////////////////////////////////////////////////////////////////////////

typedef struct _COMM_QUEUE_DEVICE_CONTEXT {
    PVOID           Data;
    WDFQUEUE         NotificationQueue;
    volatile LONG    Sequence;
} COMM_QUEUE_DEVICE_CONTEXT, * PCOMM_QUEUE_DEVICE_CONTEXT;

VOID
EchoEvtIoRead(
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
EchoEvtIoWrite(
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
EchoEvtIoQueueContextDestroy(
    WDFOBJECT Object
)
{
    UNREFERENCED_PARAMETER(Object);
    DbgBreakPoint();
}

NTSTATUS
EchoQueueInitialize(
    WDFDEVICE Device
)
{
    WDFQUEUE queue;
    NTSTATUS status;
    COMM_QUEUE_DEVICE_CONTEXT queueContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoRead = EchoEvtIoRead;
    queueConfig.EvtIoWrite = EchoEvtIoWrite;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, COMM_QUEUE_DEVICE_CONTEXT);

    return status;
}

//////////////////////////////////////////////////////////////////////////// END IO QUEUE ////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
KmdfHelloWorldEvtDeviceAdd(
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