#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "parser_env.h"
#include "types.h"
#include "macros.h"
#include <threads.h>

#define MAX_NAME_LEN 64
#define MAX_EMERGENCIES 128

typedef struct {
    char* name;
    int x;
    int y;
    int delay_sec;
    mqd_t mq;
} emergency_to_send;

/**
 * @brief Stampa le modalità d'uso del programma client.
 * @param prog Nome dell'eseguibile.
 */
void print_usage(const char* prog) {
    fprintf(stderr, "USAGE (singola emergenza): %s <nome_emergenza> <x> <y> <delay>\n", prog);
    fprintf(stderr, "USAGE (da file):           %s -f <file_path>\n", prog);
}

/**
 * @brief Funzione eseguita da un thread che invia una singola emergenza sulla message queue.
 * @param arg Puntatore a struct emergency_to_send.
 */
// void send_emergency(const char* name, int x, int y, int delay_sec, mqd_t mq) {
int send_emergency(void* arg) {
    emergency_to_send* em = (emergency_to_send*)arg;
    const char* name = strdup(em->name);
    int x = em->x;
    int y = em->y;
    int delay_sec = em->delay_sec;
    mqd_t mq = em->mq;
    free(em->name);
    free(em); // Libera la memoria allocata per l'emergenza

    emergency_request_t req;
    memset(&req, 0, sizeof(req));

    strncpy(req.emergency_name, name, MAX_NAME_LEN - 1);
    req.x = x;
    req.y = y;
    sleep(delay_sec); // Simula il delay
    req.timestamp = time(NULL); // Imposta il timestamp corrente

    if (mq_send(mq, (char*)&req, sizeof(req), 0) == -1) {
        perror("❌ mq_send");
    } else {
        printf("✅ Emergenza inviata: %s (%d,%d) %d(sec)\n", name, x, y,delay_sec);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    env_config_t env_config;
    load_env_config("./conf/env.conf", &env_config);
    char queue_name[sizeof(env_config.queue)+2]; // +2 per '/' e terminatore
    queue_name[0] = '/';
    strncpy(queue_name + 1, env_config.queue, strlen(env_config.queue));
    queue_name[strlen(env_config.queue) + 1] = '\0';

    mqd_t mq;

    // Apre la coda dei messaggi
    printf("🔓 Apertura coda: %s\n", queue_name);
    mq = mq_open(queue_name, O_WRONLY);
    CHECK_MQ_OPEN(mq, queue_name);

    // Modalità da file: invia emergenze lette da file riga per riga
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        printf("📂 Apertura file: %s\n", argv[2]);
        FILE* file = fopen(argv[2], "r");
        if (!file) {
            perror("❌ fopen");
            mq_close(mq);
            return 1;
        }
        printf("📂 File aperto correttamente.\n");

        char line[256];
        int i = 0;
        thrd_t threads[MAX_EMERGENCIES];
        while (fgets(line, sizeof(line), file) != NULL && i < MAX_EMERGENCIES) {
            // Rimuove il carattere di nuova linea
            line[strcspn(line, "\n")] = 0;
            char name[MAX_NAME_LEN];
            int x, y, delay;
            int name_end = 0;
            // Trova la fine del nome (prima cifra)
            while(!isdigit(line[name_end])) {
                name_end++;
            }
            strncpy(name, line, name_end);
            name[name_end-1] = '\0';
            if(sscanf(line + name_end, "%d %d %d", &x, &y, &delay) == 3) {
                emergency_to_send* em = malloc(sizeof(emergency_to_send));
                CHECK_MALLOC(em, fail);
                em->name = strdup(name);
                em->x = x;
                em->y = y;
                em->delay_sec = delay;
                em->mq = mq;
                thrd_create(&threads[i++], send_emergency, (void*)em);
            } else {
                fprintf(stderr, "❌ Riga ignorata (formato errato): %s\n", line);
                return 1;
            }
        }

        fclose(file);

        // Aspetta che tutti i thread finiscano
        for (int j = 0; j < i; j++) {
            thrd_join(threads[j], NULL);
        }
        
    }

    // Modalità singola: invia una sola emergenza
    else if (argc == 5) {
        char* name = argv[1];
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int delay = atoi(argv[4]);

        emergency_to_send* em = malloc(sizeof(emergency_to_send));
        CHECK_MALLOC(em, fail);
        em->name = strdup(name);
        em->x = x;
        em->y = y;
        em->delay_sec = delay;
        em->mq = mq;
        thrd_t thread;
        thrd_create(&thread, send_emergency, (void*)em);
        thrd_join(thread, NULL); // Aspetta che il thread finisca

        // send_emergency(name, x, y, delay, mq);
    }

    else {
        print_usage(argv[0]);  // Stampa l'uso corretto
        fail:
        mq_close(mq);
        return 1;
    }

    mq_close(mq);
    return 0;
}
