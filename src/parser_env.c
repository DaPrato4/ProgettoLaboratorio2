#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "logger.h"

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
    if (!file) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore nell' apertura del file %s", filename);
        log_event("101", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
        return -1;
    } // Errore apertura file

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        if (trimmed[0] == '\0') continue;
        char* eq = strchr(trimmed, '=');
        if (!eq) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", trimmed);
            log_event("101", "FILE_PARSING", log_msg);
            return -1; // Errore: formato non valido
        }
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
        } else {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Errore: chiave sconosciuta (%s)", key);
            log_event("101", "FILE_PARSING", log_msg);
            return -1; // Errore: chiave sconosciuta
        }

        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Parametro ambiente (%s) correttamente caricata da file", key);
        log_event("001", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
    }
    fclose(file);
    return 0;
}