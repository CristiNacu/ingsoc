#include "Globals.h"

LARGE_INTEGER gProcessorFrequency;

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

DWORD
WINAPI
ThreadProc(
	_In_ LPVOID lpParameter
);

NTSTATUS
CommandSetupPt(
    _In_    PVOID   InParameter,
    _Out_   PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	NTSTATUS status;
	COMMUNICATION_MESSAGE message;
	DWORD bytesWritten;
	OVERLAPPED* overlapped = NULL;
	PVOID data;

	message.MethodType = COMM_TYPE_SETUP_IPT;
	message.DataIn = NULL;
	message.DataInSize = 0;
	message.DataOut = &data;
	message.DataOutSize = sizeof(COMM_DATA_SETUP_IPT);
	message.BytesWritten = &bytesWritten;

	printf_s("[INFO] Kafka init\n");
	status = KafkaInit(
		gApplicationGlobals->KafkaConfig.BootstrapServer,
		&gApplicationGlobals->KafkaConfig.KafkaHandler
	);
	if (status != CMC_STATUS_SUCCESS)
	{
		printf_s("[ERROR] Kafka initialization failed!\n");
		return status;
	}
	printf_s("[INFO] Kafka init successful\n");


	printf_s("[INFO] Calling the driver\n");
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
	printf_s("[INFO] Driver called\n");

	
	DWORD result = WaitForSingleObject(overlapped->hEvent, INFINITE);
	printf_s("[INFO] Driver responded\n");
	if (result == WAIT_OBJECT_0)
	{
		QueryPerformanceFrequency(
			&gProcessorFrequency
		);

		printf_s("[INFO] Create worker thread\n");
		HANDLE thread = CreateThread(
			NULL,
			0,
			ThreadProc,
			data,
			0,
			NULL
		);
		if (thread == INVALID_HANDLE_VALUE || !thread)
		{
			return STATUS_FATAL_APP_EXIT;
		}
		WaitForSingleObject(thread, 0);
		printf_s("[INFO] Thread created\n");
	}
	else
	{
		printf_s("[ERROR] DeviceIoControl unsuccessful\n");
	}

	if (overlapped)
		free(overlapped);

	*Result = NULL;

	return CMC_STATUS_SUCCESS;
}

NTSTATUS
CommandGetBuffer(
	unsigned long long* BufferId,
	PVOID* Buffer
)
{
	NTSTATUS status;
	COMMUNICATION_MESSAGE message;
	DWORD bytesWritten;
	OVERLAPPED* overlapped = NULL;
	COMM_BUFFER_ADDRESS *data = calloc(1, sizeof(COMM_BUFFER_ADDRESS));

	message.MethodType = COMM_TYPE_GET_BUFFER;
	message.DataIn = NULL;
	message.DataInSize = 0;
	message.DataOut = data;
	message.DataOutSize = sizeof(COMM_BUFFER_ADDRESS);
	message.BytesWritten = &bytesWritten;

	status = CommunicationSendMessage(
		&message,
		&overlapped
	);
	if (!SUCCEEDED(status) || !overlapped)
	{
		return status;
	}

	DWORD result = WaitForSingleObject(
		overlapped->hEvent, 
		INFINITE
	);
	if (result == WAIT_OBJECT_0)
	{
		if (bytesWritten != sizeof(COMM_BUFFER_ADDRESS))
		{
			status = STATUS_INVALID_HANDLE;
			goto cleanup;
		}

		*Buffer = data;
		*BufferId = data->Header.SequenceId;
	}
	else
	{
		printf_s("WaitForSingleObject unsuccessful\n");
	}

cleanup:
	if (overlapped)
		free(overlapped);

	return status;
}


NTSTATUS
CommandFreeBuffer(
	unsigned long long BufferId
)
{
	NTSTATUS status;
	COMMUNICATION_MESSAGE message;
	DWORD bytesWritten;
	OVERLAPPED* overlapped = NULL;
	unsigned long long bufferId = BufferId;

	message.MethodType = COMM_TYPE_FREE_BUFFER;
	message.DataIn = &bufferId;
	message.DataInSize = sizeof(unsigned long long);
	message.DataOut = NULL;
	message.DataOutSize = 0;
	message.BytesWritten = &bytesWritten;

	status = CommunicationSendMessage(
		&message,
		&overlapped
	);
	if (!SUCCEEDED(status) || !overlapped)
	{
		return status;
	}

	DWORD result = WaitForSingleObject(
		overlapped->hEvent,
		INFINITE
	);
	if (result != WAIT_OBJECT_0)
	{
		printf_s("WaitForSingleObject unsuccessful\n");
	}

	if (overlapped)
		free(overlapped);

	return status;
}



DWORD
WINAPI
ThreadProc(
	_In_ LPVOID lpParameter
)
{
	UNREFERENCED_PARAMETER(lpParameter);

	NTSTATUS status;
	unsigned long long bufferId;
	COMM_BUFFER_ADDRESS *packetInfo;
	unsigned long packetSize;
	FILE* fileHandle;
	KAFKA_PACKET* packet;
	//DebugBreak();

	while (1 == 1)
	{
		//DebugBreak();

		status = CommandGetBuffer(
			&bufferId,
			&(PVOID)packetInfo
		);
		if (!SUCCEEDED(status))
		{
			continue;
		}

		long long tscInSeconds = (__rdtsc() / gProcessorFrequency.QuadPart);

		if (packetInfo->Header.Options.FirstPacket)
		{
			packetSize = sizeof(KAFKA_PACKET_FIRST);
			KAFKA_PACKET_FIRST *packetFirst = (KAFKA_PACKET_FIRST*)calloc(1, packetSize);
			if (packetFirst == NULL)
			{
				printf_s("[ERROR] Cannot allocate kafka packet!\n");
				return;
			}
			memcpy(packetFirst, &(packetInfo->Header), sizeof(PACKET_HEADER_INFORMATION));

			packetFirst->ImageBaseAddress = packetInfo->Payload.FirstPacket.ImageBaseAddress;
			packetFirst->ImageSize = packetInfo->Payload.FirstPacket.ImageSize;
			packetFirst->ProcessorFrequency = packetInfo->Payload.FirstPacket.ProcessorFrequency;


			printf_s("[INFO] Sequence %d CPU %d - first packet received. Image base %p Buffer Size %ul. TSC in ms %lld\n", packetFirst->Header.SequenceId,
				packetFirst->Header.CpuId, packetFirst->ImageBaseAddress, packetFirst->ImageSize, tscInSeconds);

			packet = (KAFKA_PACKET*)packetFirst;

		}
		else
		{
			char* iptBuffer = (char*)packetInfo->Payload.GenericPacket.BufferAddress;
			packetSize = sizeof(KAFKA_PACKET) - 1 + 10 * USN_PAGE_SIZE;
			packet = (KAFKA_PACKET*)calloc(1, packetSize);
			if (packet == NULL)
			{
				printf_s("[ERROR] Cannot allocate kafka packet!\n");
				return;
			}

			memcpy(packet, &(packetInfo->Header), sizeof(PACKET_HEADER_INFORMATION));
			memcpy(&(packet->Data), iptBuffer, 10 * USN_PAGE_SIZE);

			if (packetInfo->Header.Options.LastPacket)
			{
				printf_s("[INFO] Sequence %d - packet %ld  CPU %d Last Packet. Buffer address %p Buffer Size %lld TSC in ms %lld\n",
					packetInfo->Header.SequenceId, packetInfo->Header.PacketId, packetInfo->Header.CpuId,
					packetInfo->Payload.GenericPacket.BufferAddress, packetInfo->Payload.GenericPacket.BufferSize, 
					tscInSeconds);
			}
			else
			{
				printf_s("[INFO] Sequence %d - packet %ld  CPU %d. Buffer address %p Buffer Size %lld Tsc in ms %lld\n",
					packetInfo->Header.SequenceId, packetInfo->Header.PacketId, packetInfo->Header.CpuId,
					packetInfo->Payload.GenericPacket.BufferAddress, packetInfo->Payload.GenericPacket.BufferSize,
					tscInSeconds);
			}
		}

		
		status = KafkaSendMessage(
			gApplicationGlobals->KafkaConfig.KafkaHandler,
			gApplicationGlobals->KafkaConfig.KafkaTopicName,
			packet,
			packetSize
		);
		//if (status != CMC_STATUS_SUCCESS)
		//{
		//	printf_s("[ERROR] Could not send message to kafka!\n");
		//}
		//else
		//{
		//	printf_s("[INFO] Sent buffer %p with size %ud to Kafka!\n", iptBuffer, 10 * USN_PAGE_SIZE);
		//}
		
		/*fopen_s(
			&fileHandle,
			"ProcessorTrace",
			"a"
		);
		if (fileHandle == INVALID_HANDLE_VALUE || !fileHandle)
		{
			DebugBreak();
			return STATUS_ACCESS_VIOLATION;
		}

		for (int i = 0; i < 10 * USN_PAGE_SIZE; i++)
		{
			fprintf(fileHandle, "%c", (char*)packet[i]);
		}

		if (!FlushFileBuffers(fileHandle))
		{
			printf("Could not flush file buffers\n");
		}
		fclose(fileHandle);
		printf("Written bytes\n");*/


		status = CommandFreeBuffer(
			bufferId
		);
	}
	//DebugBreak();
	return 0;
}

NTSTATUS
CommandExit(
    _In_    PVOID   InParameter,
    _Out_   PVOID*  Result
)
{
	UNREFERENCED_PARAMETER(InParameter);
	UNREFERENCED_PARAMETER(Result);

	NTSTATUS status;

	status = KafkaUninit(
		gApplicationGlobals->KafkaConfig.KafkaHandler
	);
	if (status != CMC_STATUS_SUCCESS)
	{
		printf_s("[ERROR] Couldn't uninit Kafka!\n");
	}

	return status;
}