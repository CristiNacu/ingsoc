typedef PVOID KAFKA_HANDLER;

NTSTATUS
KafkaInit(
	char* Brokers,
	KAFKA_HANDLER* KafkaHandler
);

NTSTATUS
KafkaUninit(
	KAFKA_HANDLER* KafkaHandler
);

NTSTATUS
KafkaSendMessage(
    KAFKA_HANDLER KafkaHandler,
    char* Topic,
    BYTE* Buffer,
    unsigned BufferLength
);