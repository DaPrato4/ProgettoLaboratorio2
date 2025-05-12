#include "mq_receiver.h"
#include "types.h"
#include "emergency_queue.h"
#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_MSG_SIZE sizeof(emergency_request_t)

struct mq_receiver_args {
    emergency_type_t*   emergency_types;
    int                 emergency_count;
    char*               queue_name;
};

void* mq_receiver_thread(void* arg) {
    mqd_t mq;
    emergency_request_t req;

    // Estrae gli argomenti passati al thread dalla struttura mq_receiver_args
    struct mq_receiver_args* args = (struct mq_receiver_args*)arg;
    emergency_type_t* emergency_types = args->emergency_types; // Array dei tipi di emergenza
    int emergency_count = args->emergency_count;               // Numero di tipi di emergenza
    char* queue_name = (char*)args->queue_name;                // Nome della coda dei messaggi
    free(arg); // Libera la memoria allocata per gli argomenti
    printf("[MQ] Avvio thread ricevitore coda: ----  %s  ----\n", queue_name);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;  // massimo 10 messaggi in coda
    attr.mq_msgsize = sizeof(emergency_request_t);
    attr.mq_curmsgs = 0;

    mq_unlink(queue_name); // Rimuove la coda se esiste già
    mq = mq_open(queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        pthread_exit(NULL);
    }

    while (1) {
        ssize_t bytes = mq_receive(mq, (char*)&req, MAX_MSG_SIZE, NULL);
        if (bytes > 0) {
            printf("[MQ] Ricevuta emergenza: %s (%d,%d)\n", req.emergency_name, req.x, req.y);
            // Converti in emergency_t (semplificato per ora)        // Inizializza la struttura emergency_t a zero
            int i =0;
            for (i = 0; i < emergency_count; ++i) {
                if (strcmp(req.emergency_name, emergency_types[i].emergency_desc) == 0) {
                    break;
                }
            }
            if (i == emergency_count) {
                fprintf(stderr, "❌ Tipo di emergenza non riconosciuto: %s\n", req.emergency_name);
                continue;
            }else {
                printf("✅ Tipo di emergenza riconosciuto: %s\n", req.emergency_name);
                emergency_t em;
                memset(&em, 0, sizeof(em)); // Inizializza la struttura emergency_t a zero
                em.type = emergency_types[i]; // Associa il tipo di emergenza
                em.x = req.x;
                em.y = req.y;
                em.time = req.timestamp;
                em.status = WAITING;                
                em.rescuer_count = emergency_types[i].rescuers_req_number;
                em.rescuers_dt = malloc(em.rescuer_count * sizeof(rescuer_digital_twin_t));

                emergency_queue_add(em);    // Aggiunge l'emergenza alla coda interna
            }

        } else {
            perror("mq_receive");
            sleep(1);
        }
    }

    mq_close(mq);
    pthread_exit(NULL);
}

void start_mq_receiver_thread(emergency_type_t emergency_types[], int emergency_count, char* queue_name, pthread_t* thread) {
    char* queue_name_arg = malloc((strlen(queue_name) + 2) * sizeof(char));
    queue_name_arg[0] = '/';
    strncpy(queue_name_arg + 1, queue_name, strlen(queue_name));
    queue_name_arg[strlen(queue_name) + 1] = '\0';

    struct mq_receiver_args* args = malloc(sizeof(struct mq_receiver_args));
    args->emergency_types = emergency_types;
    args->emergency_count = emergency_count;
    args->queue_name = queue_name_arg;

    printf("[MQ] Avvio thread ricevitore coda: ----  %s  ----\n", queue_name_arg);
    pthread_create(thread, NULL, mq_receiver_thread, args);
}
