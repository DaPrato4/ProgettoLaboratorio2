#include "scheduler.h"
#include "emergency_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"
#include "emergency_status.h"

void* scheduler_thread_fun(void* arg) {
    scheduler_args_t* args = (scheduler_args_t*)arg;
    rescuer_thread_t* rescuers = args->rescuers;
    int rescuer_count = args->rescuer_count;

    while (1) {
        // 1. Attende ed estrae emergenza con priorità più alta (gestisce il mutex internamente)
        emergency_t e = emergency_queue_get();

        printf("🧭 [SCHEDULER] Emergenza da gestire: %s (%d,%d), priorità %d\n",
               e.type.emergency_desc, e.x, e.y, e.type.priority);

        // Controll se l' emergenza può essere gestita in tempo
        int max_time = 0;
        int time_to_manage = 0;
        switch (e.type.priority)
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
            printf("❌ [SCHEDULER] Priorità non valida: %d\n", e.type.priority);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Priorità non valida: %d", e.type.priority);
            char id [3];
            snprintf(id, sizeof(id), "%d", e.id);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
            update_emergency_status(&e, TIMEOUT); // Aggiorna lo stato dell'emergenza
            break;
        }

        // trovo il tempo di gestione dell'emergenza
        for (int i = 0; i < e.type.rescuers_req_number; i++) {
            rescuer_request_t req = e.type.rescuers[i];
            int travel_time = ( abs(e.x - req.type->x) + abs(e.y - req.type->y) ) / req.type->speed;
            if(req.time_to_manage + travel_time > time_to_manage) {
                time_to_manage = req.time_to_manage + travel_time;
            }
        }
        // Se il tempo di gestione supera il massimo, scarta l'emergenza
        if (max_time > 0 && time_to_manage > max_time) {
            printf("❌ [SCHEDULER] Emergenza scartata: %s (%d,%d), tempo massimo superato\n",
                   e.type.emergency_desc, e.x, e.y);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Emergenza scartata: %s (%d,%d), richiesti %d secondi per la gestione (priorità %d)",
                   e.type.emergency_desc, e.x, e.y, max_time, e.type.priority);
            char id [3];
            snprintf(id, sizeof(id), "%d", e.id);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
            update_emergency_status(&e, TIMEOUT); // Aggiorna lo stato dell'emergenza
            continue; // Passa alla prossima emergenza
        }


        // 2. Per ogni tipo di soccorritore richiesto da questa emergenza
        int assigned = 0;
        int total_needed = 0;
        rescuer_digital_twin_t** digital_twins_selected = malloc(0);
        for (int i = 0; i < e.type.rescuers_req_number; i++) {
            rescuer_request_t req = e.type.rescuers[i];
            // soccorritori riciesti di un certo tipo
            int needed = req.required_count;
            total_needed += needed;
            rescuer_digital_twin_t** temp = realloc(digital_twins_selected, total_needed * sizeof(rescuer_digital_twin_t));
            if (temp == NULL) {
                perror("Errore realloc");
                free(digital_twins_selected);
                return NULL; // O gestisci l'errore
            }
            digital_twins_selected = temp;

            //aggungo qui le prove
            
            // 3. Cerca i soccorritori disponibili
            for (int j = 0; j < rescuer_count && needed > 0; j++) {
                rescuer_digital_twin_t* r = rescuers[j].twin;

                // Cerca un soccorritore del tipo richiesto, libero
                if (r->status == IDLE &&
                    strcmp(r->rescuer->rescuer_type_name, req.type->rescuer_type_name) == 0) {
                        digital_twins_selected[assigned] = r; // Salva il soccorritore selezionato
                        assigned++;
                        needed--;
                    // pthread_cond_signal(&rescuers[j].cond);

                }
            }
            
            if (needed > 0) {
                printf("❌ [SCHEDULER] Non ci sono abbastanza soccorritori disponibili per: %s\n",
                    req.type->rescuer_type_name);
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Non ci sono abbastanza soccorritori disponibili per: %s",
                    req.type->rescuer_type_name);
                // char id [3];
                // snprintf(id, sizeof(id), "%d", r->id);
                log_event("ID_EMERGENZA", "EMERGENCY_SCHEDULER_TIMEOUT", log_msg);
                update_emergency_status(&e, TIMEOUT); // Aggiorna lo stato dell'emergenza
                emergency_queue_add(e); // la reinserisci in coda come TIMEOUT
                free(digital_twins_selected); // libera memoria
                break; // riprendi dal ciclo while
            }

            // Se sono stati assegnati tutti i soccorritori richiesti
            if (needed == 0) {
                printf("[SCHEDULER] %d soccorritori del tipo %s disponibili\n",
                       req.required_count, req.type->rescuer_type_name);
            }  

        }

        if (assigned == total_needed) {
            // 4. Assegna i soccorritori all'emergenza
            e.rescuers_dt = digital_twins_selected;

            char rescuers_assigned[64] = "";
            //risveglia i soccorritori
            for (int j = 0; j < assigned; j++) {
                rescuer_digital_twin_t* r = digital_twins_selected[j];
                pthread_mutex_lock(&rescuers[r->id].mutex);
                r->status = EN_ROUTE_TO_SCENE;
                // printf("🆕 [SCHEDULER] Twin ID %d (addr: %p) aggiornato a stato %d\n", r->id, (void*)r, r->status);
                rescuers[r->id].current_em = &e;
                pthread_mutex_unlock(&rescuers[r->id].mutex);
                pthread_cond_signal(&rescuers[r->id].cond);
                strcat(rescuers_assigned, rescuers[r->id].twin->rescuer->rescuer_type_name);
                if(j != assigned-1) strcat(rescuers_assigned, ", ");
            }
            update_emergency_status(&e, ASSIGNED); // Aggiorna lo stato dell'emergenza
            printf("✅ [SCHEDULER] Assegnati %d soccorritori all'emergenza: %s\n",
                   assigned, e.type.emergency_desc);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Assegnati %d soccorritori (%s) all'emergenza: %s",
                   assigned,rescuers_assigned ,e.type.emergency_desc);
            char id [3];
            snprintf(id, sizeof(id), "%d", e.x);
            log_event(id, "EMERGENCY_SCHEDULER", log_msg);
        }

        // Attendi un attimo prima di gestire la prossima
        usleep(200000); // 0.2s per non sovraccaricare
    }

    return NULL;
}
