#ifndef STORAGE_H
#define STORAGE_H

#include <string.h>

#include "types.h"

static inline void result_set_reset(ResultSet *result) {
    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
}

#endif
