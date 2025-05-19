#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "logger.h"

#define CHECK_MALLOC(ptr, label) \
    do { \
        if (!(ptr)) { \
            char log_msg[256]; \
            snprintf(log_msg, sizeof(log_msg), "malloc failed %s",strerror(errno)); \
            log_event("1300", "MEMORY", log_msg); \
            goto label; \
        } \
    } while(0)

#define CHECK_FOPEN(id,file, filename) \
    do { \
        if (!(file)) { \
            char log_msg[256]; \
            snprintf(log_msg, sizeof(log_msg), "Errore nell'apertura del file %s: %s", filename, strerror(errno)); \
            log_event(id, "FILE_OPENING", log_msg); \
        } \
    } while(0)

#define CHECK_MQ_OPEN(mq, qname) \
    do { \
        if ((mq) == (mqd_t)-1) { \
            char log_msg[256]; \
            snprintf(log_msg, sizeof(log_msg), "Errore aprendo coda %s: %s", qname, strerror(errno)); \
            log_event("1100", "MESSAGE_QUEUE", log_msg); \
            pthread_exit(NULL); \
        } \
    } while(0)

#endif
