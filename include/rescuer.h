#ifndef RESCUER_H
#define RESCUER_H

#include "types.h"

/**
 * @brief Avvia i thread dei gemelli digitali dei soccorritori.
 * 
 * @param rescuers Array di soccorritori digitali
 * @param count Numero totale di soccorritori
 */
void start_rescuer(rescuer_thread_t* rescuer_wrapped);
#endif // RESCUER_H