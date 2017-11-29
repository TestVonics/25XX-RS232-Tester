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

#include "serial.h"
#include "test.h"

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

//nonblocking character reading
void set_terminal_mode();
void reset_terminal_mode();

//my prototypes
void wait_for_user();

int main(int argc, char **argv)
{
    printf("25XX Tester\n");
    set_terminal_mode();
    
    if(!serial_init())
    {
        exit(1);
    }    
    test_run_all(&wait_for_user);

    serial_close();
    
    return 0;
}

void wait_for_user()
{
    printf("Press any key to continue\n");
    while(getchar() == EOF){}
}


struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);

    new_termios.c_lflag &= ~ICANON;
    new_termios.c_lflag &= ~ECHO;
    new_termios.c_lflag &= ~ISIG;
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &new_termios);
}
