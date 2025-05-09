#include "../include/types.h"
#include "../include/parser_emergency.h"
#include <string.h>

int main() {
    rescuer_type_t known_types[] = {
        { .rescuer_type_name = "ambulanza" },
        { .rescuer_type_name = "vigili del fuoco" },
        { .rescuer_type_name = "polizia" }
    };
    int known_types_count = sizeof(known_types) / sizeof(known_types[0]);

    // Ora puoi passarli alle funzioni di parser_emergency.c
    emergency_type_t* emergency_types;
    int emergency_count;
    load_emergency_types("emergenze.txt", &emergency_types, &emergency_count, known_types, known_types_count);

    // ... codice ...

    // Ricorda di liberare la memoria allocata
}