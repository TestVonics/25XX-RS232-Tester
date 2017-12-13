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
#include "test.h"
#include "serial.h"

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

//nonblocking character reading
void set_terminal_mode();
void reset_terminal_mode();

//my prototypes
void wait_for_user();
int supply_predetermined_data(const IN_DATA_ID data_id, char *buf, const size_t bufsize);
char *add_input(char *buf, size_t buflen);

/*
#define CMACHINE(OP, PARAM1, PARAM2)
#if (OP == "BUF")
char buf[PARAM1]
#elif (OP == "FUNC") 
PARAM1
#endif
*/
#ifdef SUPPLY_DBG_INPUT
    const char *get_master_id(){ return "231";}
    const char *get_slave_id(){  return "245";}
    const char *get_tester_name(){ return "Gavin Hayes";}
#else
    #define make_inputbuf_func(FUNC, NAME, SIZE, TEXT) \
    const char *FUNC() \
    { \
        printf(TEXT "\n"); \
        static char NAME[SIZE]; \
        return add_input(NAME, sizeof(NAME)); \
    } 
    make_inputbuf_func(get_master_id, master, 24, "Enter the SN of the master unit:"); \
    make_inputbuf_func(get_slave_id, slave, 24, "Enter the SN of the slave unit:");
    make_inputbuf_func(get_tester_name, name, 64, "Enter your name:");
#endif



int main(int argc, char **argv)
{    
    SCPIDeviceManager sdm;
    
    if(!lib_init(&sdm, get_master_id, get_slave_id, get_tester_name))
    {
        return 1;
    }    

    set_terminal_mode();
    test_run_all(&wait_for_user);

    lib_close(&sdm);
    
    return 0;
}

void wait_for_user()
{
    printf("Press any key to continue\n");
    while(getchar() == EOF){}
}

int supply_predetermined_data(const IN_DATA_ID data_id, char *buf, const size_t bufsize)
{        
    if(data_id & IN_DATA_CTRL_ADTS_SN)
        return snprintf(buf, bufsize, "%s", "231");
    else if(data_id & IN_DATA_MEAS_ADTS_SN)
        return snprintf(buf, bufsize, "%s", "245");

    return -1;
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

char *add_input(char *buf, size_t buflen)
{   
   /*
   int c;
   uint n = 0;
   while(((c = getchar()) != '\n') &&(n < (buflen -2))) 
   {
       if(c != EOF)
       {
           printf("%c", c);
           buf[n++] = c;          
       }
   }
   printf("\n");    
   buf[n] = '\0'; */

   if(fgets(buf, buflen, stdin) == NULL)
       return NULL;
   
   buf[strcspn(buf, "\n")] = '\0';
   return  buf;
}

