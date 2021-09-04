#ifndef _DEBUG_H_
#define _DEBUG_H_
#include "DriverCommon.h"

#if DEBUGGING_ACTIVE 
#define DEBUG_PRINT(...)	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__))
#define DEBUG_STOP()		DbgBreakPoint()
#else
#define DEBUG_PRINT(...)
#define DEBUG_STOP()
#endif

#endif // ! _DEBUG_H_

