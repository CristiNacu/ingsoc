#include "Device.h"
#include "Debug.h"

# define UninitDeviceInit(deviceInit)	WdfDeviceInitFree(deviceInit)

DECLARE_CONST_UNICODE_STRING(
COMM_QUEUE_DEVICE_PROTECTION,
L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)");
extern const UNICODE_STRING  COMM_QUEUE_DEVICE_PROTECTION;

DECLARE_CONST_UNICODE_STRING(
COMM_QUEUE_DEVICE_PROTECTION_FULL,
L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;WD)(A;;GA;;;RC)");
extern const UNICODE_STRING  COMM_QUEUE_DEVICE_PROTECTION_FULL;

NTSTATUS 
InitDeviceInit(
	_In_	WDFDRIVER				Driver,
	_In_	DEVICE_INIT_SETTINGS	*DeviceSettings,
	_Out_	PWDFDEVICE_INIT			*DeviceInit
)
{
	NTSTATUS						status;
	PWDFDEVICE_INIT                 deviceInit = NULL;
	UNICODE_STRING                  deviceName = { 0 };

	// Initialize the deviceInit object
	deviceInit = WdfControlDeviceInitAllocate(Driver, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
	if (deviceInit == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Set native device name
	RtlInitUnicodeString(&deviceName, DeviceSettings->NativeDeviceName);
	status = WdfDeviceInitAssignName(deviceInit, &deviceName);
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	// Set security string to the device
	status = WdfDeviceInitAssignSDDLString(deviceInit, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	// Set device characteristics
	WdfDeviceInitSetCharacteristics(
		deviceInit, 
		DeviceSettings->DeviceCharacteristics, 
		(!DeviceSettings->OverrideDriverCharacteristics)
	);

	// Return the device init
	*DeviceInit = deviceInit;
	return status;

cleanup:
	if (deviceInit)
	{
		UninitDeviceInit(deviceInit);
		deviceInit = NULL;
	}

	*DeviceInit = deviceInit;
	return status;
}

NTSTATUS 
InitFileConfig(
	_In_	FILE_SETTINGS *FileSettings,
	_Out_	PWDF_FILEOBJECT_CONFIG FileConfig
)
{
	WDF_FILEOBJECT_CONFIG_INIT(
		FileConfig,
		FileSettings->FileCreateCallback,
		FileSettings->FileCloseCallback,
		FileSettings->FileCleanupCallback
	);

	FileConfig->AutoForwardCleanupClose = FileSettings->AutoForwardCleanupClose ? WdfTrue : WdfFalse;
	return STATUS_SUCCESS;
}

NTSTATUS
InitDeviceControl(
	_In_	PWDFDEVICE_INIT			DeviceInit,
	_In_	PWDF_FILEOBJECT_CONFIG  FileConfig,
	_In_	DEVICE_CONFIG_SETTINGS  *DeviceConfigSettings,
	_Out_	WDFDEVICE				**WdfDevice
)
{
	NTSTATUS					status;
	UNICODE_STRING				deviceName = { 0 };
	WDFDEVICE					controlDevice = NULL;
	WDF_OBJECT_ATTRIBUTES       objAttributes;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objAttributes, COMM_QUEUE_DEVICE_CONTEXT);

	objAttributes.ExecutionLevel = WdfExecutionLevelPassive;
	objAttributes.EvtCleanupCallback = DeviceConfigSettings->CleanupCallback;
	objAttributes.EvtDestroyCallback = DeviceConfigSettings->DestroyCallback;

	WdfDeviceInitSetFileObjectConfig(DeviceInit, FileConfig, WDF_NO_OBJECT_ATTRIBUTES);
	status = WdfDeviceCreate(&DeviceInit, &objAttributes, &controlDevice);
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	RtlInitUnicodeString(&deviceName, DeviceConfigSettings->UserDeviceName);
	status = WdfDeviceCreateSymbolicLink(controlDevice, &deviceName);
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	*WdfDevice = &controlDevice;
	return status;

cleanup:
	if (controlDevice)
	{
		UninitDevice(controlDevice);
	}

	*WdfDevice = NULL;
	return status;
}

NTSTATUS
InitDevice(
	_In_ WDFDRIVER WdfDriver,
	_In_ DEVICE_SETTINGS *DeviceSettings,
	_Out_ WDFDEVICE **Device
)
{
	NTSTATUS status;
	PWDFDEVICE_INIT deviceInit;
	WDF_FILEOBJECT_CONFIG fileConfig;
	WDFDEVICE *device = NULL;

	DEBUG_PRINT("InitDevice\n");

	status = InitDeviceInit(
		WdfDriver,
		&DeviceSettings->DeviceInitSettings,
		&deviceInit
	);
	if (!NT_SUCCESS(status))
	{
		DEBUG_PRINT("InitDeviceInit error status %X\n", status);
		goto cleanup;
	}

	status = InitFileConfig(
		&DeviceSettings->FileSettings,
		&fileConfig
	);
	if (!NT_SUCCESS(status))
	{
		DEBUG_PRINT("InitFileConfig error status %X\n", status);
		goto cleanup;
	}

	status = InitDeviceControl(
		deviceInit,
		&fileConfig,
		&DeviceSettings->DeviceConfigSettings,
		&device
	);
	if (!NT_SUCCESS(status))
	{
		DEBUG_PRINT("InitDeviceControl error status %X\n", status);
		goto cleanup;
	}

	*Device = device;
	return status;

cleanup:
	DEBUG_PRINT("InitDevice error status %X\n", status);

	if (device)
	{
		UninitDevice(device);
		device = NULL;
	}
	if (deviceInit)
	{
		UninitDeviceInit(deviceInit);
	}

	*Device = device;
	return status;
}

/// TODO

VOID
DeviceEvtFileCreate(
	_In_ WDFDEVICE Device,
	_In_ WDFREQUEST Request,
	_In_ WDFFILEOBJECT FileObject
)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(FileObject);
}

VOID
DeviceEvtFileClose(
	_In_ WDFFILEOBJECT FileObject
)
{
	UNREFERENCED_PARAMETER(FileObject);
}

VOID
DeviceIngonreOperation(
	_In_ WDFFILEOBJECT FileObject
)
{
	UNREFERENCED_PARAMETER(FileObject);
}