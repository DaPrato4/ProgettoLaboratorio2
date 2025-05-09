#ifndef PARSER_EMERGENCY_H
#define PARSER_EMERGENCY_H

#include "types.h"

// int parse_emergency_type_line(
//     const char* line,
//     emergency_type_t* out_type,
//     rescuer_type_t* known_types,
//     int known_types_count
// );

int load_emergency_types(
    const char* filename,
    emergency_type_t** out_types,
    int* out_count,
    rescuer_type_t* known_types,
    int known_types_count
);

#endif // PARSER_EMERGENCY_H