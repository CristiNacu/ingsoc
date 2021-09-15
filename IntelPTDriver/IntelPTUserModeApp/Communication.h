#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include <Windows.h>

typedef struct _COMMUNICATION_MESSAGE {

    DWORD MethodType;
    PVOID DataIn;
    DWORD DataInSize;
    PVOID DataOut;
    DWORD DataOutSize;
    DWORD* BytesWritten;

} COMMUNICATION_MESSAGE;

NTSTATUS
CommunicationSendMessage(
    _In_ COMMUNICATION_MESSAGE* Message,
    _Inout_opt_ OVERLAPPED** ResponseAvailable
);


#endif // !_COMMUNICATION_H_

