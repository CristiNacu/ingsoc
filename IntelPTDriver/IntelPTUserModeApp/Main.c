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

	rd_kafka_t* rk;        /* Producer instance handle */
	rd_kafka_conf_t* conf; /* Temporary configuration object */
	char errstr[512];      /* librdkafka API error reporting buffer */
	char buf[512];         /* Message value temporary buffer */
	const char* brokers;   /* Argument: broker list */
	const char* topic;     /* Argument: topic to produce to */

	brokers = "localhost:9092";
	topic = "test_driver_topic";
	conf = rd_kafka_conf_new();

	if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr,
		sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%s\n", errstr);
		return 1;
	}

	rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

	rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
	if (!rk) {
		fprintf(stderr, "%% Failed to create new producer: %s\n",
			errstr);
		return 1;
	}

	while (run && fgets(buf, sizeof(buf), stdin)) {
		size_t len = strlen(buf);
		rd_kafka_resp_err_t err;

		if (buf[len - 1] == '\n') /* Remove newline */
			buf[--len] = '\0';

		if (len == 0) {
			/* Empty line: only serve delivery reports */
			rd_kafka_poll(rk, 0 /*non-blocking */);
			continue;

		}

    retry:
        err = rd_kafka_producev(
            /* Producer handle */
            rk,
            /* Topic name */
            RD_KAFKA_V_TOPIC(topic),
            /* Make a copy of the payload. */
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            /* Message value and length */
            RD_KAFKA_V_VALUE(buf, len),
            /* Per-Message opaque, provided in
             * delivery report callback as
             * msg_opaque. */
            RD_KAFKA_V_OPAQUE(NULL),
            /* End sentinel */
            RD_KAFKA_V_END);

        if (err) {
            /*
             * Failed to *enqueue* message for producing.
             */
            fprintf(stderr,
                "%% Failed to produce to topic %s: %s\n", topic,
                rd_kafka_err2str(err));

            if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                /* If the internal queue is full, wait for
                 * messages to be delivered and then retry.
                 * The internal queue represents both
                 * messages to be sent and messages that have
                 * been sent or failed, awaiting their
                 * delivery report callback to be called.
                 *
                 * The internal queue is limited by the
                 * configuration property
                 * queue.buffering.max.messages */
                rd_kafka_poll(rk,
                    1000 /*block for max 1000ms*/);
                goto retry;
            }
        }
        else {
            fprintf(stderr,
                "%% Enqueued message (%zd bytes) "
                "for topic %s\n",
                len, topic);
        }


        /* A producer application should continually serve
         * the delivery report queue by calling rd_kafka_poll()
         * at frequent intervals.
         * Either put the poll call in your main loop, or in a
         * dedicated thread, or call it after every
         * rd_kafka_produce() call.
         * Just make sure that rd_kafka_poll() is still called
         * during periods where you are not producing any messages
         * to make sure previously produced messages have their
         * delivery report callback served (and any other callbacks
         * you register). */
        rd_kafka_poll(rk, 0 /*non-blocking*/);
    }


    /* Wait for final messages to be delivered or fail.
     * rd_kafka_flush() is an abstraction over rd_kafka_poll() which
     * waits for all messages to be delivered. */
    fprintf(stderr, "%% Flushing final messages..\n");
    rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);

    /* If the output queue is still not empty there is an issue
     * with producing messages to the clusters. */
    if (rd_kafka_outq_len(rk) > 0)
        fprintf(stderr, "%% %d message(s) were not delivered\n",
            rd_kafka_outq_len(rk));

    /* Destroy the producer instance */
    rd_kafka_destroy(rk);

    return 0;
		//while (1==1)
		//{
		//	PrintHelp();
		//	InputCommand();
		//}
}