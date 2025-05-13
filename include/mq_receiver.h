#ifndef MQ_RECEIVER_H
#define MQ_RECEIVER_H
#include "types.h"
#include <pthread.h>

/**
 * @brief Avvia il thread che riceve le emergenze dalla message queue POSIX.
 * 
 * Questo thread si occupa di ricevere le emergenze dalla coda e di
 * convertirle in strutture di emergenza. Le emergenze ricevute vengono
 * aggiunte alla coda delle emergenze per essere elaborate successivamente.
 * 
 * @param queue_name Il nome della coda da cui ricevere le emergenze.
 * @param thread Il puntatore al thread che verrà creato.
 */
void start_mq_receiver_thread(emergency_type_t emergency_types[], int emergency_count, env_config_t* env_data, pthread_t* thread);

#endif // MQ_RECEIVER_H