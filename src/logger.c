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



// GESTIONE MESSAGGI TCP
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
int tcp_enabled = 0; // Flag per abilitare/disabilitare l'invio TCP
char* tcp_server_ip = "127.0.0.1"; // Indirizzo IP del server TCP
int tcp_server_port = 9000; // Porta del server TCP
int tcp_sock_fd = -1; // File descriptor per la socket TCP
void init_tcp_logger(); // Funzione per inizializzare la connessione TCP
void send_log_json(const log_msg_t* msg); // Funzione per inviare messaggi JSON al server TCP



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

            // INVIO TCP
            if (tcp_enabled) send_log_json(msg);

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

    //INIZIALIZZA TCP
    init_tcp_logger(); // Inizializza la connessione TCP al server
}

// Ferma il thread logger e attende la sua terminazione
void stop_logger_thread() {
    pthread_mutex_lock(&log_mutex);
    logger_running = 0; // Imposta il flag per terminare il logger
    pthread_cond_signal(&log_cond); // Risveglia il thread logger se in attesa
    pthread_mutex_unlock(&log_mutex);
    pthread_join(logger_thread, NULL); // Attende la terminazione del thread logger

    //CHIUSURA TCP
    close(tcp_sock_fd); // Chiude la socket TCP
    tcp_enabled = 0; // Disabilita l'invio TCP
    tcp_sock_fd = -1; // Resetta il file descriptor della socket TCP
}

/**
 * @brief Inserisce un nuovo messaggio di log nella coda.
 * @param id ID associato all'evento (es. emergenza, soccorritore, ecc.)
 * @param event Categoria dell'evento (es. EMERGENCY_STATUS, FILE_PARSING, ecc.)
 * @param message Messaggio descrittivo dell'evento
 */
void log_event(const char* id, const char* event, const char* message) {
    pthread_mutex_lock(&log_mutex); // Blocca il mutex per accedere alla coda
    int next_head = (log_head + 1) % LOG_QUEUE_SIZE; // Calcola la prossima posizione della testa
    if (next_head != log_tail) { // Se la coda non è piena
        // Copia i dati del messaggio nella coda circolare
        strncpy(log_queue[log_head].id, id, 31);
        strncpy(log_queue[log_head].event, event, 31);
        strncpy(log_queue[log_head].message, message, 255);
        log_queue[log_head].id[31] = 0;
        log_queue[log_head].event[31] = 0;
        log_queue[log_head].message[255] = 0;
        log_head = next_head; // Avanza la testa della coda
        pthread_cond_signal(&log_cond); // Notifica il thread logger della presenza di un nuovo messaggio
    }
    // Se la coda è piena il messaggio viene scartato (nessun overwrite)
    pthread_mutex_unlock(&log_mutex); // Sblocca il mutex
}


// AGGIUNTE PER INVIO MESSAGGI A SERVER TCP

/**
 * @brief Inizializza la connessione TCP al server per l'invio dei log.
 * Se la connessione fallisce, il logging TCP viene disabilitato.
 */
void init_tcp_logger() {
    // Inizializza la connessione TCP al server
    // Questa funzione serve per connettersi a un server TCP
    // e inviare i messaggi di log direttamente al server.
    tcp_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock_fd < 0) {
        perror("❌ Socket creation failed");
        tcp_enabled = 0;
    } else {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(tcp_server_port);
        inet_pton(AF_INET, tcp_server_ip, &addr.sin_addr);

        // Tenta la connessione al server TCP
        if (connect(tcp_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("❌ [TCP]: connection failed, continuing without TCP\n");
            close(tcp_sock_fd);
            tcp_enabled = 0;
        } else {
            printf("✅ TCP logger: connected to %s:%d\n", tcp_server_ip, tcp_server_port);
            tcp_enabled = 1; // Abilita l'invio TCP se la connessione ha successo
        }
    }
}

void send_log_json(const log_msg_t* msg) {
    if (!tcp_enabled || tcp_sock_fd < 0) return;
    char mess[256] = "";
    // int temp_int = 0;

    if(strcmp(msg->event, "RESCUER_INIT") == 0) {
        int x, y;
        char type[32], type2[32];
        //%[^)] per leggere tutto fino a ')'
        if (sscanf(msg->message, "[(%31[^)]) (%d,%d)] Creato gemello digitale per %31[^]]", type, &x, &y, type2) == 4) {
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"x\":%d, \"y\":%d",type, x, y);
        } else {
            return; // fallback
        }
    }    
    else if(strcmp(msg->event, "EMERGENCY_INIT") == 0) {
        int x, y;
        char type[32], status[32];
        //%[^)] per leggere tutto fino a ')'
        if (sscanf(msg->message, "[(%31[^)]) (%d,%d) (%31[^)])] Emergenza correttamente aggiunta alla coda", type, &x, &y, status) == 4) {
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"x\":%d, \"y\":%d, \"status\":\"%s\"",type, x, y, status);
        } else {
            return; // fallback
        }
    }    
    else if(strcmp(msg->event, "RESCUER_STATUS") == 0) {
        int x, y, time, t1, t2, t3, t4, t5;
        char type[32], status[32];
        //%[^)] per leggere tutto fino a ')'

        if (sscanf(msg->message, "[(%31[^)]) (%31[^)]) (%d,%d) (%d)] Partenza verso il luogo dell'emergenza (%d,%d) -> (%d,%d) in %d sec.",
                type, status, &x, &y, &time, &t1, &t2, &t3, &t4, &t5) == 10) {
            // EN_ROUTE_TO_SCENE
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"x\":%d, \"y\":%d, \"time\":%d, \"status\":\"%s\"",
                type, x, y,time, status);
        } else if (sscanf(msg->message, "[(%31[^)]) (%31[^)]) (%d,%d) (%d)] Intervento in corso a (%d,%d) in %d sec.",
                type, status, &x, &y, &time, &t1, &t2, &t3) == 8) {
            // ON_SCENE
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"x\":%d, \"y\":%d, \"time\":%d, \"status\":\"%s\"",
            type, x, y,time, status);
        } else if (sscanf(msg->message, "[(%31[^)]) (%31[^)]) (%d,%d) (%d)] Rientrato alla base (%d,%d) -> (%d,%d) in %d sec.",
                type, status, &x, &y, &time, &t1, &t2, &t3, &t4, &t5) == 10) {
            // RETURNING_TO_BASE
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"x\":%d, \"y\":%d, \"time\":%d, \"status\":\"%s\"",
                type, x, y,time, status);
        } else if (sscanf(msg->message, "[(%31[^)]) (%31[^)])] Intervento completato.",
                type, status) == 2) {
            // IDLE
            snprintf(mess, sizeof(mess),"\"type\":\"%s\", \"status\":\"%s\"",
                type, status);
        } else {
            return; // fallback
        }
    }    
    else if(strcmp(msg->event, "EMERGENCY_STATUS") == 0) {
        char status[32];
        //%[^)] per leggere tutto fino a ')'
        if (sscanf(msg->message, "[%31[^]]] Stato di emergenza aggiornato",status) == 1) {
            snprintf(mess, sizeof(mess),"\"status\":\"%s\"",status);
        } else {
            return; // fallback
        }
    }    
    else{
        return;
    }

    char json_msg[512];
    // Serializza semplice JSON
    char id[32];
    int num = atoi(msg->id);
    snprintf(id, sizeof(id), "0%02d", num % 100);
    snprintf(json_msg, sizeof(json_msg),
             "{\"id\":\"%s\", \"event\":\"%s\", %s}\n",
             id, msg->event, mess);

    // Invia la stringa JSON via TCP
    ssize_t sent = send(tcp_sock_fd, json_msg, strlen(json_msg), 0);
    if (sent < 0) {
        perror("❌ Error sending log JSON");
        tcp_enabled = 0;
        close(tcp_sock_fd);
    }
}