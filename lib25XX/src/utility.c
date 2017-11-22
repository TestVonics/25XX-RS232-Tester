#include <time.h>
#include <math.h>

#include "utility.h"

uint64_t time_in_ms()
{
    uint64_t ms; // Milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    ms = (uint64_t)((uint64_t)round(spec.tv_nsec / 1000000) + (uint64_t)(spec.tv_sec * 1000)); // Convert nanoseconds to milliseconds   

    return ms;
}