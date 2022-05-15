#include <stdio.h>
#include "librdkafka/rdkafka.h"

#include "Public.h"
#include "UserInterface.h"



int
main(
	int argc,
	char** argv
)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);



    while (1 == 1)
    {
        PrintHelp();
        InputCommand();
    }

    return 0;

}