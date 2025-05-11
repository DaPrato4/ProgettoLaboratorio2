#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "parser_env.h"
#include "types.h"

#define MAX_NAME_LEN 64

void print_usage(const char* prog) {
    fprintf(stderr, "USAGE (singola emergenza): %s <nome_emergenza> <x> <y> <delay>\n", prog);
    fprintf(stderr, "USAGE (da file):           %s -f <file_path>\n", prog);
}

// 🔁 Funzione per inviare una singola emergenza
void send_emergency(const char* name, int x, int y, int delay_sec, mqd_t mq) {
    emergency_request_t req;
    memset(&req, 0, sizeof(req));

    strncpy(req.emergency_name, name, MAX_NAME_LEN - 1);
    req.x = x;
    req.y = y;
    req.timestamp = delay_sec;

    if (mq_send(mq, (char*)&req, sizeof(req), 0) == -1) {
        perror("mq_send");
    } else {
        printf("✅ Emergenza inviata: %s (%d,%d) %d(sec)\n", name, x, y,delay_sec);
    }
}

int main(int argc, char* argv[]) {
    env_config_t env_config;
    load_env_config("./conf/env.conf", &env_config);
    char queue_name[sizeof(env_config.queue)+2]; // +2 for '/' and null terminator
    queue_name[0] = '/';
    strncpy(queue_name + 1, env_config.queue, strlen(env_config.queue));
    queue_name[strlen(env_config.queue) + 1] = '\0';

    mqd_t mq;

    // 🔓 Apre la coda
    printf("🔓 Apertura coda: %s\n", queue_name);
    mq = mq_open(queue_name, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    // Modalità da file DA INSERIRE
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        printf("📂 Apertura file: %s\n", argv[2]);
        FILE* file = fopen(argv[2], "r");
        if (!file) {
            perror("fopen");
            mq_close(mq);
            return 1;
        }
        printf("📂 File aperto correttamente.\n");

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
            // Rimuove il carattere di nuova linea
            line[strcspn(line, "\n")] = 0;
            char name[MAX_NAME_LEN];
            int x, y, delay;
            int name_end = 0;
            while(!isdigit(line[name_end])) {
                name_end++;
            }
            strncpy(name, line, name_end);
            name[name_end-1] = '\0';
            if(sscanf(line + name_end, "%d %d %d", &x, &y, &delay) == 3) {
                send_emergency(name, x, y, delay, mq);
            } else {
                fprintf(stderr, "❌ Riga ignorata (formato errato): %s\n", line);
                return 1;
            }
        }

        fclose(file);
    }

    // Modalità singola
    else if (argc == 5) {
        char* name = argv[1];
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int delay = atoi(argv[4]);

        send_emergency(name, x, y, delay, mq);
    }

    else {
        print_usage(argv[0]);  // Stampa l'uso corretto
        mq_close(mq);
        return 1;
    }

    mq_close(mq);
    return 0;
}
