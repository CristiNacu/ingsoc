#include <Windows.h>
#include <stdio.h>

#include "Communication.h"
#include "Public.h"

EXTERN_C
int
__cdecl
main(
	int argc,
	char** argv
)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	printf_s("Hello world app\n");
	//DriverCommunication::Instance().Run();
	DebugBreak();

	HANDLE driverHandle = CreateFileW(
		SAMPLE_DEVICE_OPEN_NAME,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);
	if (driverHandle == INVALID_HANDLE_VALUE)
	{
		printf_s("CreateFileW NOT OK\n");

		return GetLastError();
	}
	printf_s("CreateFileW OK\n");

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (overlapped.hEvent == NULL)
	{
		printf_s("CreateEvent NOT OK\n");

		return 0;
	}
	printf_s("CreateEvent OK\n");

	COMM_DATA_TEST* commTestBufferIn = (COMM_DATA_TEST*)malloc(sizeof(COMM_DATA_TEST));
	if (!commTestBufferIn)
	{
		printf_s("malloc error\n");
		return 0;
	}

	COMM_DATA_TEST* commTestBufferOut = (COMM_DATA_TEST*)malloc(sizeof(COMM_DATA_TEST));
	if (!commTestBufferOut)
	{
		printf_s("malloc error\n");
		return 0;
	}

	commTestBufferIn->Magic = UM_TEST_MAGIC;
	commTestBufferOut->Magic = 0;

	BOOL status = DeviceIoControl(
		driverHandle,
		COMM_TYPE_TEST,
		commTestBufferIn,
		sizeof(COMM_DATA_TEST),
		commTestBufferOut,
		sizeof(COMM_DATA_TEST),
		NULL,
		&overlapped);
	if (status && (GetLastError() != ERROR_IO_PENDING))
	{
		printf_s("DeviceIoControl failed with status %d\n", GetLastError());
		return 0;
	}

	DWORD result = WaitForSingleObject(overlapped.hEvent, INFINITE);
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


	printf_s("DeviceIoControl END\n");



}