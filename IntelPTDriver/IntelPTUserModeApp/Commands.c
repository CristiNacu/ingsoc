#include "Commands.h"
#include "Communication.h"
#include <stdio.h>
#include "Public.h"
#include "IntelProcessorTraceDefs.h"

NTSTATUS
CommandTest(
    PVOID   InParameter,
    PVOID*  Result
)
{
	COMMUNICATION_MESSAGE *message;
	OVERLAPPED* overlapped = NULL;
	NTSTATUS status;
	DWORD bytesWritten;

	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	message = (COMMUNICATION_MESSAGE*)malloc(sizeof(COMMUNICATION_MESSAGE));
	if (!message)
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

	message->MethodType = COMM_TYPE_TEST;
	message->DataIn = commTestBufferIn;
	message->DataInSize = sizeof(COMM_DATA_TEST);
	message->DataOut = commTestBufferOut;
	message->DataOutSize = sizeof(COMM_DATA_TEST);
	message->BytesWritten = &bytesWritten;

	status = CommunicationSendMessage(
		message,
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

	if (overlapped)
		free(overlapped);
	if (commTestBufferOut)
		free(commTestBufferOut);
	if (commTestBufferIn)
		free(commTestBufferIn);
	if (message)
		free(message);

	*Result = NULL;

	return CMC_STATUS_SUCCESS;
}

NTSTATUS
CommandQueryPtCapabilities(
    PVOID   InParameter,
    PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	NTSTATUS status;
	INTEL_PT_CAPABILITIES capabilities;
	COMMUNICATION_MESSAGE message;
	DWORD bytesWritten;
	OVERLAPPED* overlapped = NULL;


	message.MethodType = COMM_TYPE_QUERY_IPT_CAPABILITIES;
	message.DataIn = NULL;
	message.DataInSize = 0;
	message.DataOut = &capabilities;
	message.DataOutSize = sizeof(capabilities);
	message.BytesWritten = &bytesWritten;

	status = CommunicationSendMessage(
		&message,
		&overlapped
	);
	if (!SUCCEEDED(status) || !overlapped)
	{
		*Result = NULL;

		// ...
		return STATUS_INVALID_HANDLE;
	}

	DWORD result = WaitForSingleObject(overlapped->hEvent, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
		printf_s("	\
INTEL PT IS %s:\n		\
LEAF 0 EAX IS %X\n			\
LEAF 0 EBX IS %X\n			\
LEAF 0 ECX IS %X\n			\
LEAF 0 EDX IS %X\n			\
LEAF 1 EAX IS %X\n			\
LEAF 1 EBX IS %X\n			\
LEAF 1 ECX IS %X\n			\
LEAF 1 EDX IS %X\n			\
MaximumValidSubleaf %d\n	\
Cr3FilteringSupport %d\n	\
ConfigurablePsbAndCycleAccurateModeSupport %d\n	\
IpFilteringAndTraceStopSupport %d\n	\
MtcSupport %d\n	\
PtwriteSupport %d\n	\
PowerEventTraceSupport %d\n	\
PsbAndPmiPreservationSupport %d\n	\
TopaOutputSupport %d\n	\
TopaMultipleOutputEntriesSupport %d\n	\
SingleRangeOutputSupport %d\n	\
OutputToTraceTransportSubsystemSupport %d\n	\
IpPayloadsAreLip %d\n	\
NumberOfAddressRanges %d\n	\
BitmapOfSupportedMtcPeriodEncodings %d\n	\
BitmapOfSupportedCycleTresholdValues %d\n	\
BitmapOfSupportedConfigurablePsbFrequencyEncoding %d\n	\
NumberOfTopaOutputEntries %lld\n",
			capabilities.IntelPtAvailable ? "AVAILABLE" : "UNAVAILABLE",
			capabilities.IptCapabilities0.Eax,
			capabilities.IptCapabilities0.Ebx,
			capabilities.IptCapabilities0.Ecx,
			capabilities.IptCapabilities0.Edx,
			capabilities.IptCapabilities1.Eax,
			capabilities.IptCapabilities1.Ebx,
			capabilities.IptCapabilities1.Ecx,
			capabilities.IptCapabilities1.Edx,
			capabilities.IptCapabilities0.Eax.MaximumValidSubleaf,
			capabilities.IptCapabilities0.Ebx.Cr3FilteringSupport,
			capabilities.IptCapabilities0.Ebx.ConfigurablePsbAndCycleAccurateModeSupport,
			capabilities.IptCapabilities0.Ebx.IpFilteringAndTraceStopSupport,
			capabilities.IptCapabilities0.Ebx.MtcSupport,
			capabilities.IptCapabilities0.Ebx.PtwriteSupport,
			capabilities.IptCapabilities0.Ebx.PowerEventTraceSupport,
			capabilities.IptCapabilities0.Ebx.PsbAndPmiPreservationSupport,
			capabilities.IptCapabilities0.Ecx.TopaOutputSupport,
			capabilities.IptCapabilities0.Ecx.TopaMultipleOutputEntriesSupport,
			capabilities.IptCapabilities0.Ecx.SingleRangeOutputSupport,
			capabilities.IptCapabilities0.Ecx.OutputToTraceTransportSubsystemSupport,
			capabilities.IptCapabilities0.Ecx.IpPayloadsAreLip,
			capabilities.IptCapabilities1.Eax.NumberOfAddressRanges,
			capabilities.IptCapabilities1.Eax.BitmapOfSupportedMtcPeriodEncodings,
			capabilities.IptCapabilities1.Ebx.BitmapOfSupportedCycleTresholdValues,
			capabilities.IptCapabilities1.Ebx.BitmapOfSupportedConfigurablePsbFrequencyEncoding,
			capabilities.TopaOutputEntries
		);

	}
	else
	{
		printf_s("DeviceIoControl unsuccessful\n");
	}

	if (overlapped)
		free(overlapped);

	*Result = NULL;

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