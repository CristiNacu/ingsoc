#include "Commands.h"
#include "Communication.h"
#include <stdio.h>
#include "Public.h"
#include "Debug.h"

NTSTATUS
CommandTest(
    PVOID   InParameter,
    PVOID*  Result
)
{
	COMMUNICATION_MESSAGE *Message;
	OVERLAPPED* overlapped = NULL;
	NTSTATUS status;
	DWORD bytesWritten;

	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	Message = (COMMUNICATION_MESSAGE*)malloc(sizeof(COMMUNICATION_MESSAGE));
	if (!Message)
	{
		//...
		return STATUS_INVALID_HANDLE;
	}

	COMM_DATA_TEST* commTestBufferIn = (COMM_DATA_TEST*)malloc(sizeof(COMM_DATA_TEST));
	if (!commTestBufferIn)
	{
		printf_s("malloc error\n");
		return STATUS_INVALID_HANDLE;
	}

	COMM_DATA_TEST* commTestBufferOut = (COMM_DATA_TEST*)malloc(sizeof(COMM_DATA_TEST));
	if (!commTestBufferOut)
	{
		printf_s("malloc error\n");
		return STATUS_INVALID_HANDLE;
	}

	commTestBufferIn->Magic = UM_TEST_MAGIC;

	Message->MethodType = COMM_TYPE_TEST;
	Message->DataIn = commTestBufferIn;
	Message->DataInSize = sizeof(COMM_DATA_TEST);
	Message->DataOut = commTestBufferOut;
	Message->DataOutSize = sizeof(COMM_DATA_TEST);
	Message->BytesWritten = &bytesWritten;

	status = CommunicationSendMessage(
		Message,
		&overlapped
	);
	if (!SUCCEEDED(status) || !overlapped)
	{
		// ...
		return STATUS_INVALID_HANDLE;
	}
	
	DWORD result = WaitForSingleObject(overlapped->hEvent, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
		printf_s("DeviceIoControl successful\n");
	}
	else
	{
		printf_s("DeviceIoControl unsuccessful\n");
	}

	if (commTestBufferOut->Magic == KM_TEST_MAGIC)
	{
		printf_s("DeviceIoControl did return required status\n");
	}
	else
	{
		printf_s("DeviceIoControl did not return required status\n");
	}

	free(overlapped);
	return CMC_STATUS_SUCCESS;
}

NTSTATUS
CommandQueyPtCapabilities(
    _In_    PVOID   InParameter,
    _Out_   PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);
	DEBUG_STOP();



	

	return CMC_STATUS_SUCCESS;
}

NTSTATUS
CommandSetupPt(
    _In_    PVOID   InParameter,
    _Out_   PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);
	return CMC_STATUS_SUCCESS;
}

NTSTATUS
CommandExit(
    _In_    PVOID   InParameter,
    _Out_   PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	printf_s("HELLO WORLD\n");

	return CMC_STATUS_SUCCESS;
}