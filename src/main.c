#include "../include/types.h"
#include "../include/parser_emergency.h"
#include "../include/parser_rescuers.h"
#include "../include/parser_env.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    //parsing dell'ambiente
    env_config_t env_config;
    if (load_env_config("./conf/env.conf", &env_config) != 0) {
        fprintf(stderr, "Errore nel caricamento della configurazione dell'ambiente\n");
        return 1;
    }
    printf("Configurazione dell'ambiente:\n");
    printf("Coda: %s\n", env_config.queue);
    printf("Altezza: %d\n", env_config.height);
    printf("Larghezza: %d\n", env_config.width);
    printf("Caricamento della configurazione dell'ambiente completato.\n\n\n\n");


    

    // parsing dei soccorritori
    rescuer_type_info_t* rescuer_types;
    int rescuer_count;
    load_rescuer_types("./conf/rescuers.conf", &rescuer_types, &rescuer_count);

    // Stampa i soccorritori caricati
    printf("Soccorritori caricati:\n");
    for (int i = 0; i < rescuer_count; ++i) {
        printf("Soccorritore: %s, Numero: %d, Velocità: %d, Posizione: (%d, %d)\n",
               rescuer_types[i].rescuer_type.rescuer_type_name,
               rescuer_types[i].count,
               rescuer_types[i].rescuer_type.speed,
               rescuer_types[i].rescuer_type.x,
               rescuer_types[i].rescuer_type.y);
    }

    rescuer_type_t known_types[rescuer_count];
    for (int i = 0; i < rescuer_count; ++i) {
        known_types[i] = rescuer_types[i].rescuer_type;
    }
    int known_types_count = sizeof(known_types) / sizeof(known_types[0]);

    // Parsing delle emergenze
    emergency_type_t* emergency_types;
    int emergency_count;
    load_emergency_types("./conf/emergency_types.conf", &emergency_types, &emergency_count, known_types, known_types_count);

    // Stampa le emergenze caricate
    printf("\n\n\nEmergenze caricate:\n");
    for (int i = 0; i < emergency_count; ++i) {
        printf("Emergenza: %s, Priorità: %d\n", emergency_types[i].emergency_desc, emergency_types[i].priority);
        for (int j = 0; j < emergency_types[i].rescuers_req_number; ++j) {
            printf("  Tipo: %s, Numero richiesto: %d, Tempo di gestione: %d\n",
                   emergency_types[i].rescuers[j].type->rescuer_type_name,
                   emergency_types[i].rescuers[j].required_count,
                   emergency_types[i].rescuers[j].time_to_manage);
        }
    }

    // ... codice ...

    // Ricorda di liberare la memoria allocata
}