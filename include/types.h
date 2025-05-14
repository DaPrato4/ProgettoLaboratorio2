#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <pthread.h>
#define EMERGENCY_NAME_LENGTH 64
#define MAX_QUEUE_NAME 16


//TIPI E ISTANZE DI SOCCORRITORI

/**
 * @brief Enumerazione che rappresenta i possibili stati di un soccorritore
 * durante una missione di soccorso
 */
typedef enum {
    IDLE,                  // In attesa di assegnazione
    EN_ROUTE_TO_SCENE,     // In viaggio verso il luogo dell'emergenza
    ON_SCENE,              // Sul luogo dell'emergenza
    RETURNING_TO_BASE      // In ritorno alla base
} rescuer_status_t;

/**
 * @brief Struttura che definisce un tipo di soccorritore
 * Questi dati vengono caricati dal file rescuers.conf
 */
typedef struct {
    char* rescuer_type_name;   // Nome identificativo del tipo di soccorritore
    int speed;                 // Velocità di movimento (unità/tempo)
    int x;                     // Coordinata X della base operativa
    int y;                     // Coordinata Y della base operativa
} rescuer_type_t;

/**
 * @brief Struttura che rappresenta un'istanza attiva di un soccorritore
 * Contiene lo stato corrente e la posizione del soccorritore durante una missione
 */
typedef struct {
    int id;                    // Identificativo univoco del soccorritore
    int x;                     // Posizione X corrente
    int y;                     // Posizione Y corrente
    rescuer_type_t* rescuer;   // Puntatore al tipo di soccorritore
    rescuer_status_t status;   // Stato corrente del soccorritore
} rescuer_digital_twin_t;



//TIPI DI EMERGENZA

/**
 * @brief Struttura che definisce i requisiti di soccorritori per un'emergenza
 * Specifica il tipo e il numero di soccorritori necessari
 */
typedef struct {
    rescuer_type_t* type;    // Tipo di soccorritore richiesto
    int required_count;             // Numero di soccorritori necessari
    int time_to_manage;             // Tempo necessario per gestire l'emergenza
} rescuer_request_t;

/**
 * @brief Struttura che definisce un tipo di emergenza
 * Questi dati vengono caricati dal file emergency_types.conf
 */
typedef struct {
    short priority;                 // Livello di priorità dell'emergenza
    char* emergency_desc;           // Descrizione dell'emergenza
    rescuer_request_t* rescuers;    // Array di richieste di soccorritori
    int rescuers_req_number;        // Numero totale di soccorritori richiesti
} emergency_type_t;



//ISTANZE DI EMERGENZA

/**
 * @brief Stati che un'emergenza può assumere nel suo ciclo di vita
 * Utilizzati per tracciare lo stato corrente di un'emergenza all'interno del sistema.
 */
typedef enum {
    WAITING,          ///< Emergenza in attesa di essere gestita
    ASSIGNED,         ///< Sono stati assegnati i soccorritori
    IN_PROGRESS,      ///< La gestione è in corso
    PAUSED,           ///< La gestione è stata temporaneamente sospesa
    COMPLETED,        ///< L’emergenza è stata risolta
    CANCELED,         ///< L’emergenza è stata annullata
    TIMEOUT           ///< L’emergenza non è stata gestita nei tempi previsti
} emergency_status_t;

/**
 * @brief Richiesta di emergenza inviata dal client tramite message queue
 * Questa struttura rappresenta una richiesta grezza contenente le coordinate e il tipo dell'emergenza.
 */
typedef struct {
    char emergency_name[EMERGENCY_NAME_LENGTH]; ///< Nome dell’emergenza (es. "Incendio")
    int x;                                      ///< Coordinata X dell’emergenza
    int y;                                      ///< Coordinata Y dell’emergenza
    time_t timestamp;                           ///< Timestamp di ricezione della richiesta
} emergency_request_t;

/**
 * @brief Rappresentazione completa di un'emergenza in gestione
 * Include informazioni dettagliate derivate dal tipo, lo stato corrente,
 * le coordinate, il tempo, e i soccorritori assegnati.
 */
typedef struct {
    int id;                                    ///< Identificativo univoco dell’emergenza (AGGIUNTO PER COMODITÀ)
    emergency_type_t type;                     ///< Tipo di emergenza (caricato da file)
    emergency_status_t status;                 ///< Stato attuale dell’emergenza
    int x;                                     ///< Coordinata X dell’emergenza
    int y;                                     ///< Coordinata Y dell’emergenza
    time_t time;                               ///< Tempo di inizio della gestione
    int rescuer_count;                         ///< Numero di soccorritori assegnati
    rescuer_digital_twin_t** rescuers_dt;      ///< Puntatore all’elenco dei soccorritori assegnati
    pthread_mutex_t mutex;                     ///< Mutex per la sincronizzazione dell'accesso         
} emergency_t;

//AGGIUNTI
typedef struct {
    rescuer_type_t rescuer_type;    // Tipo di soccorritore
    int count;                      // Numero di soccorritori
} rescuer_type_info_t;

typedef struct {
    char queue[MAX_QUEUE_NAME];
    int height;
    int width;
} env_config_t;


/**
 * @brief Struttura che rappresenta un soccorritore in un thread
 * Contiene il gemello digitale del soccorritore e le informazioni sul thread
 */
typedef struct {
    rescuer_digital_twin_t* twin;     // Gemello digitale originale
    pthread_mutex_t mutex;            // Mutex personale
    pthread_cond_t cond;              // Condition var personale
    pthread_t thread;                 // Thread associato
    emergency_t* current_em;          // Emergenza corrente
} rescuer_thread_t;

#endif // TYPES_H