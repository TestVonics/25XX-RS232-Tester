#include <stdint.h>
#include <time.h>
#include <unistd.h>

typedef unsigned int uint;
uint64_t time_in_ms();

#define FORCEINLINE static __attribute__((always_inline)) inline
FORCEINLINE void SLEEP_MS(struct timespec *ts, unsigned long ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms - (ts->tv_sec * 1000)) * 1000000;
    //ts->tv_nsec = (ms % 1000) * 1000000;
    
    nanosleep(ts, NULL);
}
#define LENGTH_2D(x) (sizeof(x)/sizeof(x[0])) 

int log_fd_debug(const int fd, const char * const function_src, const char* const _format, ...);
int log_fd_line(int fd, const char* const _format, ...);

#define OUTPUT_PRINT(fmt, ...) log_fd_line(STDOUT_FILENO, fmt, ##__VA_ARGS__)

#ifdef DEBUG /* Print debug messages to screen and print errors in debug form */
    #define DEBUG_PRINT(fmt, ...) log_fd_debug(STDERR_FILENO, __func__, fmt, ##__VA_ARGS__)    
    #define ERROR_PRINT(fmt, ...) DEBUG_PRINT(fmt, ##__VA_ARGS__)
#else       /* print error messages to screen in user form */
    #define DEBUG_PRINT(fmt, ...)
    #define ERROR_PRINT(fmt, ...) log_fd_line(STDERR_FILENO, fmt, ##__VA_ARGS__)
#endif

/* Configurable Options */
#define LOG_SERIAL

#ifdef DEBUG /* Configure debug */ 
    #define DEBUG_SERIAL
#else        /*Configure release */
#endif 