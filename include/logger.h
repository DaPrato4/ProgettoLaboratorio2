#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

void start_logger_thread();
void stop_logger_thread();
void log_event(const char* id, const char* event, const char* message);

#endif