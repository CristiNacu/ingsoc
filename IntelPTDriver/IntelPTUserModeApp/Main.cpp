#include <Windows.h>
#include <stdio.h>

#include "Communication.h"

#define SAMPLE_DEVICE_OPEN_NAME			L"\\\\.\\SampleComm"
#define COMM_TEST						0x1

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

	BOOL status = DeviceIoControl(
		driverHandle,
		COMM_TEST,
		NULL,
		0,
		NULL,
		0,
		NULL,
		&overlapped);

	printf_s("DeviceIoControl OK\n");



}