#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "DriverCommon.h"
#include "Public.h"

# define UninitDevice(device)			WdfObjectDelete(device)

typedef struct _DEVICE_INIT_SETTINGS {

	PWCHAR NativeDeviceName;
	ULONG DeviceCharacteristics;
	BOOLEAN OverrideDriverCharacteristics;

} DEVICE_INIT_SETTINGS;

typedef struct _FILE_SETTINGS {
	
	BOOLEAN AutoForwardCleanupClose;
	PFN_WDF_DEVICE_FILE_CREATE FileCreateCallback;
	PFN_WDF_FILE_CLOSE FileCloseCallback;
	PFN_WDF_FILE_CLEANUP FileCleanupCallback;

} FILE_SETTINGS;

typedef struct _DEVICE_CONFIG_SETTINGS {

	PCWSTR UserDeviceName;
	PFN_WDF_OBJECT_CONTEXT_CLEANUP CleanupCallback;
	PFN_WDF_OBJECT_CONTEXT_DESTROY DestroyCallback;

} DEVICE_CONFIG_SETTINGS;


typedef struct _DEVICE_SETTINGS {

	DEVICE_INIT_SETTINGS DeviceInitSettings;
	FILE_SETTINGS FileSettings;
	DEVICE_CONFIG_SETTINGS DeviceConfigSettings;

} DEVICE_SETTINGS;

VOID
DeviceEvtFileCreate(
	_In_ WDFDEVICE Device,
	_In_ WDFREQUEST Request,
	_In_ WDFFILEOBJECT FileObject
);

VOID
DeviceEvtFileClose(
	_In_ WDFFILEOBJECT FileObject
);

VOID
DeviceIngonreOperation(
	_In_ WDFFILEOBJECT FileObject
);

NTSTATUS
InitDevice(
	_In_ WDFDRIVER WdfDriver,
	_In_ DEVICE_SETTINGS* DeviceSettings,
	_Out_ WDFDEVICE** Device
);

#endif