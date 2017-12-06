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

#include "utility.h"
#include "serial.h"

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

/* If DEBUG_SERIAL enabled, enable _debug_serial and enable LOG_SERIAL*/
#ifdef DEBUG_SERIAL
    #define _debug_serial(fmt, ...) DEBUG_PRINT(fmt, ##__VA_ARGS__)
    #ifndef LOG_SERIAL
        #define LOG_SERIAL
    #endif
#endif

/*If LOG_SERIAL is enabled, enable log_serial */
#ifdef LOG_SERIAL
    static int Serial_Com_Log; 
    #define log_serial(fmt, ...) log_fd_line(Serial_Com_Log, fmt, ##__VA_ARGS__)
#else
    #define log_serial(fmt, ...)    
#endif

/* Set debug_serial to call the functions of enabled macros*/
#define debug_serial(fmt, ...) _debug_serial(fmt, ##__VA_ARGS__); \
    log_serial(fmt, ##__VA_ARGS__)

/*Print error statements to screen, will print to log if logging is enabled */
#define error_serial(fmt, ...) ERROR_PRINT(fmt, ##__VA_ARGS__); \
log_serial(fmt, ##__VA_ARGS__)


static int Serial;
const char* const RS232 = "/dev/ttyUSB0";

//lowlevel unexposed api
static inline int serial_try_read(char *buf, size_t bufsize);
static inline bool serial_write(const char *str);
static inline int serial_read_or_timeout(char *buf, size_t bufsize, uint64_t timeout);

bool serial_init()
{
    #ifdef LOG_SERIAL
        Serial_Com_Log = open("com.log", O_WRONLY | O_APPEND);
        if(Serial_Com_Log == -1)
            return false;        
    #endif 
    Serial = open(RS232, O_RDWR | O_NOCTTY | O_NDELAY);

    if (Serial == -1)
    {
        error_serial("Unable to open serial device");
        return false;
    }
    else
    {
        debug_serial("serial opened");
    }

    //setup after opening
    fcntl(Serial, F_SETFL, 0);
    struct termios options;
    tcgetattr(Serial, &options);
    
    
    //Nonblocking read
    fcntl(Serial, F_SETFL, FNDELAY);
    
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
    tcsetattr(Serial, TCSANOW, &options);    



    return true;
}

int serial_try_read(char *buf, size_t bufsize)
{
    int n;
    if((n = read(Serial, buf, bufsize)) > 0)
    {
        buf[n-1] = '\0';
        log_serial("RECV (%d): %s", n, buf);
    }
    return n;
}

int serial_read_or_timeout(char *buf, size_t bufsize, uint64_t timeout)
{
    int n; 
    uint64 start_time = time_in_ms();
    while(((n = serial_try_read(buf, bufsize)) <= 0) && ((time_in_ms() - start_time) < timeout));
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


bool serial_write(const char *str)
{   
    //store in writeable area, append message end character
    char buf[256];
    strcpy(buf, str);
    size_t message_len = strlen(buf)+1; 
    buf[message_len-1] = '\n';

    bool bRet = (write(Serial, buf, message_len) > 0);
    
    //print what we just sent
    buf[message_len-1] = '\0';
    log_serial("SEND (%lu): %s", message_len, buf);
    return bRet;    
}

bool serial_do(const char *cmd, void *result, size_t result_size, int *num_result_read)
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

    //Loop until confirmed success or failure
    bool redo;
    do{
    redo = false;

    //Keep sending until it goes through   
    while(!serial_write(cmd)); 

    //Read for one second max
    if((*num_result_read = serial_read_or_timeout(result, result_size, 1000)) > 0) 
    {
        //See if what we read was an ERROR 
        if(strncmp((const char*)result, "ERROR", strlen("ERROR")) == 0)
        { 
            //We recieved an error, get it and return false           
            serial_do(":SYST:ERR?", result, result_size, num_result_read); 
            return false; 
        }
        //We RECV non error data, success
        return true; 
    }
    else if(result != buf)
        redo = true; //Resend the command, we were expecting a response and didn't RECV anything
    else
        return true; //We weren't expecting a response and did not RECV ERROR, success
    } while(redo);
    return true;
}

void serial_close()
{
    close(Serial);

    #ifdef LOG_SERIAL
        close(Serial_Com_Log);        
    #endif 
}
