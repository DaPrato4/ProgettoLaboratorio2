#include "types.h"
#include <pthread.h>
#include <stdio.h>

// Numero massimo di emergenze che la coda può contenere
#define MAX_EMERGENCIES 100

// Coda circolare di emergenze
static emergency_t emergency_queue[MAX_EMERGENCIES];  // array circolare per le emergenze

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
void emergency_queue_add(emergency_t e) {
    pthread_mutex_lock(&queue_mutex);    // Acquisisce il mutex per l'accesso esclusivo

    // Controlla se la coda è piena
    if (count == MAX_EMERGENCIES) {
        fprintf(stderr, "[queue] Errore: coda piena, emergenza scartata!\n");
        pthread_mutex_unlock(&queue_mutex);  // Rilascia il mutex prima di uscire
        return;
    }

    // Inserisce l'emergenza in coda e aggiorna l'indice di tail
    emergency_queue[tail] = e;
    tail = (tail + 1) % MAX_EMERGENCIES; // Gestione circolare dell'indice
    count++;                             // Incrementa il conteggio degli elementi

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
emergency_t emergency_queue_get() {
    pthread_mutex_lock(&queue_mutex);    // Acquisisce il mutex per l'accesso esclusivo

    // Attende finché la coda è vuota
    while (count == 0) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex); // Attende una segnalazione
    }

    // Estrae l'emergenza dalla testa della coda e aggiorna l'indice di head
    emergency_t e = emergency_queue[head];
    head = (head + 1) % MAX_EMERGENCIES; // Gestione circolare dell'indice
    count--;                             // Decrementa il conteggio degli elementi

    pthread_mutex_unlock(&queue_mutex);  // Rilascia il mutex
    return e;                            // Restituisce l'emergenza estratta
}
