#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "utility.h"

int vbuild_debug(char *dest, size_t dest_len ,const char * func_src, const char *_format, va_list arg);
int vlog_fd(const int fd, const char * const format, va_list arg);


uint64_t time_in_ms()
{
    uint64_t ms; // Milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    ms = (uint64_t)((uint64_t)round(spec.tv_nsec / 1000000) + (uint64_t)(spec.tv_sec * 1000)); // Convert nanoseconds to milliseconds   

    return ms;
}

int vbuild_debug(char *dest, size_t dest_len ,const char * function_src, const char *_format, va_list arg)
{
    char format[768];
    if(snprintf(format, sizeof(format), "%-22.22s| %s\n", function_src, _format) < 0)
        return -1;

    return vsnprintf(dest, dest_len, format, arg);
}

int vlog_fd(const int fd, const char * const format, va_list arg)
{
    char message[1024];
    int msglen = vsnprintf(message, 1024, format, arg);     
    return write(fd, message, msglen);
}


int log_fd_debug(const int fd, const char * const function_src, const char* const _format, ...)
{
    char message[1024];
    va_list arg;    
    va_start (arg, _format); 
    int len = vbuild_debug(message, sizeof(message), function_src, _format, arg);      
    va_end (arg);
    return write(fd, message, len);
}

int log_fd_line(int fd, const char *const _format, ...)
{
    //Add \n to format string
    char format[768]; 
    uint i;
    for(i = 0; i < sizeof(format) - 1; i++)
    {
        if(_format[i] == '\0')
        {
           format[i] = '\n';
           format[i+1] = '\0'; 
           break;
        }
        format[i] = _format[i];
    }
    if(i >= (sizeof(format) - 1))
    {
        return -1;
    }

    va_list arg;    
    va_start (arg, _format);
    int ret = vlog_fd(fd, format, arg);
    va_end (arg);
    return ret;
}

