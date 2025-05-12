#ifndef RESCUER_H
#define RESCUER_H

#include "types.h"

/**
 * @brief Avvia i thread dei gemelli digitali dei soccorritori.
 * 
 * @param rescuers Array di soccorritori digitali
 * @param count Numero totale di soccorritori
 */
void start_rescuers(rescuer_thread_t* rescuers, rescuer_digital_twin_t* twins, int rescuers_count);

#endif // RESCUER_H