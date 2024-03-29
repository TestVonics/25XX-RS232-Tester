#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "utility.h"
#include "serial.h"

typedef struct {
    FD_MASK mask;
    int     fd;
} FD_MAP;

bool log_init(const char *filepath);
void log_close();
int vbuild_debug(char *dest, size_t dest_len ,const char * func_src, const char *_format, va_list arg);
static int add_newline(char *dest, const char *src, size_t dest_size);
static int vformat_and_newline(char *dest, size_t dest_size, const char *const _format, va_list arg);
ssize_t FDM_write(FD_MASK mask, const void *buf, size_t count);
static inline bool build_filename_from_sn(char *filename, const char *sn);

#define FDM_MAX 8
#define FD_INVALID -1
static int FD_MAP_Free_Pos = 2;
FD_MAP Fd_Map[FDM_MAX] = {{FDM_STDOUT, STDOUT_FILENO}, {FDM_STDERR, STDERR_FILENO}, {FDM_FREE0, FD_INVALID}, {FDM_FREE1, FD_INVALID}, {FDM_FREE2, FD_INVALID}, {FDM_FREE3, FD_INVALID}, {FDM_FREE4, FD_INVALID}, {FDM_FREE5, FD_INVALID}};

FD_MASK FDM_register_fd(const int fd)
{
    if(FD_MAP_Free_Pos == -1)
        return FDM_INVALID;

    Fd_Map[FD_MAP_Free_Pos].fd = fd;
    FD_MASK ret = Fd_Map[FD_MAP_Free_Pos].mask;

    //if the next position is out of bounds we hit our max    
    if(++FD_MAP_Free_Pos == FDM_MAX)
        FD_MAP_Free_Pos = -1;

    return ret;
}

void FDM_close(FD_MASK mask)
{
    for(uint i = 0; i < FDM_MAX; i++)
    {
        if(Fd_Map[i].fd == FD_INVALID)
            return;
        
        if(mask & Fd_Map[i].mask)
        {
            close(Fd_Map[i].fd);
        }
    }
}

//Write to specified FD
ssize_t FDM_write(FD_MASK mask, const void *buf, size_t count)
{
    ssize_t last_write = -1;    
    for(uint i = 0; i < FDM_MAX; i++)
    {
        if(Fd_Map[i].fd == FD_INVALID)
            return last_write;
        
        if(mask & Fd_Map[i].mask)
        {
            last_write = write(Fd_Map[i].fd, buf, count);
        }
    }
    return last_write;
}

int vbuild_debug(char *dest, size_t dest_len ,const char * function_src, const char *_format, va_list arg)
{
    char format[768];
    if(snprintf(format, sizeof(format), "%-22.22s| %s\n", function_src, _format) < 0)
        return -1;

    return vsnprintf(dest, dest_len, format, arg);
}

static inline int add_newline(char *dest, const char *src, size_t dest_size)
{
    //Add \n to string
    uint i;
    for(i = 0; i < dest_size - 1; i++)
    {
        if(src[i] == '\0')
        {
           dest[i] = '\n';
           dest[i+1] = '\0'; 
           break;
        }
        dest[i] = src[i];
    }
    if(i >= (dest_size - 1))
    {
        return -1;
    }

    return 0;
}

static inline int vformat_and_newline(char *dest, size_t dest_size, const char *const _format, va_list arg)
{
    char format[768]; 
    if(add_newline(format, _format, LENGTH_2D(format)) != 0)
        return -1;

    return vsnprintf(dest, dest_size, format, arg);   
}

ssize_t log_format_debug(const FD_MASK fdm, const char * const function_src, const char* const _format, ...)
{
    char message[1024];
    va_list arg;    
    va_start (arg, _format); 
    int len = vbuild_debug(message, sizeof(message), function_src, _format, arg);      
    va_end (arg);
    return FDM_write(fdm, message, len);
}

ssize_t log_format_line(const FD_MASK fdm, const char *const _format, ...)
{
    char dest[1024]; 
    va_list arg;    
    va_start (arg, _format);
    int len = vformat_and_newline(dest, LENGTH_2D(dest), _format, arg); 
    va_end (arg);
    return FDM_write(fdm, dest, len);     
}

bool log_init(const char *filepath)
{
    int test_log_fd;
    //chmod 644
    if((test_log_fd = open(filepath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
    {
        perror("");
        return false;
    }
    FDM_TEST_LOG = FDM_register_fd(test_log_fd);
 
    return true;
}

void log_close()
{
    FDM_close(FDM_TEST_LOG);
}

bool parse_sn(char *result, const char *src)
{
    for(uint i = 0; i < (strlen(src) - 2); i++)
    {
        if((src[i] == 'S') && (src[i+1] == '/'))
        {
            if(sscanf(&src[i], "S/N %s", result) == 1)
            {
                result[strlen(result)-1] = '\0';
                return true;
            }
        }
        
    }
    return false;
}

static inline bool build_filename_from_sn(char *filename, const char *sn)
{
    time_t now = time(NULL);
    struct tm *timenow;
    timenow = localtime(&now);
    
    char filetemp[256];
    strftime(filetemp, sizeof(filetemp), "_%Y-%m-%d_%H:%M:%S.log", timenow);
    snprintf(filename, 256, "SN_%s%s", sn, filetemp);
    DEBUG_PRINT("log filename: %s", filename);
    return true;
}

uint64_t time_in_ms()
{
    uint64_t ms; // Milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    ms = (uint64_t)((uint64_t)round(spec.tv_nsec / 1000000) + (uint64_t)(spec.tv_sec * 1000)); // Convert nanoseconds to milliseconds   

    return ms;
}

bool lib_init(SCPIDeviceManager *sdm, get_buf_func master_sn, get_buf_func slave_sn, get_buf_func ask_name, yes_or_no_func yes_no)
{
    
    const char *name = ask_name();
    const char *master = master_sn();
    const char *slave = slave_sn();
    //make the main log file named after the master sn
    char filename[256];
    if(!build_filename_from_sn(filename, master))
        return false;
    if(!log_init(filename))
        return false; 

    OUTPUT_PRINT("ADC|ADTS|LSU RS232 Tester V0.1");
    OUTPUT_PRINT("Tester name: %s", name);
    OUTPUT_PRINT("Master unit S/N: %s", master);     
    OUTPUT_PRINT("Slave unit S/N: %s", slave);

    //Initialize serial   
    if(!serial_init(sdm, master, slave))
        return false;

    Yes_No = yes_no;
    return true;
}

void lib_close(SCPIDeviceManager *sdm)
{
    serial_close(sdm);    
    log_close();
}