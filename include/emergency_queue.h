#ifndef EMERGENCY_QUEUE_H
#define EMERGENCY_QUEUE_H

#include "types.h"

void emergency_queue_init();
void emergency_queue_add(emergency_t emergenza);
emergency_t emergency_queue_get();

#endif // EMERGENCY_QUEUE_H