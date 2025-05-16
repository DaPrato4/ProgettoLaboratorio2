#include "types.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "logger.h"
#include <stdlib.h>

// Numero massimo di emergenze che la coda può contenere
#define MAX_EMERGENCIES 100

// Coda circolare di emergenze
static emergency_t* emergency_queue[MAX_EMERGENCIES];  // array circolare per le emergenze

// Indici per la testa (head), la coda (tail) e il conteggio degli elementi (count)
static int head = 0, tail = 0, count = 0;             // indici e conteggio

// Mutex per la mutua esclusione nell'accesso alla coda
static pthread_mutex_t queue_mutex;

// Variabile di condizione per notificare la presenza di nuove emergenze
static pthread_cond_t queue_not_empty;

/**
 * @brief Inizializza la coda delle emergenze, mutex e variabili di condizione.
 * 
 * Va chiamata una sola volta prima di utilizzare le funzioni add/get.
 * Inizializza mutex e variabile di condizione, e azzera gli indici della coda.
 */
void emergency_queue_init() {
    pthread_mutex_init(&queue_mutex, NULL);      // Inizializza il mutex
    pthread_cond_init(&queue_not_empty, NULL);   // Inizializza la variabile di condizione
    head = tail = count = 0;                     // Reset degli indici e del conteggio
}

/**
 * @brief Aggiunge un'emergenza alla coda.
 * 
 * Se la coda è piena, l'emergenza viene scartata e viene stampato un messaggio di errore.
 * La funzione è thread-safe grazie all'uso del mutex.
 * Dopo aver aggiunto un elemento, segnala eventuali thread in attesa.
 * 
 * @param e L'emergenza da aggiungere alla coda.
 */
void emergency_queue_add(emergency_t* e) {
    pthread_mutex_lock(&queue_mutex);    // Acquisisce il mutex per l'accesso esclusivo

    // Controlla se la coda è piena
    if (count == MAX_EMERGENCIES) {
        fprintf(stderr, "[queue] Errore: coda piena, emergenza scartata!\n");
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Errore: coda piena, emergenza scartata!");
        char id [4];
        snprintf(id, sizeof(id), "1%02d", e->id);
        log_event(id, "MESSAGE_QUEUE", log_msg); // Logga lo scarto dell'emergenza
        pthread_mutex_unlock(&queue_mutex);  // Rilascia il mutex prima di uscire
        return;
    }

    char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Emergenza (%s) correttamente aggiunta alla coda", e->type.emergency_desc);
        char id [4];
        snprintf(id, sizeof(id), "0%02d", e->id);
        log_event(id, "MESSAGE_QUEUE", log_msg); // Logga L'aggiunta dell'emergenza

    // Inserisce l'emergenza in coda e aggiorna l'indice di tail
    emergency_queue[tail] = e;
    tail = (tail + 1) % MAX_EMERGENCIES; // Gestione circolare dell'indice
    count++;                             // Incrementa il conteggio degli elementi


    //stampa la coda
    printf("[queue] Aggiunta emergenza: %s (%d,%d)\n", e->type.emergency_desc, e->x, e->y);
    printf("[queue] Coda attuale: %d emergenze\n", count);
    for (int i = 0; i < count; i++) {
        int index = (head + i) % MAX_EMERGENCIES;
        printf("[queue] [%d] %s (%d,%d) stato [%d] id[%d]\n", i, emergency_queue[index]->type.emergency_desc, emergency_queue[index]->x, emergency_queue[index]->y, emergency_queue[index]->status, emergency_queue[index]->id);
        //stampa tutti i dati dell'emergenza
        // printf("[queue] [emergenza] %s\n", emergency_queue[index].type.emergency_desc);
        // printf("[queue] [status] %d\n", emergency_queue[index].status);
        // printf("[queue] [x] %d\n", emergency_queue[index].x);
        // printf("[queue] [y] %d\n", emergency_queue[index].y);
        // printf("[queue] [time] %ld\n", emergency_queue[index].time);
        // printf("[queue] [rescuer_count] %d\n", emergency_queue[index].rescuer_count);
        // printf("[queue] [rescuer_dt] %p\n", emergency_queue[index].rescuers_dt);
        // printf("[queue] ------------------------\n");
    }

    
    pthread_cond_signal(&queue_not_empty); // Notifica i thread in attesa che la coda non è più vuota
    pthread_mutex_unlock(&queue_mutex);    // Rilascia il mutex
}

/**
 * @brief Estrae un'emergenza dalla coda.
 * 
 * Se la coda è vuota, il thread si blocca fino a quando non viene aggiunta un'emergenza.
 * La funzione è thread-safe grazie all'uso del mutex e della variabile di condizione.
 * 
 * @return emergency_t L'emergenza estratta dalla coda.
 */
emergency_t* emergency_queue_get() {
    emergency_t* e = malloc(sizeof(emergency_t)); // Alloca memoria per l'emergenza
    while (1){  // Ciclo infinito per attendere un'emergenza
    
        pthread_mutex_lock(&queue_mutex);    // Acquisisce il mutex per l'accesso esclusivo

        // Attende finché la coda è vuota
        while (count == 0) {
            pthread_cond_wait(&queue_not_empty, &queue_mutex); // Attende una segnalazione
            printf("numero di emergenze in coda: %d\n", count);
        }
        printf("numero di emergenze in coda: %d\n", count);

        // Estrae l'emergenza con priorità più alta
        
        short max_priority = -1;
        int max_index = -1;
        for (int i = 0; i < count; i++){
            int index = (head + i) % MAX_EMERGENCIES; // Calcola l'indice circolare
            if(emergency_queue[index]->type.priority > max_priority && emergency_queue[index]->status == WAITING) {
                max_priority = emergency_queue[index]->type.priority;
                max_index = index;
            }
        }
        if (max_index != -1) {
            e = emergency_queue[max_index]; // Estrae l'emergenza con priorità più alta
            // Rimuove l'emergenza dalla coda
            for (int i = max_index; i != tail; i = (i + 1) % MAX_EMERGENCIES) {
                int next = (i + 1) % MAX_EMERGENCIES;
                if (next != tail)
                    emergency_queue[i] = emergency_queue[next];
            }
            tail = (tail - 1 + MAX_EMERGENCIES) % MAX_EMERGENCIES; // Gestione circolare dell'indice
            count--; // Decrementa il conteggio degli elementi
            pthread_mutex_unlock(&queue_mutex);  // Rilascia il mutex
            return e;                            // Restituisce l'emergenza estratta
        }

        // Nessuna emergenza con status WAITING trovata → aspetta ancora
        pthread_mutex_unlock(&queue_mutex);
        // usleep(500000); // Aspetta mezzo secondo prima di riprovare
    }

}
