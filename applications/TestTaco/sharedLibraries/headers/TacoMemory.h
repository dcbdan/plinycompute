#pragma once

#include <Object.h>
#include <Nothing.h>
#include "Arr.h"

extern "C" {
    void* tacoMalloc(size_t size);
    void* tacoRealloc(void* ptr, size_t new_size);
} // extern "C"

