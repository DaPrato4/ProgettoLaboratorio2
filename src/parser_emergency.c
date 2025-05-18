#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "logger.h"

// Definisce la lunghezza massima di una riga letta dal file di configurazione
#define MAX_LINE_LENGTH 128
// Numero massimo di richieste di soccorritori per tipo di emergenza
#define MAX_RESCUERS_PER_TYPE 8
// Numero massimo di tipi di emergenza gestibili
#define MAX_EMERGENCY_TYPES 16

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
 * @brief Effettua il parsing di una riga del file di configurazione delle emergenze.
 * @param line Riga da parsare.
 * @param out_type Puntatore dove scrivere la struttura risultante.
 * @param known_types Array di tipi di soccorritori noti.
 * @param known_types_count Numero di tipi di soccorritori noti.
 * @return 0 se il parsing ha successo, -1 altrimenti.
 */
int parse_emergency_type_line(
    const char* line,
    emergency_type_t* out_type,
    rescuer_type_t* known_types,
    int known_types_count
) {
    char buffer[MAX_LINE_LENGTH];
    // Copia la riga in un buffer modificabile
    strncpy(buffer, line, MAX_LINE_LENGTH);
    buffer[MAX_LINE_LENGTH-1] = '\0';

    // Parsing nome emergenza: cerca la prima coppia di parentesi quadre
    char* name_start = strchr(buffer, '[');
    char* name_end = strchr(buffer, ']');
    if (!name_start || !name_end || name_end <= name_start){
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("103", "FILE_PARSING", log_msg);
        return -1; // Errore: formato non valido
    };
    *name_end = '\0'; // Termina la stringa del nome
    char* emergency_name = trim(name_start + 1); // Ottieni il nome senza spazi

    // Parsing priorità: cerca la seconda coppia di parentesi quadre
    char* priority_start = strchr(name_end + 1, '[');
    char* priority_end = strchr(name_end + 1, ']');
    if (!priority_start || !priority_end || priority_end <= priority_start) {
        // Se il formato non è valido, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: formato non valido (%s)", line);
        log_event("103", "FILE_PARSING", log_msg);
        return -1; // Errore: formato non valido
    }
    *priority_end = '\0'; // Termina la stringa della priorità
    int priority = atoi(trim(priority_start + 1)); // Converte la priorità in intero

    // Parsing richieste soccorritori: tutto ciò che segue la seconda parentesi chiusa
    char* rescuers_str = priority_end + 1;
    // Alloca un array per le richieste di soccorritori
    rescuer_request_t* rescuers = malloc(sizeof(rescuer_request_t) * MAX_RESCUERS_PER_TYPE);
    int rescuer_count = 0;

    // Divide la stringa delle richieste per ';'
    char* token = strtok(rescuers_str, ";");
    while (token && rescuer_count < MAX_RESCUERS_PER_TYPE) {
        // Ogni richiesta ha il formato: tipo:numero,tempo
        char* colon = strchr(token, ':');
        char* comma = strchr(token, ',');
        if (!colon || !comma || comma < colon) break;
        *colon = '\0'; // Termina la stringa del tipo
        *comma = '\0'; // Termina la stringa del numero

        char* rescuer_type_name = trim(token); // Tipo soccorritore
        int required_count = atoi(trim(colon + 1)); // Numero richiesto
        int time_to_manage = atoi(trim(comma + 1)); // Tempo di gestione

        // Cerca il tipo di soccorritore tra quelli noti
        rescuer_type_t* rescuer_type_ptr = NULL;
        for (int i = 0; i < known_types_count; ++i) {
            if (strcmp(known_types[i].rescuer_type_name, rescuer_type_name) == 0) {
                rescuer_type_ptr = &known_types[i];
                break;
            }
        }

        // Se il tipo non è noto, ignora la richiesta
        if (!rescuer_type_ptr) {
            token = strtok(NULL, ";");
            continue;
        }

        // Alloca e popola la richiesta
        rescuer_request_t req;
        req.type = rescuer_type_ptr;
        req.required_count = required_count;
        req.time_to_manage = time_to_manage;
        rescuers[rescuer_count++] = req;

        token = strtok(NULL, ";"); // Passa al prossimo token
    }

    // Popola la struttura emergency_type_t con i dati estratti
    out_type->priority = (short)priority;
    out_type->emergency_desc = strdup(emergency_name); // Copia la descrizione
    out_type->rescuers = rescuers; // Array di richieste
    out_type->rescuers_req_number = rescuer_count; // Numero di richieste

    return 0; // Successo
}

/**
 * @brief Carica tutte le emergenze da un file di configurazione.
 * @param filename Nome del file di configurazione.
 * @param out_types Puntatore dove scrivere l'array di emergenze.
 * @param out_count Puntatore dove scrivere il numero di emergenze caricate.
 * @param known_types Array di tipi di soccorritori noti.
 * @param known_types_count Numero di tipi di soccorritori noti.
 * @return 0 se il caricamento ha successo, -1 altrimenti.
 */
int load_emergency_types(
    const char* filename,
    emergency_type_t** out_types,
    int* out_count,
    rescuer_type_t* known_types,
    int known_types_count
) {
    // Apre il file in lettura
    FILE* file = fopen(filename, "r");
    if (!file) {
        // Se il file non può essere aperto, logga l'errore e ritorna -1
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore nell' apertura del file %s", filename);
        log_event("103", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
        return -1;
    } // Errore apertura file

    // Alloca spazio per un massimo di 16 tipi di emergenza
    emergency_type_t* types = malloc(sizeof(emergency_type_t) * MAX_EMERGENCY_TYPES);
    int count = 0;
    char line[MAX_LINE_LENGTH];

    // Legge il file riga per riga
    while (fgets(line, sizeof(line), file)) {
        if (strlen(trim(line)) == 0) continue; // Salta righe vuote
        // Parsea la riga e aggiunge la struttura risultante all'array
        if (parse_emergency_type_line(line, &types[count], known_types, known_types_count) == 0) {
            count++;
            // Logga il caricamento corretto dell'emergenza
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Emergenza (%s) correttamente caricata da file", types[count - 1].emergency_desc);
            log_event("003", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
        }else{
            // Logga l'errore di parsing della riga
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Errore nel caricamento dell'emergenza da file: (%s)", line);
            log_event("103", "FILE_PARSING", log_msg); // Logga l'errore
        }
    }

    fclose(file); // Chiude il file
    *out_types = types; // Restituisce l'array di emergenze
    *out_count = count; // Restituisce il numero di emergenze caricate
    return 0; // Successo
}