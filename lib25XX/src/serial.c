#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <glob.h>
#include <assert.h>

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

int serial_init_device(const char *path)
{
    int device = open(path, O_RDWR | O_NOCTTY | O_NDELAY);

    if (device == -1)
    {
        error_serial("Unable to open serial device");
        return device;
    }
    else
    {
        debug_serial("serial opened");
    }

    //setup after opening
    fcntl(device, F_SETFL, 0);
    struct termios options;
    tcgetattr(device, &options);
    
    
    //Nonblocking read
    fcntl(device, F_SETFL, FNDELAY);
    
    options.c_cflag |= (CLOCAL | CREAD);
    
    //9600
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);    
    
    //8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    //Canonical input seperate by LF (0x0A)
    options.c_lflag |= (ICANON);     
    
    //turn on software handshaking  
    options.c_iflag |= (IXON | IXOFF | IXANY);  
    
    //set it finally
    tcsetattr(device, TCSANOW, &options);    

    return device;
}

static SCPIDeviceManager SDM;

SCPIDeviceManager *serial_get_SDM()
{
    return &SDM;
}

bool serial_init(SCPIDeviceManager *sdm, const char *master_sn, const char *slave_sn)
{   
    #ifdef LOG_SERIAL
        int serial_com_log = open("com.log", O_WRONLY | O_APPEND);
        if((serial_com_log == -1)||((FDM_SER_LOG = FDM_register_fd(serial_com_log)) == FDM_INVALID))
            return false;        
    #endif 

    sdm->master.fd = -1;
    sdm->slave.fd = -1;
    sdm->lsu.fd = -1;
    bool bRet = true;
    glob_t glob_results;

    if(glob("/dev/ttyUSB*", 0, NULL, &glob_results) == 0)
    {
        int fd;
        for(uint i = 0; i < glob_results.gl_pathc; i++)
        {
            debug_serial("glob | Device %s found", glob_results.gl_pathv[i]);
            fd = serial_init_device(glob_results.gl_pathv[i]);
            if(fd == -1)
            {                
                error_serial("%s could not be initialized", glob_results.gl_pathv[i]);
                //bRet = false;
                continue;
            }
            
            char buf[256];
            if(!serial_fd_do(fd, "*IDN?", buf, sizeof(buf), 0))
            {
                debug_serial("*IDN? failed for device: %s", glob_results.gl_pathv[i]);
            }
            else
            {
                char sn[32];
                if(parse_sn(sn, buf))
                {
                    if(strncmp(sn, master_sn, strlen(master_sn)) == 0) 
                    {
                        sdm->master.fd = fd;
                        debug_serial("SCPI Master set to fd %d", fd);                        
                    }
                    else if(strncmp(sn, slave_sn, strlen(slave_sn)) == 0)
                    {
                        sdm->slave.fd = fd;
                        debug_serial("SCPI Slave set to fd %d", fd);
                    }
                    else
                    {
                        error_serial("SN %s not expected", sn);
                        bRet = false;
                    }
                }
                else if(strstr(buf, "LSU") != NULL)
                {
                    sdm->lsu.fd = fd;
                    debug_serial("LSU set to fd %d", fd);
                }
                else
                {
                    error_serial("Unknown SCPI device connected");
                    bRet = false;
                }
                OUTPUT_PRINT("%s", buf);
                serial_fd_do(fd, "*CLS", NULL, 0, NULL);        
            }
        }        
    }
    else
    {
        bRet = false;
    }
    globfree(&glob_results); 
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
    int n;
    if((n = read(fd, buf, bufsize)) > 0)
    {
        update_last_time();
        buf[n-1] = '\0';
        log_serial("RECV|t=%llu|(%d): %s", time_in_ms(), n, buf);
        
        char stuff[256];
        ssize_t z;     
        if((z = read(fd, stuff, sizeof(stuff))) > 0)
            debug_serial("WHY is there DATA: (%ld) %s", z, stuff);
    }
    return n;
}

int serial_read_or_timeout(const int fd, char *buf, const size_t bufsize, const uint64_t timeout)
{
    int n; 
    uint64 start_time = time_in_ms();
    while(((n = serial_try_read(fd, buf, bufsize)) <= 0) && ((time_in_ms() - start_time) < timeout));
    return n;
}

bool serial_integer_cmd(const char *cmd, int *result)
{    
    char buf[256];
    if(serial_do(cmd, buf, sizeof(buf), NULL))
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
    //store in writeable area, append message end character
    char buf[256];
    strcpy(buf, str);
    size_t message_len = strlen(buf)+1; 
    buf[message_len-1] = '\n';

    wait_for_time_to_write();
    
    bool bRet = (write(fd, buf, message_len) > 0);
    
    //print what we just sent
    buf[message_len-1] = '\0';
    log_serial("SEND|t=%llu|(%lu): %s", time_in_ms(), message_len, buf);
    return bRet;    
}

bool serial_do(const char *cmd, void *result, size_t result_size, int *num_result_read)
{
    return serial_fd_do(SDM.master.fd, cmd, result, result_size, num_result_read);
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
    if(((!serial_write(fd, cmd)) && (sleep(4) >= 0))&&  (!serial_write(fd, cmd)))
        return false;

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
    close(sdm->master.fd);
    close(sdm->slave.fd);
    close(sdm->lsu.fd);

    #ifdef LOG_SERIAL
        FDM_close(FDM_SER_LOG);        
    #endif 
}
