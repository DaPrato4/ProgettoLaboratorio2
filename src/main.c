#include "../include/types.h"
#include "../include/parser_emergency.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    rescuer_type_t known_types[] = {
        { .rescuer_type_name = "Ambulanza", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Pompieri", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Polizia", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Carabinieri", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Protezione Civile", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Soccorso Alpino", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Guardia Costiera", .speed = 0, .x = 0, .y = 0 },
        { .rescuer_type_name = "Vigili Urbani", .speed = 0, .x = 0, .y = 0 }
    };
    int known_types_count = sizeof(known_types) / sizeof(known_types[0]);

    // Parsing delle emergenze
    emergency_type_t* emergency_types;
    int emergency_count;
    load_emergency_types("./conf/emergency_types.conf", &emergency_types, &emergency_count, known_types, known_types_count);





    

    // Stampa le emergenze caricate
    printf("Emergenze caricate:\n");
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