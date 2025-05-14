#include "types.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "emergency_status.h"

char* stato(rescuer_status_t status) {
    switch (status) {
        case IDLE: return "IDLE";
        case EN_ROUTE_TO_SCENE: return "EN_ROUTE_TO_SCENE";
        case ON_SCENE: return "ON_SCENE";
        case RETURNING_TO_BASE: return "RETURNING_TO_BASE";
        default: return "UNKNOWN_STATUS";
    }
}


void* rescuer_thread(void* arg) {
    // Cast del parametro arg a un puntatore a struttura rescuer_thread_t
    rescuer_thread_t* wrapper = (rescuer_thread_t*)arg; 
    rescuer_digital_twin_t* r = wrapper->twin;

    while (1) {
        pthread_mutex_lock(&wrapper->mutex);
        // Aspetta di essere assegnato
        while (r->status == IDLE) {
            pthread_cond_wait(&wrapper->cond, &wrapper->mutex);
            // printf("🤷🤷[RESCUER %d] RISVEGLIATO (Twin addr: %p) con stato %d😢😲🤑\n", r->id, (void*)r, r->status);

        }
        pthread_mutex_unlock(&wrapper->mutex);

        emergency_t* current_em = wrapper->current_em;

        //Calcolo tempi di viaggio
        int travel_time = ( abs(r->x - current_em->x) + abs(r->y - current_em->y) ) / r->rescuer->speed;
        int index = -1;
        for (int i = 0; i < current_em->type.rescuers_req_number; i++) {
            if (strcmp(current_em->type.rescuers[i].type->rescuer_type_name,
                    r->rescuer->rescuer_type_name) == 0) {
                index = i;
                break;
            }
        }
        // tempo di intervento dell'emergenza
        int emergency_time = current_em->type.rescuers[index].time_to_manage;

        pthread_mutex_lock(&wrapper->mutex);
        //Partenza verso il luogo dell'emergenza
        r->status = EN_ROUTE_TO_SCENE;
        printf("🚀 [%s #%d] Partenza verso il luogo dell'emergenza (%d,%d) -> (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, r->id, r->x, r->y, current_em->x, current_em->y,travel_time);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "[%s #%d (%s)] Partenza verso il luogo dell'emergenza (%d,%d) -> (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, r->id, stato(r->status) ,r->x, r->y, current_em->x, current_em->y,travel_time);
        char id [3];
        snprintf(id, sizeof(id), "%d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(travel_time); // Simula il tempo di viaggio

        // Simula intervento
        // wrapper->current_em->status = IN_PROGRESS;
        update_emergency_status(current_em, IN_PROGRESS);
        r->x = current_em->x;
        r->y = current_em->y;
        r->status = ON_SCENE;
        printf("🚨 [%s #%d] Intervento in corso a (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, r->id, r->x, r->y, emergency_time);
        snprintf(log_msg, sizeof(log_msg), "[%s #%d (%s)] Intervento in corso a (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, r->id, stato(r->status), r->x, r->y, emergency_time);
        snprintf(id, sizeof(id), "%d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(emergency_time); // Simula il tempo di intervento
        // wrapper->current_em->status = COMPLETED;
        


        //Riritorno alla base
        r->x = r->rescuer->x;
        r->y = r->rescuer->y;
        r->status = RETURNING_TO_BASE;
        update_emergency_status(current_em, COMPLETED);
        printf("🏡 [%s #%d] Rientrato alla base (%d,%d) -> (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, r->id,current_em->x, current_em->y, r->x, r->y, travel_time);
        snprintf(log_msg, sizeof(log_msg), "[%s #%d (%s)] Rientrato alla base (%d,%d) -> (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, r->id, stato(r->status),current_em->x, current_em->y, r->x, r->y, travel_time);
        snprintf(id, sizeof(id), "%d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(travel_time); // Simula il tempo di viaggio di ritorno
        
        // Completa e torna IDLE
        r->status = IDLE;
        printf("✅ [%s #%d] Intervento completato.\n", r->rescuer->rescuer_type_name, r->id);
        snprintf(log_msg, sizeof(log_msg), "[%s #%d (%s)] Intervento completato.", r->rescuer->rescuer_type_name, r->id, stato(r->status));
        snprintf(id, sizeof(id), "%d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        pthread_mutex_unlock(&wrapper->mutex);
        
        wrapper->current_em = NULL;
    }

    return NULL;
}

void start_rescuer(rescuer_thread_t* rescuer_wrapped) {
    pthread_mutex_init(&rescuer_wrapped->mutex, NULL);
    pthread_cond_init(&rescuer_wrapped->cond, NULL);
    pthread_create(&rescuer_wrapped->thread, NULL, rescuer_thread, rescuer_wrapped);
}
