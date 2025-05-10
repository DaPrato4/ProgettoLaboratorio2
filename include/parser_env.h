#ifndef PARSER_ENV_H
#define PARSER_ENV_H

#include "types.h"

int load_env_config(
    const char* filename, 
    env_config_t* config
);

#endif // PARSER_ENV_H