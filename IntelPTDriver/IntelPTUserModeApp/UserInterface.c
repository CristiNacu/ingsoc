#include "UserInterface.h"
#include "Commands.h"
#include <stdio.h>
#include <Windows.h>
typedef enum _PARAMETER_TYPE {

    ParameterTypeInt,
    ParameterTypeString,
    ParameterTypeFloat

} PARAMETER_TYPE;


typedef struct _PARAMETER_STRUCTURE {

    PARAMETER_TYPE Type;
    const char* Explanation;

} PARAMETER_STRUCTURE;

typedef struct _COMMAND_DEFINITION_STRUCTURE {

    const char* CommandNameString;
    const char* HelpString;
    COMMAND_METHOD Method;
    PARAMETER_STRUCTURE* Parameters;

} COMMAND_DEFINITION_STRUCTURE;


COMMAND_DEFINITION_STRUCTURE Commands[] = {
    {
        "test",
        "performs a KM - UM communication test",
        CommandTest,
        NULL
    },
    {
        "query",
        "queries PT capabilities on all processors",
        CommandQueryPtCapabilities,
        NULL
    },
    {
        "setup",
        "configures PT mechanism on all processors",
        CommandSetupPt,
        NULL
    },
    {
        "help",
        "prints help",
        PrintHelp,
        NULL
    },
    {
        "exit",
        "exits the application",
        CommandExit,
        NULL
    }
};
#define NUMBER_OF_COMMANDS ((unsigned)_countof(Commands))


void
PrintHelp(
)
{
    printf_s("HELP:\n");
    for (DWORD currentCommand = 0; currentCommand < NUMBER_OF_COMMANDS; currentCommand++)
    {
        printf_s("\t%s -- %s\n", Commands[currentCommand].CommandNameString, Commands[currentCommand].HelpString);
    }
}

void
GetParameters(
    PARAMETER_STRUCTURE* parameterStructure,
    PVOID* outputParameters
)
{
    // TODO: Implement
}


void
InputCommand(
)
{
    char command[31];

    scanf_s("%s", command, (unsigned)_countof(command) - 1);

    for (DWORD currentCommand = 0; currentCommand < NUMBER_OF_COMMANDS; currentCommand++)
    {
        if (!strcmp(Commands[currentCommand].CommandNameString, command))
        {
            NTSTATUS status;
            PVOID params = NULL;
            PVOID output = NULL;
            if (Commands[currentCommand].Parameters)
            {
                GetParameters(
                    Commands[currentCommand].Parameters,
                    &params
                );
            }

            status = Commands[currentCommand].Method(
                params,
                &output
            );
            
            if (!SUCCEEDED(status))
            {
                printf_s("Method %s failed with status %d\n", Commands[currentCommand].CommandNameString, status);
            }

            // TODO: Is output necessary? If so, handle it

            return;

        }
    }

    printf_s("Unknown command\n");
    PrintHelp();

}
