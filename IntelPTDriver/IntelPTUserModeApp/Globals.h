#include "Commands.h"
#include "Communication.h"
#include <stdio.h>
#include "Public.h"
#include "ProcessorTraceShared.h"
#include <fileapi.h>
#include "KafkaUtils.h"

typedef struct _IPT_CONFIG_STRUCT {
    short UseKafka;

    struct {
        char* BootstrapServer;
        char* KafkaTopicName;
        KAFKA_HANDLER KafkaHandler;
    } KafkaConfig;

    struct {

        HANDLE gDriverHandle;
        int NumberOfThreads;

    } Ipt;

} IPT_CONFIG_STRUCT;

IPT_CONFIG_STRUCT *gApplicationGlobals;