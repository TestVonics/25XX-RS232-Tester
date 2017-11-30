#include <stdint.h>
#include <time.h>
uint64_t time_in_ms();

#define FORCEINLINE static __attribute__((always_inline)) inline
FORCEINLINE void SLEEP_MS(struct timespec *ts, unsigned long ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms - (ts->tv_sec * 1000)) * 1000000;
    //ts->tv_nsec = (ms % 1000) * 1000000;
    
    nanosleep(ts, NULL);
}