//#ifndef _COMMUNICATION_CALLBACKS_H_
//#define  _COMMUNICATION_CALLBACKS_H_
//
//#include "DriverCommon.h"
//
//VOID
//CommIoControlCallback(
//    _In_ WDFQUEUE Queue,
//    _In_ WDFREQUEST Request,
//    _In_ size_t OutputBufferLength,
//    _In_ size_t InputBufferLength,
//    _In_ ULONG IoControlCode
//);
//
//NTSTATUS
//CommandTest
//(
//    size_t InputBufferLength,
//    size_t OutputBufferLength,
//    PVOID* InputBuffer,
//    PVOID* OutputBuffer,
//    UINT32* BytesWritten
//);
//
//NTSTATUS
//CommandGetPtCapabilities
//(
//    size_t InputBufferLength,
//    size_t OutputBufferLength,
//    PVOID* InputBuffer,
//    PVOID* OutputBuffer,
//    UINT32* BytesWritten
//);
//
//NTSTATUS
//CommandTestIptSetup
//(
//    size_t InputBufferLength,
//    size_t OutputBufferLength,
//    PVOID* InputBuffer,
//    PVOID* OutputBuffer,
//    UINT32* BytesWritten
//);
//
//NTSTATUS
//CommandSendBuffers(
//    size_t InputBufferLength,
//    size_t OutputBufferLength,
//    PVOID* InputBuffer,
//    PVOID* OutputBuffer,
//    UINT32* BytesWritten
//);
//
//NTSTATUS
//CommandFreeBuffer(
//    size_t InputBufferLength,
//    size_t OutputBufferLength,
//    PVOID* InputBuffer,
//    PVOID* OutputBuffer,
//    UINT32* BytesWritten
//);
//
//VOID
//CommIngonreOperation(
//    _In_ WDFFILEOBJECT FileObject
//);
//
//
//#endif