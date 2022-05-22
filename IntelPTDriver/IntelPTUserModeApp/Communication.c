#include "Communication.h"
#include "Public.h"
#include <stdio.h>
#include "Globals.h"

NTSTATUS
CommunicationGetDriverHandle(
	HANDLE *Handle
)
{
	NTSTATUS status = CMC_STATUS_SUCCESS;
	if (gApplicationGlobals->Ipt.gDriverHandle == INVALID_HANDLE_VALUE)
	{
		gApplicationGlobals->Ipt.gDriverHandle = CreateFileW(
			SAMPLE_DEVICE_OPEN_NAME,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);
		if (gApplicationGlobals->Ipt.gDriverHandle == INVALID_HANDLE_VALUE)
		{
			printf_s("Could not retrieve driver handle! Error %X\n", GetLastError());
			status = STATUS_INVALID_HANDLE;
		}
		printf_s("[INFO] New driver handle %p\n", gApplicationGlobals->Ipt.gDriverHandle);
	}

	*Handle = gApplicationGlobals->Ipt.gDriverHandle;

	return status;
}

NTSTATUS 
CommunicationSendMessage(
    _In_ COMMUNICATION_MESSAGE *Message, 
    _Inout_opt_ OVERLAPPED **ResponseAvailable
)
{
	NTSTATUS status;
	HANDLE driverHandle = NULL;
	OVERLAPPED* overlapped = NULL;

	overlapped = (OVERLAPPED*)malloc(sizeof(OVERLAPPED));
	if (!overlapped)
	{
		printf_s("Could not allocate overlapped structure\n");
		return STATUS_INTERRUPTED;
	}

	overlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (overlapped->hEvent == NULL)
	{
		printf_s("Could not create event! Error %X\n", GetLastError());
		status = STATUS_INTERRUPTED;
		goto cleanup;
	}

	status = CommunicationGetDriverHandle(
		&driverHandle
	);
	if (!SUCCEEDED(status))
	{
		printf_s("CommunicationGetDriverHandle returned error!\n");
		goto cleanup;

	}

	BOOL controlStatus = DeviceIoControl(
		driverHandle,
		Message->MethodType,
		Message->DataIn,
		Message->DataInSize,
		Message->DataOut,
		Message->DataOutSize,
		Message->BytesWritten,
		overlapped);
	if (controlStatus && (GetLastError() != ERROR_IO_PENDING))
	{
		printf_s("DeviceIoControl failed with status %d\n", GetLastError());
	}

cleanup:

	if (ResponseAvailable && SUCCEEDED(status))
		*ResponseAvailable = overlapped;
	else if(ResponseAvailable && !SUCCEEDED(status))
		*ResponseAvailable = NULL;

	if (!SUCCEEDED(status) && overlapped)
		free(overlapped);


	return status;
}