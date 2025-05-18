#include "types.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "emergency_status.h"

/**
 * @brief Restituisce una stringa rappresentativa dello stato del soccorritore.
 * @param status Stato del soccorritore.
 * @return Stringa costante con il nome dello stato.
 */
char* stato(rescuer_status_t status) {
    switch (status) {
        case IDLE: return "IDLE";
        case EN_ROUTE_TO_SCENE: return "EN_ROUTE_TO_SCENE";
        case ON_SCENE: return "ON_SCENE";
        case RETURNING_TO_BASE: return "RETURNING_TO_BASE";
        default: return "UNKNOWN_STATUS";
    }
}

/**
 * @brief Funzione eseguita dal thread del soccorritore.
 * Gestisce il ciclo di vita del soccorritore: attesa, viaggio, intervento e ritorno.
 * @param arg Puntatore a rescuer_thread_t.
 * @return NULL.
 */
void* rescuer_thread(void* arg) {
    // Cast del parametro arg a un puntatore a struttura rescuer_thread_t
    rescuer_thread_t* wrapper = (rescuer_thread_t*)arg; 
    rescuer_digital_twin_t* r = wrapper->twin;

    while (1) {
        pthread_mutex_lock(&wrapper->mutex);
        // Attende di essere assegnato a una nuova emergenza
        while (r->status == IDLE) {
            pthread_cond_wait(&wrapper->cond, &wrapper->mutex);
        }
        pthread_mutex_unlock(&wrapper->mutex);

        emergency_t* current_em = wrapper->current_em;

        // Calcola il tempo di viaggio verso il luogo dell'emergenza (distanza Manhattan / velocità)
        int travel_time = ( abs(r->x - current_em->x) + abs(r->y - current_em->y) ) / r->rescuer->speed;
        if(travel_time == 0) travel_time = 1; // per evitare viaggi istantanei

        // Trova l'indice della richiesta di soccorritore corrispondente al tipo
        int index = -1;
        for (int i = 0; i < current_em->type.rescuers_req_number; i++) {
            if (strcmp(current_em->type.rescuers[i].type->rescuer_type_name,
                    r->rescuer->rescuer_type_name) == 0) {
                index = i;
                break;
            }
        }
        // Tempo di intervento specifico per il tipo di soccorritore
        int emergency_time = current_em->type.rescuers[index].time_to_manage;

        pthread_mutex_lock(&wrapper->mutex);
        // Aggiorna stato: partenza verso il luogo dell'emergenza
        r->status = EN_ROUTE_TO_SCENE;
        printf("🦺 [RESCUER] 🚀 [(%s) (%s)] Partenza verso il luogo dell'emergenza (%d,%d) -> (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, stato(r->status), current_em->x, current_em->y, r->x, r->y, travel_time);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "[(%s) (%s) (%d,%d) (%d)] Partenza verso il luogo dell'emergenza (%d,%d) -> (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, stato(r->status), current_em->x, current_em->y, travel_time, r->x, r->y, current_em->x, current_em->y,travel_time);
        char id [4];
        snprintf(id, sizeof(id), "0%02d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(travel_time); // Simula il tempo di viaggio

        // Simula intervento: aggiorna posizione e stato, notifica l'inizio dell'intervento
        r->x = current_em->x;
        r->y = current_em->y;
        r->status = ON_SCENE;
        update_emergency_status(current_em, IN_PROGRESS);
        printf("🦺 [RESCUER] 🚨 [%s #%d] Intervento in corso a (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, r->id, r->x, r->y, emergency_time);

        snprintf(log_msg, sizeof(log_msg), "[(%s) (%s) (%d,%d) (%d)] Intervento in corso a (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, stato(r->status), r->x, r->y , emergency_time, r->x, r->y, emergency_time);
        snprintf(id, sizeof(id), "0%02d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(emergency_time); // Simula il tempo di intervento        

        //Riritorno alla base
        r->x = r->rescuer->x;
        r->y = r->rescuer->y;
        r->status = RETURNING_TO_BASE;
        update_emergency_status(current_em, COMPLETED);
    
        printf("🦺 [RESCUER] 🏡 [%s #%d] Rientrato alla base (%d,%d) -> (%d,%d) in %d sec.\n",
            r->rescuer->rescuer_type_name, r->id,current_em->x, current_em->y, r->x, r->y, travel_time);
        snprintf(log_msg, sizeof(log_msg), "[(%s) (%s) (%d,%d) (%d)] Rientrato alla base (%d,%d) -> (%d,%d) in %d sec.",
            r->rescuer->rescuer_type_name, stato(r->status), r->x, r->y, travel_time ,current_em->x, current_em->y, r->x, r->y, travel_time);
        snprintf(id, sizeof(id), "0%02d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        sleep(travel_time); // Simula il tempo di viaggio di ritorno
        
        // Completa e torna IDLE
        r->status = IDLE;
        printf("🦺 [RESCUER] ✅ [%s #%d] Intervento completato.\n", r->rescuer->rescuer_type_name, r->id);
        snprintf(log_msg, sizeof(log_msg), "[(%s) (%s)] Intervento completato.", r->rescuer->rescuer_type_name, stato(r->status));
        snprintf(id, sizeof(id), "0%02d", r->id);
        log_event(id, "RESCUER_STATUS", log_msg);
        pthread_mutex_unlock(&wrapper->mutex);
        
        wrapper->current_em = NULL;
    }

    return NULL;
}

/**
 * @brief Inizializza e avvia il thread del soccorritore.
 * @param rescuer_wrapped Puntatore a rescuer_thread_t da avviare.
 */
void start_rescuer(rescuer_thread_t* rescuer_wrapped) {
    pthread_mutex_init(&rescuer_wrapped->mutex, NULL);
    pthread_cond_init(&rescuer_wrapped->cond, NULL);
    pthread_create(&rescuer_wrapped->thread, NULL, rescuer_thread, rescuer_wrapped);
}
