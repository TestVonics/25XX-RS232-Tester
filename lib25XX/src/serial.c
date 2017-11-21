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

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

static int Serial;
const char* const RS232 = "/home/gavin/dev/ttyS21";

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
    return true;
}

int serial_try_read(char *buf, size_t bufsize)
{
    int n;
    if((n = read(Serial, buf, bufsize)) > 0)
    {
        printf("\nRECV: (%d): %s\n", n, buf);
    }
    return n;
}


void serial_write(const char *str)
{   
    char buf[256];
    strcpy(buf, str);
    size_t message_len = strlen(buf)+2;     

    buf[message_len - 1] = '\n';
    printf("\nSEND (%lu): %s\n", message_len, buf);
    write(Serial, buf, message_len);
}

void serial_close()
{
    close(Serial);
}
