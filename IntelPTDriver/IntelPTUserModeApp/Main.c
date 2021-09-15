#include <Windows.h>
#include <stdio.h>

#include "Public.h"
#include "UserInterface.h"

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

	/*
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

	*/


	PrintHelp();
	InputCommand();

}