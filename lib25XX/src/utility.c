#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "utility.h"

uint64_t time_in_ms()
{
    uint64_t ms; // Milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    ms = (uint64_t)((uint64_t)round(spec.tv_nsec / 1000000) + (uint64_t)(spec.tv_sec * 1000)); // Convert nanoseconds to milliseconds   

    return ms;
}

void log_message(const char* const function_src, const char* const _format, ...)
{
    //Add the start to the full format string   
    char format[512];
    snprintf(format, 512, "%-22.22s| %s\n", function_src, _format); //first 22 characters of function name   
   
    //Now expand the format
    va_list arg;    
    va_start (arg, _format);
    char message[1024];
    vsnprintf(message, 1024, format, arg);     
    va_end (arg);     
   
    //Now print the message
    fprintf(stderr, message);        
}

/*
int log_func_fd(int fd, const char *const function_src, const char* const _format, ...)
{
    char format[512];
    snprintf(format, 512, "%-22.22s| %s", function_src, _format); //first 22 characters of function name   
    return log_fd(fd, format, 
}
*/

int log_fd(int fd, const char* const _format, ...)
{
    //Add the start to the full format string   
    char format[768];
    snprintf(format, sizeof(format), "%s\n", _format);
   
    //Now expand the format
    va_list arg;    
    va_start (arg, _format);
    char message[1024];
    int msglen = vsnprintf(message, 1024, format, arg);     
    va_end (arg);     
   
    //Now print the message
    return write(fd, message, msglen);           
}