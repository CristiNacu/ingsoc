#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_
#include <windows.h>

class DriverCommunication
{
public:
	static const CHAR* const SERVICE_NAME_A;
	static const WCHAR* const SERVICE_NAME_W;
	static DriverCommunication& Instance();

	void Run();
	bool IsRunning() { return isRunning; }

	static
		VOID
		WINAPI CbServiceControlHandler(
			DWORD    dwControl
		);

private:
	
	SERVICE_STATUS_HANDLE statusHandle;
	
	bool isRunning;
	SERVICE_STATUS_HANDLE statusHandle;
	HANDLE stopEvent;
	SERVICE_STATUS serviceStatus;

	DriverCommunication();
	~DriverCommunication();

	static VOID WINAPI ReportServiceStatus(
		_In_ DWORD CurrentState,
		_In_ DWORD Win32ExitCode,
		_In_ DWORD WaitHint);

	static
	VOID
	WINAPI DriverCallback(
		DWORD   dwNumServicesArgs,
		LPSTR* lpServiceArgVectors
	);

};


#endif // !_COMMUNICATION_H_

