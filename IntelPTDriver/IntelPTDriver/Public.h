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

typedef struct _COMM_BUFFER_ADDRESS {

    unsigned long long PageId;
    PVOID BufferAddress;

} COMM_BUFFER_ADDRESS;


#define UM_TEST_MAGIC                       0x1234
#define KM_TEST_MAGIC                       0x4321



#define COMM_TYPE_TEST                      0x0
#define COMM_TYPE_QUERY_IPT_CAPABILITIES    0x1
#define COMM_TYPE_SETUP_IPT                 0x2
#define COMM_TYPE_MAX                       0x3
#endif
