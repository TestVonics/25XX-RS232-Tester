#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

typedef unsigned int uint;
typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

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

typedef enum FD_MASK {
    FDM_INVALID = 0,
    FDM_STDOUT  = 1 << 0,
    FDM_STDERR  = 1 << 1,
    FDM_FREE0   = 1 << 2,
    FDM_FREE1   = 1 << 3,
    FDM_FREE2   = 1 << 4,
    FDM_FREE3   = 1 << 5,
    FDM_FREE4   = 1 << 6,
    FDM_FREE5   = 1 << 7    
} FD_MASK;
FD_MASK FDM_TEST_LOG;

bool lib_init();
bool log_init();
void lib_close();
typedef struct SCPIDeviceManager SCPIDeviceManager;

typedef const char *(*get_buf_func)(void);
typedef bool (*yes_or_no_func)();
bool lib_init(SCPIDeviceManager  *sdm, get_buf_func master_sn, get_buf_func slave_sn, get_buf_func ask_name, yes_or_no_func yes_no);

FD_MASK FDM_register_fd(const int fd);
void FDM_close(FD_MASK mask);
ssize_t log_format_debug(const FD_MASK fdm, const char * const function_src, const char* const _format, ...);
ssize_t log_format_line(const FD_MASK fdm, const char *const _format, ...);
bool parse_sn(char *result, const char *src);
yes_or_no_func Yes_No;

#define OUTPUT_PRINT(fmt, ...) log_format_line(FDM_STDOUT | FDM_TEST_LOG, fmt, ##__VA_ARGS__)

#ifdef DEBUG /* Print debug messages to screen and print errors in debug form */
    #define _DEBUG_PRINT(fdm, fmt, ...) log_format_debug(fdm | FDM_STDERR, __func__, fmt, ##__VA_ARGS__)    
    #define DEBUG_PRINT(fmt, ...) _DEBUG_PRINT(0, fmt, ##__VA_ARGS__) 
    #define _ERROR_PRINT(fdm, fmt, ...)  _DEBUG_PRINT(fdm | FDM_TEST_LOG, fmt, ##__VA_ARGS__)   
    #define ERROR_PRINT(fmt, ...) _ERROR_PRINT(0, fmt, ##__VA_ARGS__) 
#else       /* print error messages to screen in user form */
    #define _DEBUG_PRINT(fdm, fmt, ...)
    #define DEBUG_PRINT(fmt, ...)
    #define _ERROR_PRINT(fdm, fmt, ...) log_format_line(fdm | FDM_TEST_LOG | FDM_STDERR, fmt, ##__VA_ARGS__)
    #define ERROR_PRINT(fmt, ...) _ERROR_PRINT(0, fmt, ##__VA_ARGS__)
#endif

/* Configurable Options */
#define LOG_SERIAL

#ifdef DEBUG /* Configure debug */ 
    #define DEBUG_SERIAL
#else        /*Configure release */
#endif 

