#include "Communication.h"
#include <iostream>

const CHAR* const DriverCommunication::SERVICE_NAME_A = "SampleSVC";
const WCHAR* const DriverCommunication::SERVICE_NAME_W = L"SampleSVC";

DriverCommunication& DriverCommunication::Instance()
{
	static DriverCommunication s_instance;
	return s_instance;
}

void DriverCommunication::Run()
{

	SERVICE_TABLE_ENTRYW serviceTable[] =
	{
		{ 
			(LPWSTR)DriverCommunication::SERVICE_NAME_W, 
			(LPSERVICE_MAIN_FUNCTIONW)&DriverCommunication::DriverCallback 
		},
		{
			NULL, 
			NULL
		}
	};

	if (!StartServiceCtrlDispatcherW(serviceTable))
	{
		// Todo: Throw exceptions
		std::cout << "StartServiceCtrlDispatcherW errorr: " << GetLastError();
	}
}


VOID 
__stdcall 
DriverCommunication::CbServiceControlHandler(
	DWORD dwControl
)
{
	DriverCommunication driverComm = DriverCommunication::Instance();

	if (dwControl == SERVICE_CONTROL_STOP)
	{
		// Todo: Implement functionality
	}
	else if (dwControl == SERVICE_CONTROL_INTERROGATE)
	{
		// Todo: Implement functionality
	}
	else
	{
		// Todo: Throw error / print error message
	}
}

VOID
WINAPI
DriverCommunication::DriverCallback(
	DWORD dwNumServicesArgs, 
	LPSTR* lpServiceArgVectors
)
{
	UNREFERENCED_PARAMETER(dwNumServicesArgs);
	UNREFERENCED_PARAMETER(lpServiceArgVectors);

	DriverCommunication& currentObject = DriverCommunication::Instance();

	// register our control handler
	DriverCommunication::Instance().statusHandle = RegisterServiceCtrlHandlerW(
		DriverCommunication::SERVICE_NAME_W, 
		&DriverCommunication::CbServiceControlHandler
	);
	if (NULL == DriverCommunication::Instance().statusHandle)
	{
		return;
	}

	// Todo: Fix bugs
	DriverCommunication::Instance().ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	DriverCommunication::Instance().isRunning = TRUE;
	DriverCommunication::Instance().ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 3000);

	// wait for the stop request
	WaitForSingleObject(DriverCommunication::Instance().stopEvent, INFINITE);

	// report service stopped
	DriverCommunication::Instance().ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
	DriverCommunication::Instance().isRunning = FALSE;
	DriverCommunication::Instance().statusHandle = 0;


	return VOID();
}
