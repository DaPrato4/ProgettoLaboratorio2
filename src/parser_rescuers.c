#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "logger.h"
#include "macros.h"

#define MAX_LINE_LENGTH 256
#define MAX_RESCUER_TYPES 32

/**
 * @brief Rimuove spazi iniziali e finali da una stringa.
 * @param str Stringa da ripulire (modificata in-place).
 * @return Puntatore alla stringa ripulita.
 */
static char* trim(char* str) {
    // Avanza il puntatore finché trova spazi o tab
    while(*str == ' ' || *str == '\t') str++;
    if(*str == 0) return str; // Stringa vuota
    char* end = str + strlen(str) - 1;
    // Torna indietro finché trova spazi, tab, newline o carriage return
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end+1) = 0; // Termina la stringa
    return str;
}

/**
 * @brief Effettua il parsing di una riga del file rescuers.conf.
 * @param line Riga da parsare.
 * @param out_type_info Puntatore dove scrivere la struttura risultante.
 * @return 0 se il parsing ha successo, -1 altrimenti.
 */
int parse_rescuer_type_line(const char* line, rescuer_type_info_t* out_type_info) {
    // Copia la riga in un buffer modificabile
    char buffer[MAX_LINE_LENGTH];
    strncpy(buffer, line, MAX_LINE_LENGTH);
    buffer[MAX_LINE_LENGTH-1] = '\0';

    // Parsing nome soccorritore: prima coppia di parentesi quadre
    char* name_start = strchr(buffer, '[');
    char* name_end = strchr(buffer, ']');
    if (!name_start || !name_end || name_end <= name_start) {
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("102", "FILE_PARSING", log_msg);
        return -1;
    }
    *name_end = '\0';
    char* rescuer_name = trim(name_start + 1);

    // Parsing numero soccorritori: seconda coppia di parentesi quadre
    char* num_start = strchr(name_end + 1, '[');
    char* num_end = strchr(name_end + 1, ']');
    if (!num_start || !num_end || num_end <= num_start) {
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("102", "FILE_PARSING", log_msg);
        return -1;
    }
    *num_end = '\0';
    int count = atoi(trim(num_start + 1));

    // Parsing velocità: terza coppia di parentesi quadre
    char* speed_start = strchr(num_end + 1, '[');
    char* speed_end = strchr(num_end + 1, ']');
    if (!speed_start || !speed_end || speed_end <= speed_start) {
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("102", "FILE_PARSING", log_msg);
        return -1;
    }
    *speed_end = '\0';
    int speed = atoi(trim(speed_start + 1));

    // Parsing posizione base: quarta coppia di parentesi quadre
    char* base_start = strchr(speed_end + 1, '[');
    char* base_end = strchr(speed_end + 1, ']');
    if (!base_start || !base_end || base_end <= base_start) {
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("102", "FILE_PARSING", log_msg);
        return -1;
    }
    *base_end = '\0';
    char* times_str = trim(base_start + 1);

    // Divide la stringa dei tempi per ';' per ottenere le coordinate
    int position[2] = {0, 0};
    char* token = strtok(times_str, ";");
    int t = 0;
    while (token && t < 2) {
        position[t++] = atoi(trim(token));
        token = strtok(NULL, ";");
    }
    if (t != 2 || token != NULL) {
        // Se il formato delle coordinate non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("102", "FILE_PARSING", log_msg);
        return -1;
    }

    // Popola la struttura rescuer_type_info_t con i dati estratti
    out_type_info->rescuer_type.rescuer_type_name = strdup(rescuer_name);
    out_type_info->rescuer_type.speed = speed;
    out_type_info->rescuer_type.x = position[0];
    out_type_info->rescuer_type.y = position[1];
    out_type_info->count = count;

    return 0;
}

/**
 * @brief Carica tutti i tipi di soccorritori da un file di configurazione.
 * @param filename Nome del file di configurazione.
 * @param out_types Puntatore dove scrivere l'array di tipi soccorritore.
 * @param out_count Puntatore dove scrivere il numero di tipi caricati.
 * @return 0 se il caricamento ha successo, -1 altrimenti.
 */
int load_rescuer_types(
    const char* filename,
    rescuer_type_info_t** out_types,
    int* out_count
) {
    FILE* file = fopen(filename, "r");
    CHECK_FOPEN("1020",file, filename);

    // Alloca spazio per un massimo di 32 tipi di soccorritore
    *out_types = malloc(sizeof(rescuer_type_info_t) * MAX_RESCUER_TYPES);
    CHECK_MALLOC(*out_types, fail);
    int count = 0;
    char line[MAX_LINE_LENGTH];

    // Legge il file riga per riga
    while (fgets(line, sizeof(line), file)) {
        if (strlen(trim(line)) == 0) continue; // Salta righe vuote
        if (parse_rescuer_type_line(line, &(*out_types)[count]) == 0) {
            count++;
        }
    }

    *out_count = count;

    fclose(file); // Chiude il file
    return 0;
    fail:
    return -1; // Errore: memoria insufficiente
}
