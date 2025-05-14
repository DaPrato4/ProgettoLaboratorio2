#include "types.h"
#include "logger.h"
#include <pthread.h>

//Aggiungo un mutex alla struct di emergenza per gestire l' accesso concorrente senza dover bloccare tutti i soccorritori

void update_emergency_status(emergency_t* em, emergency_status_t new_status) {
    pthread_mutex_lock(&em->mutex); // Acquisisce il mutex per l'accesso esclusivo
    int status = 0; // Inizializza lo stato a 0
    char id[3];
    switch (new_status)
    {
    case ASSIGNED:
        status = 0;
        for (int i = 0; i < em->rescuer_count; i++) {
            if (em->rescuers_dt[i] == NULL) {
                status = 1;
                break;
            }
        }
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "%d", em->id);
            log_event(id, "EMERGENCY_STATUS", "Stato di emergenza aggiornato (ASSIGNED)");
        }
        break;
    case IN_PROGRESS:
        status = 0;
        for (int i = 0; i < em->rescuer_count; i++) {
            if (em->rescuers_dt[i]->status == EN_ROUTE_TO_SCENE) {
                status = 1;
                break;
            }
        }
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "%d", em->id);
            log_event(id, "EMERGENCY_STATUS", "Stato di emergenza aggiornato (IN_PROGRESS)");
        }
        break;
    case COMPLETED:
        status = 0;
        for (int i = 0; i < em->rescuer_count; i++) {
            if (em->rescuers_dt[i]->status == ON_SCENE || em->rescuers_dt[i]->status == EN_ROUTE_TO_SCENE) {
                status = 1;
                break;
            }
        }
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "%d", em->id);
            log_event(id, "EMERGENCY_STATUS", "Stato di emergenza aggiornato (COMPLETED)");
        }
        break;
    case TIMEOUT:
        em->status = new_status;
        snprintf(id, sizeof(id), "%d", em->id);
        log_event(id, "EMERGENCY_STATUS", "Stato di emergenza aggiornato (TIMEOUT)");
        break;
    case CANCELED:
        em->status = new_status;
        snprintf(id, sizeof(id), "%d", em->id);
        log_event(id, "EMERGENCY_STATUS", "Stato di emergenza aggiornato (CANCELED)");
        break;
    default:
        log_event("ID_EMERGENZA", "EMERGENCY_STATUS", "Stato di emergenza non valido");
        break;
    }
    pthread_mutex_unlock(&em->mutex); // Rilascia il mutex
}