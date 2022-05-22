#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "librdkafka/rdkafka.h"

#include "Public.h"
#include "UserInterface.h"
#include "Globals.h"

char * 
TrimString(
    char* string
)
{
    while (*string == ' ' && *string != 0)
        string++;
    return string;
}

void
GatherConfigData(
    char *ConfigFilePath
)
{
    FILE *fs;

    int bufferLength = 255;
    char buffer[255];
    errno_t err;

    err = fopen_s(&fs, ConfigFilePath, "r");
    if (err != 0 || !fs)
    {
        printf_s("[ERROR] Could not open config file!\n");
        return;
    }
    
    while (fgets(buffer, bufferLength, fs)) {
        printf("%s\n", buffer);

        char *key = TrimString(strtok(buffer, "=\n"));
        char* value = TrimString(strtok(NULL, "=\n"));

        printf("key %s\n", key);
        printf("value %s\n", value);

        if (strstr(key, "bootstrap_server"))
        {
            char* cpyValue = malloc(sizeof(char) * strlen(value));
            if (cpyValue == NULL)
            {
                printf_s("[ERROR] Failed to allocate memory for saving config values\n");
                continue;
            }
            memcpy(cpyValue, value, strlen(value) + 1);
            gApplicationGlobals->KafkaConfig.BootstrapServer = cpyValue;
            cpyValue = 0;
        }
    }

    fclose(fs);
}


int
main(
	int argc,
	char** argv
)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

    gApplicationGlobals = calloc(1, sizeof(IPT_CONFIG_STRUCT));
    if (gApplicationGlobals == NULL)
    {
        printf_s("[ERROR] Could not allocate space for global variables");
        return 1;
    }

    gApplicationGlobals->Ipt.gDriverHandle = INVALID_HANDLE_VALUE;

    if (argc == 2)
    {
        GatherConfigData(argv[1]);
    }
    else
    {
        GatherConfigData("config.cfg");
    }

    while (1 == 1)
    {
        PrintHelp();
        InputCommand();
    }

    return 0;

}