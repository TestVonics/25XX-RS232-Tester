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

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

static int Serial;
const char* const RS232 = "/dev/ttyUSB0";

bool serial_init()
{
    Serial = open(RS232, O_RDWR | O_NOCTTY | O_NDELAY);

    if (Serial == -1)
    {
        printf("serial_init Unable to open serial device\n");
        return false;
    }
    else
    {
        printf("serial_init Serial Opened\n");
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

static inline int serial_try_read(char *buf, size_t bufsize)
{
    int n;
    if((n = read(Serial, buf, bufsize)) > 0)
    {
        buf[n-1] = '\0';
        printf("RECV: (%d): %s\n", n, buf);
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
    serial_write(cmd);
    char buf[256];
    int n = serial_read_or_timeout(buf, sizeof(buf), 5000);
    if( n <= 0)
        return false; 

    *result = atoi(buf);
    return true;
}


void serial_write(const char *str)
{   
    char buf[256];
    strcpy(buf, str);
    size_t message_len = strlen(buf)+1;     

    buf[message_len-1] = '\n';
    write(Serial, buf, message_len);
    buf[message_len-1] = '\0';
    printf("\nSEND (%lu): %s\n", message_len, buf);
    
}

void serial_close()
{
    close(Serial);
}
