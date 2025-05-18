#include "types.h"
#include "logger.h"
#include <pthread.h>
#include <stdlib.h>

//Aggiungo un mutex alla struct di emergenza per gestire l' accesso concorrente senza dover bloccare tutti i soccorritori

/**
 * @brief Aggiorna lo stato di una emergenza in modo thread-safe e gestisce la transizione di stato.
 * 
 * @param em Puntatore alla struttura emergency_t da aggiornare.
 * @param new_status Nuovo stato da impostare (ASSIGNED, IN_PROGRESS, COMPLETED, TIMEOUT, CANCELED).
 * 
 * Questa funzione si occupa di:
 * - Aggiornare lo stato dell'emergenza solo se le condizioni sono soddisfatte.
 * - Loggare ogni transizione di stato.
 * - Liberare la memoria dell'emergenza se lo stato finale lo richiede.
 * - Utilizzare un mutex per garantire la mutua esclusione sull'emergenza.
 */
void update_emergency_status(emergency_t* em, emergency_status_t new_status) {
    pthread_mutex_lock(&em->mutex); // Acquisisce il mutex per l'accesso esclusivo
    int status = 0; // Variabile di supporto per controlli di stato
    char id[5];
    switch (new_status)
    {
    case ASSIGNED:
        // Verifica che tutti i soccorritori siano assegnati (nessun NULL)
        status = 0;
        for (int i = 0; i < em->rescuer_count; i++) {
            if (em->rescuers_dt[i] == NULL) {
                status = 1;
                break;
            }
        }
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "0%03d", em->id);
            log_event(id, "EMERGENCY_STATUS", "[ASSIGNED] Stato di emergenza aggiornato");
        }
        break;
    case IN_PROGRESS:
        // Aggiorna lo stato solo se non già IN_PROGRESS
        status = (em->status == IN_PROGRESS);
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "0%03d", em->id);
            log_event(id, "EMERGENCY_STATUS", "[IN_PROGRESS] Stato di emergenza aggiornato");
        }
        break;
    case COMPLETED:
        // Verifica che nessun soccorritore sia ancora sulla scena o in viaggio
        status = 0;
        for (int i = 0; i < em->rescuer_count; i++) {
            if (em->rescuers_dt[i]->status == ON_SCENE || em->rescuers_dt[i]->status == EN_ROUTE_TO_SCENE) {
                status = 1;
                break;
            }
        }
        if (!status) {
            em->status = new_status;
            snprintf(id, sizeof(id), "0%03d", em->id);
            log_event(id, "EMERGENCY_STATUS", "[COMPLETED] Stato di emergenza aggiornato ");
            free(em); // Libera la memoria dell'emergenza completata
        }
        break;
    case TIMEOUT:
        // Gestione emergenza scaduta per timeout
        em->status = new_status;
        snprintf(id, sizeof(id), "1%03d", em->id);
        log_event(id, "EMERGENCY_STATUS", "[TIMEOUT] Stato di emergenza aggiornato");
        free(em);
        break;
    case CANCELED:
        // Gestione emergenza annullata
        em->status = new_status;
        snprintf(id, sizeof(id), "1%03d", em->id);
        log_event(id, "EMERGENCY_STATUS", "[CANCELED] Stato di emergenza aggiornato");
        free(em);
        break;
    default:
        // Gestione stato non valido
        snprintf(id, sizeof(id), "1%03d", em->id);
        log_event(id, "EMERGENCY_STATUS", "Stato di emergenza non valido");
        free(em);
        break;
    }
    pthread_mutex_unlock(&em->mutex); // Rilascia il mutex
}