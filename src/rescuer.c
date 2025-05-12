#include "types.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>


void* rescuer_thread(void* arg) {
    // Cast del parametro arg a un puntatore a struttura rescuer_thread_t
    rescuer_thread_t* wrapper = (rescuer_thread_t*)arg; 
    rescuer_digital_twin_t* r = wrapper->twin;

    while (1) {
        pthread_mutex_lock(&wrapper->mutex);

        // Aspetta di essere assegnato
        while (r->status == IDLE) {
            pthread_cond_wait(&wrapper->cond, &wrapper->mutex);
            printf("🤷🤷[RESCUER %d] RISVEGLIATO (Twin addr: %p) con stato %d😢😲🤑\n", r->id, (void*)r, r->status);

        }

        pthread_mutex_unlock(&wrapper->mutex);

        // Simula intervento
        printf("🚨 [%s #%d] Intervento in corso a (%d,%d)...\n",
            r->rescuer->rescuer_type_name, r->id, r->x, r->y);

        // qui dovrà essere calcolato il tempo di intervento in cui stare on rout on scee e returning to base
        sleep(5); // Simula il tempo di intervento (3 secondi)

        // Completa e torna IDLE
        pthread_mutex_lock(&wrapper->mutex);
        r->status = IDLE;
        printf("✅ [%s #%d] Intervento completato.\n", r->rescuer->rescuer_type_name, r->id);
        pthread_mutex_unlock(&wrapper->mutex);
    }

    return NULL;
}

void start_rescuer(rescuer_thread_t* rescuer_wrapped) {
    pthread_mutex_init(&rescuer_wrapped->mutex, NULL);
    pthread_cond_init(&rescuer_wrapped->cond, NULL);
    pthread_create(&rescuer_wrapped->thread, NULL, rescuer_thread, rescuer_wrapped);
}
