#ifndef _PROCESSOR_TRACE_WINDOWS_CONTROL_
#define _PROCESSOR_TRACE_WINDOWS_CONTROL_

#include <ntddk.h>

NTSTATUS
PtwInit();

void
PtwUninit();

NTSTATUS
PtwSetup();

NTSTATUS
PtwEnable();

NTSTATUS
PtwDisable();

NTSTATUS
PtwHookProcessCodeBase();

NTSTATUS
PtwHookProcessCr3();

NTSTATUS
PtwHookSSDT();



#endif
