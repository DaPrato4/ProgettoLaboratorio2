#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "logger.h"
#include "macros.h"

/**
 * @brief Rimuove spazi iniziali e finali da una stringa.
 * @param str Stringa da ripulire (modificata in-place).
 * @return Puntatore alla stringa ripulita.
 */
static char* trim(char* str) {
    char* end;
    // Avanza il puntatore finché trova spazi o tab
    while(*str == ' ' || *str == '\t') str++;
    if(*str == 0) return str; // Stringa vuota
    end = str + strlen(str) - 1;
    // Torna indietro finché trova spazi, tab, newline o carriage return
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end+1) = 0; // Termina la stringa
    return str;
}

/**
 * @brief Carica la configurazione dell'ambiente da file.
 * @param filename Nome del file di configurazione.
 * @param config Puntatore alla struttura di configurazione da riempire.
 * @return 0 se il caricamento ha successo, -1 altrimenti.
 */
int load_env_config(const char* filename, env_config_t* config) {
    FILE* file = fopen(filename, "r");
    CHECK_FOPEN("1010",file, filename);

    char line[256];
    // Legge il file riga per riga
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        if (trimmed[0] == '\0') continue; // Salta righe vuote
        char* eq = strchr(trimmed, '=');
        if (!eq) {
            // Se la riga non contiene '=', logga l'errore e ritorna -1
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", trimmed);
            log_event("101", "FILE_PARSING", log_msg);
            return -1; // Errore: formato non valido
        }
        *eq = '\0';
        char* key = trim(trimmed);      // Chiave del parametro
        char* value = trim(eq + 1);     // Valore del parametro

        // Gestione delle diverse chiavi di configurazione
        if (strcmp(key, "queue") == 0) {
            // Copia il nome della coda nella struttura di configurazione
            strncpy(config->queue, value, MAX_QUEUE_NAME-1);
            config->queue[MAX_QUEUE_NAME-1] = '\0';
        } else if (strcmp(key, "height") == 0) {
            // Imposta l'altezza dell'ambiente
            config->height = atoi(value);
        } else if (strcmp(key, "width") == 0) {
            // Imposta la larghezza dell'ambiente
            config->width = atoi(value);
        } else {
            // Chiave sconosciuta: logga l'errore e ritorna -1
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Errore: chiave sconosciuta (%s)", key);
            log_event("101", "FILE_PARSING", log_msg);
            return -1; // Errore: chiave sconosciuta
        }

        // Logga il caricamento corretto del parametro
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Parametro ambiente (%s) correttamente caricata da file", key);
        log_event("001", "FILE_PARSING", log_msg);
    }
    fclose(file); // Chiude il file
    return 0;
}