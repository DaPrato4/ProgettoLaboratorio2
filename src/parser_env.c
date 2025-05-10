#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/types.h"

// Rimuove spazi iniziali e finali
static char* trim(char* str) {
    char* end;
    while(*str == ' ' || *str == '\t') str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end+1) = 0;
    return str;
}

// Carica la configurazione da env.conf
int load_env_config(const char* filename, env_config_t* config) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        if (trimmed[0] == '\0') continue;
        char* eq = strchr(trimmed, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = trim(trimmed);
        char* value = trim(eq + 1);

        if (strcmp(key, "queue") == 0) {
            strncpy(config->queue, value, MAX_QUEUE_NAME-1);
            config->queue[MAX_QUEUE_NAME-1] = '\0';
        } else if (strcmp(key, "height") == 0) {
            config->height = atoi(value);
        } else if (strcmp(key, "width") == 0) {
            config->width = atoi(value);
        }
    }
    fclose(file);
    return 0;
}