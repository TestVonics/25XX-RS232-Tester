#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <glob.h>
#include <assert.h>
#include <pthread.h>

#include "utility.h"
#include "serial.h"

typedef enum {
    SCPIDeviceType_Master = 1 << 0,
    SCPIDeviceType_Slave  = 1 << 1,
    SCPIDeviceType_Lsu    = 1 << 2
} SCPIDeviceType;  

/* If DEBUG_SERIAL enabled, enable _debug_serial and enable LOG_SERIAL*/
#ifdef DEBUG_SERIAL
    #define _debug_serial(fdm, fmt, ...) _DEBUG_PRINT(fdm, fmt, ##__VA_ARGS__)
    #ifndef LOG_SERIAL
        #define LOG_SERIAL
    #endif
#endif

/*If LOG_SERIAL is enabled, enable log_serial */
static FD_MASK FDM_SER_LOG = FDM_INVALID;
#ifdef LOG_SERIAL   
    #define log_serial(fmt, ...) log_format_line(FDM_SER_LOG, fmt, ##__VA_ARGS__)
#else
    #define log_serial(fmt, ...)    
#endif

/* Set debug_serial to call the functions of enabled macros*/
#define debug_serial(fmt, ...) do {_debug_serial(0, fmt, ##__VA_ARGS__); \
    log_serial(fmt, ##__VA_ARGS__);} while(0)

/*Print error statements to screen, will print to log if logging is enabled */
#ifdef DEBUG
    #define error_serial(fmt, ...) do { ERROR_PRINT(fmt, ##__VA_ARGS__); \
    log_serial(fmt, ##__VA_ARGS__);} while(0)
#else
    #define error_serial(fmt, ...) _ERROR_PRINT(FDM_SER_LOG, fmt, ##__VA_ARGS__) 
#endif

//lowlevel unexposed api
static inline int serial_init_device(const char *path);
static inline int serial_try_read(const int fd, char *buf, const size_t bufsize);
static inline bool serial_write(const int fd, const char *str);
static inline int serial_read_or_timeout(const int fd, char *buf, const size_t bufsize, const uint64_t timeout);
static void *serial_check_device(void *_instance);

int serial_init_device(const char *path)
{
    (void)path;
    static int device = 10;
    device++;
    return device;
}

static SCPIDeviceManager SDM;

SCPIDeviceManager *serial_get_SDM()
{
    return &SDM;
}

typedef struct SDevGlobal{
    SCPIDeviceManager *sdm;
    const char *master_sn;
    const char *slave_sn;
} SDevGlobal;

typedef struct SDevInstance{
    const SDevGlobal *sDev;
    const char *device;
} SDevInstance;

void *serial_check_device(void *_instance)
{
    SDevInstance *instance = (SDevInstance*)_instance;
    SCPIDeviceManager *sdm = instance->sDev->sdm;
    
    bool bRet = true;
    
    debug_serial("glob | Device %s found", instance->device);
    int fd = serial_init_device(instance->device);
    if(fd == -1)
    {                
        error_serial("%s could not be initialized", instance->device);
        return (void*)true;    
    }

    const char *device_name = "SCPI Unknown";
    switch(fd)
    {
        case 11:
        sdm->master.fd = fd;
        debug_serial("SCPI Master set to fd %d", fd); 
        device_name = "SCPI Master";   
        break;
        case 12:
        sdm->slave.fd = fd;
        debug_serial("SCPI Slave set to fd %d", fd);
        device_name = "SCPI Slave";
        break;
        case 13:
        sdm->lsu.fd = fd;
        debug_serial("LSU set to fd %d", fd);
        device_name = "SCPI LSU";
        break;
        default:
        error_serial("Unknown SCPI device connected");
        bRet = false;
        break;
    }
    (void)device_name;
    return (void*)bRet;
}

bool serial_init(SCPIDeviceManager *sdm, const char *master_sn, const char *slave_sn)
{   
    #ifdef LOG_SERIAL
        int serial_com_log = open("com.log", O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if((serial_com_log == -1)||((FDM_SER_LOG = FDM_register_fd(serial_com_log)) == FDM_INVALID))
            return false;        
    #endif 

    sdm->master.fd = -1;
    sdm->slave.fd = -1;
    sdm->lsu.fd = -1;
    bool bRet = true;

    //check each possible device in parallel
    SDevGlobal sdg;
    sdg.sdm = sdm;
    sdg.master_sn = master_sn;
    sdg.slave_sn = slave_sn;
    
    SDevInstance sdi[3];
    pthread_t threads[3];
    for(uint i = 0; i < 3; i++)
    {
        sdi[i].sDev = &sdg;
        sdi[i].device = "";
        pthread_create(&threads[i], NULL, &serial_check_device, &sdi[i]);
    }  
    
    for(uint i = 0; i < 3; i++)
    {
        bool tempRet;
        pthread_join(threads[i], (void**)&tempRet);
        if(!tempRet)
            bRet = false;
    }    

    SDM = *sdm;
    
    return bRet;
}

//BAD HACK 
static uint64 last_time;
void update_last_time()
{
    last_time = time_in_ms();
}

int serial_try_read(const int fd, char *buf, const size_t bufsize)
{
    (void)fd;
    (void)buf;
    (void)bufsize;
    #ifdef DELAY_BEFORE_SERIAL_READ
        struct timespec ts;
        SLEEP_MS(&ts, DELAY_BEFORE_SERIAL_READ);
    #endif
    return -1;
}

int serial_read_or_timeout(const int fd, char *buf, const size_t bufsize, const uint64_t timeout)
{
    int n; 
    uint64 start_time = time_in_ms();
    while(((n = serial_try_read(fd, buf, bufsize)) <= 0) && ((time_in_ms() - start_time) < timeout));
    return n;
}

bool serial_integer_cmd(const int fd, const char *cmd, int *result)
{    
    char buf[256];
    if(serial_fd_do(fd, cmd, buf, sizeof(buf), NULL))
    {
         *result = atoi(buf);
          return true;
    }
    return false;   
}

#define TIME_TO_WAIT 100
void wait_for_time_to_write()
{
    struct timespec ts;
    uint64 time_elapsed = time_in_ms() - last_time;
    if(time_elapsed < TIME_TO_WAIT)
    {
        SLEEP_MS(&ts, TIME_TO_WAIT - time_elapsed);
    }
}

bool serial_write(const int fd, const char *str)
{   
    (void)fd;
    //store in writeable area, append message end character
    char buf[256];
    strcpy(buf, str);
    size_t message_len = strlen(buf)+1; 
    buf[message_len-1] = '\n';

    wait_for_time_to_write();
    
    bool bRet = false;
    
    //print what we just sent
    buf[message_len-1] = '\0';
    log_serial("SEND|t=%llu|(%lu): %s", time_in_ms(), message_len, buf);
    return bRet;    
}

bool serial_fd_do(const int fd, const char *cmd, void *result, size_t result_size, int *num_result_read)
{      
    //Setup some variables to store data if not provided by caller
    char buf[256];
    if(result == NULL)
    {
        result = buf;        
        result_size = sizeof(buf);
    }
    int n;
    if(num_result_read == NULL)
        num_result_read = &n;

    //DEBUG_PRINT("%p %p buf, &buf", buf, &buf);
    //Loop until confirmed success or failure
    for(int i = 0; i < 3; i++) {
    //DEBUG_PRINT("%p result", result);

    //Fail if a write fails and still fails after a sleep  
    if(!serial_write(fd, cmd))
    {
        sleep(4);
        if(!serial_write(fd, cmd)) return false;
    }

    //Read for one second max
    if((*num_result_read = serial_read_or_timeout(fd, result, result_size, 1000)) > 0) 
    {
        //See if what we read was an ERROR 
        if(strncmp((const char*)result, "ERROR", strlen("ERROR")) == 0)
        { 
            //We recieved an error, get it and return false           
            serial_fd_do(fd, ":SYST:ERR?", result, result_size, num_result_read); 
            return false; 
        }
        //We RECV non error data, success
        return true; 
    }
    else if(result == buf)
        return true; //We weren't expecting a response and did not RECV ERROR, success 
    
    }
 
    return false; //We didnt recieve a response after a certain amount of attempts
}

void serial_close(SCPIDeviceManager *sdm)
{
    (void)sdm;
    #ifdef LOG_SERIAL
        FDM_close(FDM_SER_LOG);        
    #endif 
}
