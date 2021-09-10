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

	static
		VOID
		WINAPI CbServiceControlHandler(
			DWORD    dwControl
		);

private:
	
	SERVICE_STATUS_HANDLE statusHandle;
	


	static
	VOID
	WINAPI DriverCallback(
		DWORD   dwNumServicesArgs,
		LPSTR* lpServiceArgVectors
	);

};


#endif // !_COMMUNICATION_H_

