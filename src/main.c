// main.c - Entry point del sistema di gestione emergenze
// Avvia logger, parsing configurazioni, digital twin, ricezione MQ e scheduler

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
#include "macros.h"

int main() {

    // ------ LOGGER ------
    start_logger_thread(); // Avvia il thread logger

    // ------ PARSING DELL'AMBIENTE ------
    env_config_t env_config;
    if (load_env_config("./conf/env.conf", &env_config) != 0) {
        printf("❌ Errore nel caricamento della configurazione dell'ambiente\n");
        return 1;
    }

    // ------ PARSING DEI SOCCORRITORI ------
    rescuer_type_info_t* rescuer_types_info;
    int rescuer_count;
    load_rescuer_types("./conf/rescuers.conf", &rescuer_types_info, &rescuer_count);

    rescuer_type_t rescuer_types[rescuer_count];
    for (int i = 0; i < rescuer_count; ++i) {
        if(rescuer_types_info[i].rescuer_type.x > env_config.width || rescuer_types_info[i].rescuer_type.y > env_config.height) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Soccorritore (%s) fori limiti di mappa", rescuer_types_info[i].rescuer_type.rescuer_type_name);
            log_event("102", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
        }else{
            rescuer_types[i] = rescuer_types_info[i].rescuer_type;
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Soccorritore (%s) correttamente caricata da file", rescuer_types_info[i].rescuer_type.rescuer_type_name);
            log_event("002", "FILE_PARSING", log_msg); // Logga l'emergenza caricata
        }
        
    }
    int rescuer_types_count = sizeof(rescuer_types) / sizeof(rescuer_types[0]);


    // ------ PARSING DELLE EMERGENZE ------
    emergency_type_t* emergency_types;
    int emergency_count;
    load_emergency_types("./conf/emergency_types.conf", &emergency_types, &emergency_count, rescuer_types, rescuer_types_count);

    // ------ CREAZIONE DIGITAL TWIN DEI SOCCORRITORI ------
    int total_rescuers = 0;
    for (int i = 0; i < rescuer_count; i++) {
        total_rescuers += rescuer_types_info[i].count;
    }

    // Alloca e avvia i thread dei digital twin
    int idx = 0;
    rescuer_thread_t* rescuers_twin_thread = malloc(total_rescuers * sizeof(rescuer_thread_t)); 
    CHECK_MALLOC(rescuers_twin_thread, label);
    for (int i = 0; i < rescuer_count; ++i) {
        for (int j = 0; j < rescuer_types_info[i].count; ++j) {
            rescuers_twin_thread[idx].twin = malloc(sizeof(rescuer_digital_twin_t));
            CHECK_MALLOC(rescuers_twin_thread[idx].twin, label);
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

    //------PROVE CODA------
    emergency_queue_init(); // Inizializza la coda delle emergenze

    // ------ AVVIO THREAD MQ RECEIVER ------
    pthread_t mq_thread;
    start_mq_receiver_thread(emergency_types, emergency_count, &env_config, &mq_thread);

    // ------ AVVIO THREAD SCHEDULER ------
    scheduler_args_t* args = malloc(sizeof(scheduler_args_t));
    CHECK_MALLOC(args, label);
    args->rescuer_count = total_rescuers;
    args->rescuers = rescuers_twin_thread;
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler_thread_fun, args);

    // Attende la fine del thread scheduler (il programma resta attivo)
    pthread_join(scheduler_thread, NULL);
    label:
    printf("❌ Errore durante l'esecuzione del programma\n");
    free(args);
    return 0;
}