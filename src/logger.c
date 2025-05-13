#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Dimensione massima della coda dei messaggi di log
#define LOG_QUEUE_SIZE 1024

// Struttura che rappresenta un messaggio di log
typedef struct {
    char id[32];         // ID associato all'evento (es. emergenza, soccorritore, ecc.)
    char event[32];      // Categoria dell'evento (es. EMERGENCY_STATUS, FILE_PARSING, ecc.)
    char message[256];   // Messaggio descrittivo dell'evento
} log_msg_t;

// Coda circolare per i messaggi di log
static log_msg_t log_queue[LOG_QUEUE_SIZE];
// Indici di testa e coda della coda circolare
static int log_head = 0, log_tail = 0;
// Mutex per proteggere l'accesso concorrente alla coda
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
// Condition variable per notificare il thread logger della presenza di nuovi messaggi
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;
// Flag che indica se il logger deve continuare a funzionare
static int logger_running = 1;
// Variabile per il thread logger
static pthread_t logger_thread;

// Funzione eseguita dal thread logger: estrae messaggi dalla coda e li scrive su file
static void* logger_func(void* arg) {
    FILE* f = fopen("system.log", "w"); // Apre il file di log in modalità append
    if (!f) return NULL; // Se non riesce ad aprire il file, termina il thread
    while (logger_running || log_head != log_tail) { // Continua finché il logger è attivo o ci sono messaggi da scrivere
        pthread_mutex_lock(&log_mutex); // Blocca il mutex per accedere alla coda
        while (log_head == log_tail && logger_running) { // Se la coda è vuota e il logger è attivo, aspetta nuovi messaggi
            pthread_cond_wait(&log_cond, &log_mutex);
        }
        while (log_head != log_tail) { // Finché ci sono messaggi nella coda
            log_msg_t* msg = &log_queue[log_tail]; // Prende il messaggio in testa alla coda
            long timestamp = (long)time(NULL); // Ottiene il timestamp corrente
            // Scrive il messaggio di log nel file con il formato richiesto
            fprintf(f, "[%ld] [%s] [%s] %s\n", timestamp, msg->id, msg->event, msg->message);
            log_tail = (log_tail + 1) % LOG_QUEUE_SIZE; // Avanza la coda
        }
        fflush(f); // Forza la scrittura su disco
        pthread_mutex_unlock(&log_mutex); // Sblocca il mutex
    }
    fclose(f); // Chiude il file di log
    return NULL;
}

// Avvia il thread logger
void start_logger_thread() {
    pthread_create(&logger_thread, NULL, logger_func, NULL);
}

// Ferma il thread logger e attende la sua terminazione
void stop_logger_thread() {
    pthread_mutex_lock(&log_mutex);
    logger_running = 0; // Imposta il flag per terminare il logger
    pthread_cond_signal(&log_cond); // Risveglia il thread logger se in attesa
    pthread_mutex_unlock(&log_mutex);
    pthread_join(logger_thread, NULL); // Attende la terminazione del thread logger
}

// Inserisce un nuovo messaggio di log nella coda
void log_event(const char* id, const char* event, const char* message) {
    pthread_mutex_lock(&log_mutex); // Blocca il mutex per accedere alla coda
    int next_head = (log_head + 1) % LOG_QUEUE_SIZE; // Calcola la prossima posizione della testa
    if (next_head != log_tail) { // Se la coda non è piena
        // Copia i dati del messaggio nella coda
        strncpy(log_queue[log_head].id, id, 31);
        strncpy(log_queue[log_head].event, event, 31);
        strncpy(log_queue[log_head].message, message, 255);
        log_queue[log_head].id[31] = 0;
        log_queue[log_head].event[31] = 0;
        log_queue[log_head].message[255] = 0;
        log_head = next_head; // Avanza la testa della coda
        pthread_cond_signal(&log_cond); // Notifica il thread logger della presenza di un nuovo messaggio
    }
    pthread_mutex_unlock(&log_mutex); // Sblocca il mutex
}