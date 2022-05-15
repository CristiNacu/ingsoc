#include "librdkafka/rdkafka.h"
#include "KafkaUtils.h"

static volatile int gRun = 1;
char gErrstr[512];

// TODO: Support for multiple polling threads? 
HANDLE gPollingThreadHandle = NULL;


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

DWORD
WINAPI
PollKafkaThread(
    _In_ LPVOID lpParameter
)
{
    do {
        rd_kafka_t* rk = (rd_kafka_t*)lpParameter;
        rd_kafka_poll(rk, 0);
    } while (gRun);
    return 0;
}

NTSTATUS
StartKafkaPolling(
    KAFKA_HANDLER KafkaHandler
)
{
    if (gPollingThreadHandle != NULL)
        return CMC_STATUS_SUCCESS;

    gPollingThreadHandle = CreateThread(
        NULL,
        0,
        PollKafkaThread,
        (LPVOID)KafkaHandler,
        0,
        NULL
    );
    if (gPollingThreadHandle == INVALID_HANDLE_VALUE || !gPollingThreadHandle)
    {
        return STATUS_FATAL_APP_EXIT;
    }

    return CMC_STATUS_SUCCESS;
}

NTSTATUS
StopKafkaPolling()
{
    WaitForSingleObject(gPollingThreadHandle, INFINITE);
}

NTSTATUS
KafkaInit(
	char *Brokers,
	KAFKA_HANDLER *KafkaHandler
)
{
	rd_kafka_t* rk;        /* Producer instance handle */
	rd_kafka_conf_t* conf; /* Temporary configuration object */

	if (!KafkaHandler)
		return STATUS_INVALID_PARAMETER;

	conf = rd_kafka_conf_new();

	if (rd_kafka_conf_set(conf, "bootstrap.servers", Brokers, gErrstr,
		sizeof(gErrstr)) != RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%s\n",gErrstr);
		return STATUS_INTERRUPTED;
	}

	rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

	rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, gErrstr, sizeof(gErrstr));
	if (!rk) {
		fprintf(stderr, "%% Failed to create new producer: %s\n",
            gErrstr);
		return STATUS_INTERRUPTED;
	}

    *KafkaHandler = rk;
	return CMC_STATUS_SUCCESS;
}

NTSTATUS
KafkaUninit(
    KAFKA_HANDLER* KafkaHandler
)
{
    rd_kafka_t* rk = (rd_kafka_t*)KafkaHandler;

    /* Wait for final messages to be delivered or fail.
     * rd_kafka_flush() is an abstraction over rd_kafka_poll() which
     * waits for all messages to be delivered. */
    fprintf(stderr, "%% Flushing final messages..\n");
    StopKafkaPolling();
    rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);

    /* If the output queue is still not empty there is an issue
     * with producing messages to the clusters. */
    if (rd_kafka_outq_len(rk) > 0)
        fprintf(stderr, "%% %d message(s) were not delivered\n",
            rd_kafka_outq_len(rk));

    /* Destroy the producer instance */
    rd_kafka_destroy(rk);
}

NTSTATUS
KafkaSendMessage(
    KAFKA_HANDLER KafkaHandler,
    char *Topic,
    BYTE *Buffer,
    unsigned BufferLength
)
{
    rd_kafka_resp_err_t err;
    rd_kafka_t* rk = (rd_kafka_t*)KafkaHandler;

    if(!gPollingThreadHandle)
        StartKafkaPolling(KafkaHandler);

retry:
    err = rd_kafka_producev(
        /* Producer handle */
        rk,
        /* Topic name */
        RD_KAFKA_V_TOPIC(Topic),
        /* Make a copy of the payload. */
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
        /* Message value and length */
        RD_KAFKA_V_VALUE(Buffer, BufferLength),
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
            "%% Failed to produce to topic %s: %s\n", Topic,
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
            
            // gotos are not * THE BEST * technical solution but for the moment it does the job.
            // also it is highly unlikely to get here during testing phase *HOPEFULLY* 

            goto retry;
        }
    }
    else {
        fprintf(stderr,
            "%% Enqueued message (%ud bytes) "
            "for topic %s\n",
            BufferLength, Topic);
    }
}