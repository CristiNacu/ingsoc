#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include <guiddef.h>

DEFINE_GUID(
    GUID_DEVINTERFACE_ECHO,
    0xcdc35b6e, 0xbe4, 0x4936, 0xbf, 0x5f, 0x55, 0x37, 0x38, 0xa, 0x7c, 0x1a);

#define SAMPLE_DEVICE_NATIVE_NAME    L"\\Device\\SampleComm"
#define SAMPLE_DEVICE_USER_NAME      L"\\Global??\\SampleComm"
#define SAMPLE_DEVICE_OPEN_NAME      L"\\\\.\\SampleComm"

typedef struct _COMM_DATA_TEST {
    int Magic;
} COMM_DATA_TEST;

typedef struct _COMM_DATA_QUERY_IPT_CAPABILITIES {
    int placeholder;
} COMM_DATA_QUERY_IPT_CAPABILITIES;

typedef struct _COMM_DATA_SETUP_IPT {
    void* BufferAddress;
    unsigned long long BufferSize;
} COMM_DATA_SETUP_IPT;

#pragma pack(push, 1)

typedef struct _PACKET_HEADER_INFORMATION {
    unsigned HeaderSize;
    unsigned long long PacketId;
    unsigned CpuId;
    unsigned SequenceId;

    struct PacketInformation
    {
        short FirstPacket : 1;
        short LastPacket : 1;
        short ExecutableTrace : 1;
    } Options;
} PACKET_HEADER_INFORMATION;

typedef struct _COMM_BUFFER_ADDRESS {
    
    PACKET_HEADER_INFORMATION Header;

    union {
        struct {
            PVOID BufferAddress;
            unsigned long BufferSize;
        } GenericPacket;
        struct {
            PVOID ImageBaseAddress;
            unsigned long ImageSize;
            ULONGLONG ProcessorFrequency;
        } FirstPacket;

    } Payload;

} COMM_BUFFER_ADDRESS;

typedef struct _KAFKA_PACKET {
    PACKET_HEADER_INFORMATION Header;
    char Data[1];
} KAFKA_PACKET;

typedef struct _KAFKA_PACKET_FIRST {
    PACKET_HEADER_INFORMATION Header;
    PVOID ImageBaseAddress;
    ULONG ImageSize;
    ULONGLONG ProcessorFrequency;
} KAFKA_PACKET_FIRST;

#pragma pack(pop)

#define UM_TEST_MAGIC                       0x1234
#define KM_TEST_MAGIC                       0x4321

#define COMM_TYPE_TEST                      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8B0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define COMM_TYPE_QUERY_IPT_CAPABILITIES    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8B1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define COMM_TYPE_SETUP_IPT                 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8B2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define COMM_TYPE_GET_BUFFER                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8B3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define COMM_TYPE_FREE_BUFFER               CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8B4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif
