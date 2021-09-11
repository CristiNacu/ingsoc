#ifndef _COMMUNICATION_CALLBACKS_H_
#define  _COMMUNICATION_CALLBACKS_H_

#include "DriverCommon.h"

VOID
CommIoControlCallback(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
);

VOID
CommIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
);


#endif