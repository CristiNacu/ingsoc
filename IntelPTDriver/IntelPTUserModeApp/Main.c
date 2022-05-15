#include <stdio.h>
#include "librdkafka/rdkafka.h"

#include "Public.h"
#include "UserInterface.h"



static void
dr_msg_cb(rd_kafka_t* rk, const rd_kafka_message_t* rkmessage, void* opaque) {
	if (rkmessage->err)
		fprintf(stderr, "%% Message delivery failed: %s\n",
			rd_kafka_err2str(rkmessage->err));
	else
		fprintf(stderr,
			"%% Message delivered (%zd bytes, "
			"partition %" PRId32 ")\n",
			rkmessage->len, rkmessage->partition);

	/* The rkmessage is destroyed automatically by librdkafka */
}

static volatile int run = 1;


EXTERN_C
int
__cdecl
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