#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/types.h"

#define MAX_LINE_LENGTH 256
#define MAX_RESCUERS_PER_TYPE 8
#define MAX_EMERGENCY_TYPES 16

// Funzione di utilità per rimuovere spazi iniziali e finali da una stringa
// Restituisce un puntatore alla stringa "ripulita"
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

// Parsing di una singola riga del file di configurazione delle emergenze
// line: riga da parsare
// out_type: puntatore dove scrivere la struttura risultante
// known_types: array di tipi di soccorritori noti
// known_types_count: numero di tipi di soccorritori noti
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
    if (!name_start || !name_end || name_end <= name_start) return -1;
    *name_end = '\0'; // Termina la stringa del nome
    char* emergency_name = trim(name_start + 1); // Ottieni il nome senza spazi

    // Parsing priorità: cerca la seconda coppia di parentesi quadre
    char* priority_start = strchr(name_end + 1, '[');
    char* priority_end = strchr(name_end + 1, ']');
    if (!priority_start || !priority_end || priority_end <= priority_start) return -1;
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
        req.type = (struct rescuer_type_s*)rescuer_type_ptr;
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

// Funzione per caricare tutte le emergenze da un file di configurazione
// filename: nome del file
// out_types: puntatore dove scrivere l'array di emergenze
// out_count: puntatore dove scrivere il numero di emergenze caricate
// known_types: array di tipi di soccorritori noti
// known_types_count: numero di tipi di soccorritori noti
int load_emergency_types(
    const char* filename,
    emergency_type_t** out_types,
    int* out_count,
    rescuer_type_t* known_types,
    int known_types_count
) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1; // Errore apertura file

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
        }
    }

    fclose(file); // Chiude il file
    *out_types = types; // Restituisce l'array di emergenze
    *out_count = count; // Restituisce il numero di emergenze caricate
    return 0; // Successo
}