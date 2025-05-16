#include "types.h"
#include "parser_emergency.h"
#include "parser_rescuers.h"
#include "parser_env.h"
#include "emergency_queue.h"
#include "mq_receiver.h"
#include "rescuer.h"
#include "scheduler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "logger.h"

int main() {

    //------LOGGER------
    start_logger_thread(); // Avvia il thread logger
    printf("Avvio del thread logger...\n");
    // log_event("SYSTEM", "START", "Sistema avviato"); // Logga l'evento di avvio del sistema
    // log_event("MQ_RECEIVER", "START", "Thread ricevitore della coda avviato"); // Logga l'evento di avvio del thread ricevitore
    // log_event("SCHEDULER", "START", "Thread scheduler avviato"); // Logga l'evento di avvio del thread scheduler
    // log_event("RESCUER", "START", "Thread soccorritore avviato"); // Logga l'evento di avvio del thread soccorritore


    //------parsing dell'ambiente------
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


    // ------parsing dei soccorritori------
    rescuer_type_info_t* rescuer_types_info;
    int rescuer_count;
    load_rescuer_types("./conf/rescuers.conf", &rescuer_types_info, &rescuer_count);

    // Stampa i soccorritori caricati
    printf("Soccorritori caricati:\n");
    for (int i = 0; i < rescuer_count; ++i) {
        printf("Soccorritore: %s, Numero: %d, Velocità: %d, Posizione: (%d, %d)\n",
               rescuer_types_info[i].rescuer_type.rescuer_type_name,
               rescuer_types_info[i].count,
               rescuer_types_info[i].rescuer_type.speed,
               rescuer_types_info[i].rescuer_type.x,
               rescuer_types_info[i].rescuer_type.y);
    }

    rescuer_type_t rescuer_types[rescuer_count];
    for (int i = 0; i < rescuer_count; ++i) {
        rescuer_types[i] = rescuer_types_info[i].rescuer_type;
    }
    int rescuer_types_count = sizeof(rescuer_types) / sizeof(rescuer_types[0]);


    // ------Parsing delle emergenze------
    emergency_type_t* emergency_types;
    int emergency_count;
    load_emergency_types("./conf/emergency_types.conf", &emergency_types, &emergency_count, rescuer_types, rescuer_types_count);

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

    // ------Creazione digital twin dei soccorritori------
    int total_rescuers = 0;
    for (int i = 0; i < rescuer_count; i++) {
        total_rescuers += rescuer_types_info[i].count;
    }

    int idx = 0;
    // Alloca un array di strutture rescuer_thread_t per gestire i thread dei digital twin dei soccorritori
    rescuer_thread_t* rescuers_twin_thread = malloc(total_rescuers * sizeof(rescuer_thread_t)); 
    for (int i = 0; i < rescuer_count; ++i) {
        for (int j = 0; j < rescuer_types_info[i].count; ++j) {
            rescuers_twin_thread[idx].twin = malloc(sizeof(rescuer_digital_twin_t));
            rescuers_twin_thread[idx].twin->id = idx;
            rescuers_twin_thread[idx].twin->x = rescuer_types_info[i].rescuer_type.x;
            rescuers_twin_thread[idx].twin->y = rescuer_types_info[i].rescuer_type.y;
            rescuers_twin_thread[idx].twin->rescuer = &rescuer_types_info[i].rescuer_type;
            rescuers_twin_thread[idx].twin->status = IDLE;
            start_rescuer(&rescuers_twin_thread[idx]); // Avvia il thread del soccorritore
            //logga la creazione del gemello digitale
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "[(%s) (%d,%d)] Creato gemello digitale per %s",
                rescuers_twin_thread[idx].twin->rescuer->rescuer_type_name,
                rescuers_twin_thread[idx].twin->x,
                rescuers_twin_thread[idx].twin->y,
                rescuers_twin_thread[idx].twin->rescuer->rescuer_type_name);
            char id [4];
            snprintf(id, sizeof(id), "0%02d", rescuers_twin_thread[idx].twin->id);
            log_event(id, "RESCUER_INIT", log_msg); // Logga la creazione del gemello digitale
            idx++;
        }
    }

    //qui abbiamo i soccorritori digital twin con i loro thread tutti avviati

    // Stampa i digital twin creati
        printf("\nDigital twin dei soccorritori creati:\n");
        for (int i = 0; i < total_rescuers; ++i) {
            printf("DT ID: %d, Tipo: %s, Posizione: (%d, %d), Disponibile: %d\n",
                rescuers_twin_thread[i].twin->id,
                rescuers_twin_thread[i].twin->rescuer->rescuer_type_name,
                rescuers_twin_thread[i].twin->x,
                rescuers_twin_thread[i].twin->y,
                rescuers_twin_thread[i].twin->status);
        }


        
    //------PROVE CODA------
    emergency_queue_init(); // Inizializza la coda delle emergenze
    // emergency_t* emergenza1 = malloc(sizeof(emergency_t));
    // *emergenza1 = (emergency_t){ .type = emergency_types[0], .status = WAITING, .x = 10, .y = 20, .time = time(NULL), .rescuer_count = 0, .rescuers_dt = NULL };
    // emergency_t* emergenza2 = malloc(sizeof(emergency_t));
    // *emergenza2 = (emergency_t){ .type = emergency_types[1], .status = WAITING, .x = 30, .y = 40, .time = time(NULL), .rescuer_count = 0, .rescuers_dt = NULL };
    // // Aggiunge le emergenze alla coda
    // emergency_queue_add(emergenza1); // Aggiunge l'emergenza alla coda
    // emergency_queue_add(emergenza2); // Aggiunge l'emergenza alla coda
    // emergency_t* emergenza3 = emergency_queue_get(); // Ottiene l'emergenza dalla coda
    // printf("Emergenza ottenuta dalla coda: %d\n", emergenza3->status); // Stampa l'emergenza ottenuta
    // emergency_t* emergenza4 = emergency_queue_get(); // Ottiene l'emergenza dalla coda
    // printf("Emergenza ottenuta dalla coda: %d\n", emergenza4->status); // Stampa l'emergenza ottenuta


    //------AVVIO MQ------
    printf("\n\n\nAvvio del thread ricevitore della coda...\n");
    pthread_t mq_thread;
    start_mq_receiver_thread(emergency_types,emergency_count, &env_config , &mq_thread); // Avvia il thread per ricevere le emergenze dalla coda
    //pthread_join(mq_thread, NULL); // Attende la fine del thread ricevitore


    //------SCHEDULER------
    scheduler_args_t* args = malloc(sizeof(scheduler_args_t));
    args->rescuer_count = total_rescuers;
    args->rescuers = rescuers_twin_thread; // Passa i soccorritori al thread scheduler
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler_thread_fun, args);

    



    pthread_join(scheduler_thread, NULL); // Attende la fine del thread scheduler

    // Ricorda di liberare la memoria allocata
}