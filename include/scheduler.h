#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "rescuer.h"

/**
 * @brief Gestore delle emergenze.
 * 
 * Lo scheduler si occupa di assegnare le emergenze ai soccorritori disponibili.
 * 
 * @param arg Puntatore a una struttura contenente tutti i soccorritori e il numero degli stessi (scheduler_args_t).
 */
int scheduler_thread_fun(void* arg);

/**
 * Struct contenente dati da passare allo scheduler
 */
typedef struct {
    rescuer_thread_t* rescuers;
    int rescuer_count;
} scheduler_args_t;

#endif
