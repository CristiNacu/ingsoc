#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <Windows.h>

typedef 
NTSTATUS
(*COMMAND_METHOD)(
    PVOID   InParameter,
    PVOID* Result
    );

NTSTATUS
CommandTest(
    PVOID   InParameter,
    PVOID* Result
);

NTSTATUS
CommandQueyPtCapabilities(
    PVOID   InParameter,
    PVOID* Result
);

NTSTATUS
CommandSetupPt(
    PVOID   InParameter,
    PVOID* Result
);

NTSTATUS
CommandExit(
    PVOID   InParameter,
    PVOID* Result
);

#endif