#ifndef PARSER_RESCUERS_H
#define PARSER_RESCUERS_H

#include "types.h"

int load_rescuer_types(
    const char* filename,
    rescuer_type_info_t** out_types,
    int* out_count
);

#endif // PARSER_RESCUERS_H