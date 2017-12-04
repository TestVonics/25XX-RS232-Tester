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

void log_message(const char* const function_src, const char* const _format, ...);
int log_fd(int fd, const char* const _format, ...);

#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...)    log_message(__func__, fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#define ERROR_PRINT(fmt, ...) DEBUG_PRINT(fmt, ##__VA_ARGS__); \
fprintf(stderr, fmt, ##__VA_ARGS__)


