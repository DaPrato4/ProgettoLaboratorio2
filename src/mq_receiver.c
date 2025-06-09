#include "mq_receiver.h"
#include "types.h"
#include "emergency_queue.h"
#include <mqueue.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "logger.h"
#include "macros.h"
#include <threads.h>

#define MAX_MSG_SIZE sizeof(emergency_request_t)

/**
 * @brief Struttura per passare gli argomenti al thread ricevitore della message queue.
 */
struct mq_receiver_args {
    emergency_type_t*   emergency_types;
    int                 emergency_count;
    env_config_t*       env_data;
};

/**
 * @brief Funzione eseguita dal thread ricevitore della message queue.
 * Riceve richieste di emergenza, valida i dati e le inserisce nella coda interna.
 * @param arg Puntatore a struct mq_receiver_args.
 * @return NULL.
 */
int mq_receiver_thread(void* arg) {
    mqd_t mq;
    emergency_request_t req;

    // Estrae gli argomenti passati al thread dalla struttura mq_receiver_args
    struct mq_receiver_args* args = (struct mq_receiver_args*)arg;
    emergency_type_t* emergency_types = args->emergency_types; // Array dei tipi di emergenza
    int emergency_count = args->emergency_count;               // Numero di tipi di emergenza
    env_config_t* env_data = args->env_data;                   // Configurazione ambiente
    free(arg); // Libera la memoria allocata per gli argomenti

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;  // massimo 10 messaggi in coda
    attr.mq_msgsize = sizeof(emergency_request_t);
    attr.mq_curmsgs = 0;

    // Prepara il nome della coda
    char* queue_name = malloc((strlen(env_data->queue) + 2) * sizeof(char));
    CHECK_MALLOC(queue_name, fail);
    snprintf(queue_name, strlen(env_data->queue) + 2, "/%s", env_data->queue);

    mq_unlink(queue_name); // Rimuove la coda se esiste già
    mq = mq_open(queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    CHECK_MQ_OPEN(mq, queue_name);
    free(queue_name); // Libera la memoria allocata per il nome della coda

    int id = 0;  // ID dell'emergenza
    while (1) {
        ssize_t bytes = mq_receive(mq, (char*)&req, MAX_MSG_SIZE, NULL);
        if (bytes > 0) {
            printf("📨 [MQ] Ricevuta emergenza: %s (%d,%d)\n", req.emergency_name, req.x, req.y);
            // Logga l'evento di ricezione
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Ricevuta emergenza: %s luogo:(%d,%d) ora:%ld", req.emergency_name, req.x, req.y, (long)&req.timestamp);
            log_event("0110", "MESSAGE_QUEUE", log_msg);

            // Controlla se le coordinate sono valide
            if (req.x < 0 || req.x >= env_data->width || req.y < 0 || req.y >= env_data->height) {
                fprintf(stderr, "❌ Coordinate non valide: (%d,%d)\n", req.x, req.y);
                // Logga l'errore di coordinate non valide
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Coordinate non valide: (%d,%d)", req.x, req.y);
                log_event("1120", "MESSAGE_QUEUE", log_msg);
                continue;
            }

            // Controlla se il tipo di emergenza è valido
            int i =0;
            for (i = 0; i < emergency_count; ++i) {
                if (strcmp(req.emergency_name, emergency_types[i].emergency_desc) == 0) {
                    break;
                }
            }
            if (i == emergency_count) {
                fprintf(stderr, "❌ Tipo di emergenza non riconosciuto: %s\n", req.emergency_name);
                // Logga l'errore di tipo non riconosciuto
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Tipo di emergenza non riconosciuto: %s", req.emergency_name);
                log_event("1120", "MESSAGE_QUEUE", log_msg);
                continue;
            }else {
                printf("📨 [MQ] ✅ Tipo di emergenza riconosciuto: %s\n", req.emergency_name);
                // Logga il riconoscimento del tipo di emergenza
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Tipo di emergenza riconosciuto: %s", req.emergency_name);
                log_event("0120", "MESSAGE_QUEUE", log_msg);
                // Alloca e inizializza la struttura emergency_t
                emergency_t* em = malloc(sizeof(emergency_t));
                CHECK_MALLOC(em, fail);
                memset(em, 0, sizeof(emergency_t));
                em->type = emergency_types[i];
                em->x = req.x;
                em->y = req.y;
                em->status = WAITING;
                // Calcola il numero totale di soccorritori richiesti
                em->rescuer_count = 0;
                for(int j = 0; j < emergency_types[i].rescuers_req_number; ++j) {
                    em->rescuer_count += emergency_types[i].rescuers[j].required_count;
                }
                em->rescuers_dt = malloc(em->rescuer_count * sizeof(rescuer_digital_twin_t));
                em->id = id++; // Assegna un ID univoco all'emergenza
                mtx_init(&em->mutex, mtx_plain); // Inizializza il mutex
                emergency_queue_add(em);    // Aggiunge l'emergenza alla coda interna
            }

        } else {
            perror("❌ mq_receive");
            sleep(1);
        }
    }

    

fail:
    mq_close(mq);
    return 0;
}

/**
 * @brief Avvia il thread ricevitore della message queue.
 * @param emergency_types Array dei tipi di emergenza.
 * @param emergency_count Numero di tipi di emergenza.
 * @param env_data Puntatore alla configurazione ambiente.
 * @param thread Puntatore al thread da avviare.
 */
void start_mq_receiver_thread(emergency_type_t emergency_types[], int emergency_count, env_config_t* env_data, thrd_t* thread) {

    struct mq_receiver_args* args = malloc(sizeof(struct mq_receiver_args));
    CHECK_MALLOC(args, fail);
    args->emergency_types = emergency_types;
    args->emergency_count = emergency_count;
    args->env_data = env_data;

    printf("📨 [MQ] Avvio thread ricevitore coda: /%s\n", env_data->queue);
    thrd_create(thread, mq_receiver_thread, args);
    return;
    fail:
    free(args);
}
