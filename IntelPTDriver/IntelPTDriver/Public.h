#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include <guiddef.h>

DEFINE_GUID(
    GUID_DEVINTERFACE_ECHO,
    0xcdc35b6e, 0xbe4, 0x4936, 0xbf, 0x5f, 0x55, 0x37, 0x38, 0xa, 0x7c, 0x1a);

#define SAMPLE_DEVICE_NATIVE_NAME    L"\\Device\\SampleComm"
#define SAMPLE_DEVICE_USER_NAME      L"\\Global??\\SampleComm"
#define SAMPLE_DEVICE_OPEN_NAME      L"\\\\.\\SampleComm"


typedef struct _COMM_PROT_TEST {
    int Magic;
} COMM_PROT_TEST;

#define UM_TEST_MAGIC 0x1234
#define KM_TEST_MAGIC 0x4321


#define COMM_TEST                           0x1
#define COMM_STOP_COMMUNICATION             0x2
#define COMM_UPDATE_COMMUNICATION           0x3
#define COMM_MAX_COMMUNICATION_FUNCTIONS    0x4
#endif
