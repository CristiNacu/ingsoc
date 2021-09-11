#include "Communication.h"
#include <iostream>

const CHAR* const DriverCommunication::SERVICE_NAME_A = "SampleSVC";
const WCHAR* const DriverCommunication::SERVICE_NAME_W = L"SampleSVC";

DriverCommunication::DriverCommunication()
{
	isRunning = FALSE;
	statusHandle = 0;
	serviceStatus = { 0 };
	stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (NULL == stopEvent)
	{
		// Todo: Throw exceptions
		std::cout << "CreateEventW failed with status " << GetLastError() << "\n";
	}

	// prepare service status structure
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwServiceSpecificExitCode = 0;
}

DriverCommunication::~DriverCommunication()
{
	if (NULL != stopEvent)
	{
		CloseHandle(stopEvent);
		stopEvent = NULL;
	}
}

VOID 
__stdcall 
DriverCommunication::ReportServiceStatus(
	DWORD CurrentState, 
	DWORD Win32ExitCode, 
	DWORD WaitHint
)
{
	DriverCommunication driverComm = DriverCommunication::Instance();
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.
	driverComm.serviceStatus.dwCurrentState = CurrentState;
	driverComm.serviceStatus.dwWin32ExitCode = Win32ExitCode;
	driverComm.serviceStatus.dwWaitHint = WaitHint;

	if (CurrentState == SERVICE_START_PENDING)
	{
		driverComm.serviceStatus.dwControlsAccepted = 0;
	}
	else
	{
		driverComm.serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}
	if ((CurrentState == SERVICE_RUNNING) ||
		(CurrentState == SERVICE_STOPPED))
	{
		driverComm.serviceStatus.dwCheckPoint = 0;
	}
	else
	{
		driverComm.serviceStatus.dwCheckPoint = dwCheckPoint++;
	}

	// Report the status of the service to the SCM.
	if (SetServiceStatus(driverComm.statusHandle, &driverComm.serviceStatus))
	{
		// TODO: Throw exception
		std::cout << "SetServiceStatus returned error: " << GetLastError() << "\n";
	}
}

DriverCommunication& 
DriverCommunication::Instance()
{
	static DriverCommunication s_instance;
	return s_instance;
}

void 
DriverCommunication::Run()
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
		std::cout << "StartServiceCtrlDispatcherW errorr: " << GetLastError() << "\n";
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
		driverComm.ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		SetEvent(driverComm.stopEvent);
	}
	else if (dwControl == SERVICE_CONTROL_INTERROGATE)
	{
		// Todo: Implement functionality
		// return NO_ERROR;
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


	return;
}
