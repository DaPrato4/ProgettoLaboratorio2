#include "scheduler.h"
#include "emergency_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void* scheduler_thread_fun(void* arg) {
    scheduler_args_t* args = (scheduler_args_t*)arg;
    rescuer_thread_t* rescuers = args->rescuers;
    int rescuer_count = args->rescuer_count;

    while (1) {
        // 1. Attende ed estrae emergenza con priorità più alta (gestisce il mutex internamente)
        emergency_t e = emergency_queue_get();

        printf("🧭 [SCHEDULER] Emergenza da gestire: %s (%d,%d), priorità %d\n",
               e.type.emergency_desc, e.x, e.y, e.type.priority);


        // 2. Per ogni tipo di soccorritore richiesto da questa emergenza
        int assigned = 0;
        int total_needed = 0;
        rescuer_digital_twin_t** digital_twins_selected = malloc(0);
        for (int i = 0; i < e.type.rescuers_req_number; i++) {
            rescuer_request_t req = e.type.rescuers[i];
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

                e.status = TIMEOUT;
                emergency_queue_add(e); // la reinserisci in coda come TIMEOUT
                free(digital_twins_selected); // libera memoria
                //sleep(1); // evita ciclo infinito e stress alla CPU
                break; // riprendi dal ciclo while
            }

            // Se sono stati assegnati tutti i soccorritori richiesti
            if (needed == 0) {
                printf("[SCHEDULER] %d soccorritori del tipo %s disponibili\n",
                       req.required_count, req.type->rescuer_type_name);
            } else {
                printf("❌ [SCHEDULER] Non ci sono abbastanza soccorritori disponibili per: %s\n",
                       req.type->rescuer_type_name);
            }   

        }

        if (assigned == total_needed) {
            // 4. Assegna i soccorritori all'emergenza
            e.rescuers_dt = digital_twins_selected;
            //risveglia i soccorritori
            for (int j = 0; j < assigned; j++) {
                rescuer_digital_twin_t* r = digital_twins_selected[j];
                pthread_mutex_lock(&rescuers[r->id].mutex);
                r->status = EN_ROUTE_TO_SCENE;
                printf("🆕 [SCHEDULER] Twin ID %d (addr: %p) aggiornato a stato %d\n", r->id, (void*)r, r->status);
                rescuers[r->id].current_em = &e;
                pthread_mutex_unlock(&rescuers[r->id].mutex);
                pthread_cond_signal(&rescuers[r->id].cond);
            }
            e.status = ASSIGNED;
            printf("✅ [SCHEDULER] Assegnati %d soccorritori all’emergenza: %s\n",
                   assigned, e.type.emergency_desc);
        }

        // Attendi un attimo prima di gestire la prossima
        usleep(200000); // 0.2s per non sovraccaricare
    }

    return NULL;
}
