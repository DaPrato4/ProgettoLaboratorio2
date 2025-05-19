#include "scheduler.h"
#include "emergency_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"
#include "emergency_status.h"
#include "macros.h"

/**
 * @brief Funzione eseguita dal thread scheduler.
 * Estrae emergenze dalla coda, valuta se possono essere gestite e assegna i soccorritori disponibili.
 * @param arg Puntatore a scheduler_args_t.
 * @return NULL.
 */
void* scheduler_thread_fun(void* arg) {
    scheduler_args_t* args = (scheduler_args_t*)arg;
    rescuer_thread_t* rescuers = args->rescuers;
    int rescuer_count = args->rescuer_count;

    while (1) {
        // 1. Attende ed estrae emergenza con priorità più alta (gestisce il mutex internamente)
        emergency_t* e = emergency_queue_get();

        printf("🧭 [SCHEDULER] Emergenza da gestire: %s (%d,%d), priorità %d\n",
               e->type.emergency_desc, e->x, e->y, e->type.priority);

        // Controlla se l'emergenza può essere gestita in tempo in base alla priorità
        int max_time = 0;
        int time_to_manage = 0;
        switch (e->type.priority)
        {
        case 0:
            max_time = -1; // priorità 0, non ha un tempo massimo
            break;
        case 1:
            max_time = 30; // priorità 1, tempo massimo 30 secondi
            break;
        case 2:
            max_time = 10; // priorità 2, tempo massimo 10 secondi
            break;
        default:
            printf("❌ [SCHEDULER] Priorità non valida: %d\n", e->type.priority);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Priorità non valida: %d", e->type.priority);
            char id [5];
            snprintf(id, sizeof(id), "1%3d", e->id);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
            update_emergency_status(e, CANCELED); // Aggiorna lo stato dell'emergenza
            break;
        }

        // Calcola il tempo massimo necessario per la gestione dell'emergenza
        for (int i = 0; i < e->type.rescuers_req_number; i++) {
            rescuer_request_t req = e->type.rescuers[i];
            int travel_time = ( abs(e->x - req.type->x) + abs(e->y - req.type->y) ) / req.type->speed;
            if(req.time_to_manage + travel_time > time_to_manage) {
                time_to_manage = req.time_to_manage + travel_time;
            }
        }
        e->time=time_to_manage; // Salva il tempo stimato per la gestione dell'emergenza
        // Se il tempo di gestione supera il massimo, scarta l'emergenza
        printf("🧭 [SCHEDULER] Tempo di gestione stimato: %d secondi\n", time_to_manage);
        if (max_time > 0 && time_to_manage > max_time) {
            printf("❌ [SCHEDULER] Emergenza scartata: %s (%d,%d), tempo massimo superato\n",
                   e->type.emergency_desc, e->x, e->y);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Emergenza scartata: %s (%d,%d), richiesti %d secondi per la gestione (priorità %d)",
                   e->type.emergency_desc, e->x, e->y, time_to_manage, e->type.priority);
            char id [5];
            snprintf(id, sizeof(id), "1%03d", e->id);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
            update_emergency_status(e, TIMEOUT); // Aggiorna lo stato dell'emergenza
            continue; // Passa alla prossima emergenza
        }

        // 2. Per ogni tipo di soccorritore richiesto da questa emergenza
        int assigned = 0;
        int total_needed = 0;
        // Array dinamico per i soccorritori selezionati
        rescuer_digital_twin_t** digital_twins_selected = NULL;
        for (int i = 0; i < e->type.rescuers_req_number; i++) {
            rescuer_request_t req = e->type.rescuers[i];
            // Numero di soccorritori richiesti di un certo tipo
            int needed = req.required_count;
            total_needed += needed;
            rescuer_digital_twin_t** temp = realloc(digital_twins_selected, total_needed * sizeof(rescuer_digital_twin_t));
            if (temp == NULL) {
                perror("❌ Errore realloc");
                // free(digital_twins_selected);
                digital_twins_selected = NULL;
                return NULL; // O gestisci l'errore
            }
            digital_twins_selected = temp;

            // 3. Cerca i soccorritori disponibili del tipo richiesto
            for (int j = 0; j < rescuer_count && needed > 0; j++) {
                rescuer_digital_twin_t* r = rescuers[j].twin;

                // Cerca un soccorritore del tipo richiesto, libero
                if (r->status == IDLE &&
                    strcmp(r->rescuer->rescuer_type_name, req.type->rescuer_type_name) == 0) {
                        digital_twins_selected[assigned] = r; // Salva il soccorritore selezionato
                        assigned++;
                        needed--;
                }
            }
            
            if (needed > 0) {
                // Se non ci sono abbastanza soccorritori disponibili, scarta l'emergenza
                printf("❌ [SCHEDULER] Non ci sono abbastanza soccorritori disponibili per: %s\n",
                    req.type->rescuer_type_name);
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Non ci sono abbastanza soccorritori disponibili per: %s",
                    req.type->rescuer_type_name);
                char id [5];
                snprintf(id, sizeof(id), "1%03d", e->id);
                log_event(id, "EMERGENCY_SCHEDULER", log_msg);
                update_emergency_status(e, TIMEOUT); // Aggiorna lo stato dell'emergenza
                free(digital_twins_selected);
                break; // riprendi dal ciclo while
            }

            // Se sono stati assegnati tutti i soccorritori richiesti
            if (needed == 0) {
                printf("🧭 [SCHEDULER] %d soccorritori del tipo %s disponibili\n",
                       req.required_count, req.type->rescuer_type_name);
            }  
        }

        if (assigned == total_needed) {
            // 4. Assegna i soccorritori all'emergenza
            e->rescuers_dt = digital_twins_selected;

            char rescuers_assigned[64] = "";
            printf("✅ [SCHEDULER] Assegnati %d soccorritori all'emergenza: %s (id: %02d)\n",
                   assigned, e->type.emergency_desc, e->id);
            // Risveglia i soccorritori assegnati e aggiorna i loro stati
            for (int j = 0; j < assigned; j++) {
                rescuer_digital_twin_t* r = digital_twins_selected[j];
                pthread_mutex_lock(&rescuers[r->id].mutex);
                r->status = EN_ROUTE_TO_SCENE;
                rescuers[r->id].current_em = e;
                pthread_mutex_unlock(&rescuers[r->id].mutex);
                pthread_cond_signal(&rescuers[r->id].cond);
                strcat(rescuers_assigned, rescuers[r->id].twin->rescuer->rescuer_type_name);
                if(j != assigned-1) strcat(rescuers_assigned, ", ");
            }
            update_emergency_status(e, ASSIGNED); // Aggiorna lo stato dell'emergenza
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Assegnati %d soccorritori (%s) all'emergenza: %s",
                   assigned,rescuers_assigned ,e->type.emergency_desc);
            char id [5];
            snprintf(id, sizeof(id), "0%03d", e->x);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
        }

    }

    return NULL;

}
