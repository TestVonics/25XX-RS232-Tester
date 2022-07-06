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
#include <ctype.h>

#include "test25XX.h"

#ifdef __CYGWIN__
#define NO_RESET_ATEXIT
#define ENABLE_BASIC_INPUT
#endif

#ifdef ENABLE_BASIC_INPUT
#define SLEEP_MS_IF_DEFINED(X) do {\
struct timespec ts; \
SLEEP_MS(&ts, X); \
}while(0)
#else
#define SLEEP_MS_IF_DEFINED(X)
#endif

typedef uint8_t byte;
typedef uint64_t uint64;
typedef uint32_t uint32;

//nonblocking character reading
void set_terminal_mode();
void reset_terminal_mode();

//my prototypes
TEST_CHOICE my_tc();
char *add_input(char *buf, size_t buflen);
bool yes_no();

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
    make_inputbuf_func(get_master_id, master, 24, "Enter the SN of the master (bottom) unit:"); \
    make_inputbuf_func(get_slave_id, slave, 24, "Enter the SN of the slave unit:");
    make_inputbuf_func(get_tester_name, name, 64, "Enter your name:");
#endif



int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if(!test25XX_init(get_master_id, get_slave_id, get_tester_name, yes_no, my_tc))
    {
        return 1;
    }    

    #ifndef BLOCKING_KEYPRESS
    set_terminal_mode();
    #endif
    
    test25XX_run_tests();

    test25XX_close();

    #ifdef NO_RESET_ATEXIT
    reset_terminal_mode();
    #endif 
    
    return 0;
}

bool yes_no()
{    
    #ifndef ENABLE_BASIC_INPUT
    //dump buffered keypresses
    while(getchar() != EOF){SLEEP_MS_IF_DEFINED(300);}
    #endif

    printf("(Y)es or (N)o? ");
    fflush(stdout);
    int c;
    do {
        while((c = getchar()) == EOF){SLEEP_MS_IF_DEFINED(300);}
        c = tolower(c);
        if(c == 'y')
            return true;
        else if( c == 'n')
            return false;
        else
            printf("Invalid key pressed %c\n", c);
    }while(1);
}

TEST_CHOICE my_tc()
{
    //dump buffered keypressed
    #ifndef ENABLE_BASIC_INPUT
    while(getchar() != EOF){SLEEP_MS_IF_DEFINED(300);}
    #endif

    printf("Press R to Run, P for Previous Test, S for Skip Test\n");
    TEST_CHOICE tc = TC_UNKNOWN;
    do {
        int c;
        while((c = getchar()) == EOF){SLEEP_MS_IF_DEFINED(300);}
        c = tolower(c);
        if(c == 'r')
        {
            tc = TC_RUN;
        }
        else if(c == 'p') {
            tc = TC_PREV;
        }
        else if(c == 's') {
            tc = TC_SKIP;
        }
    } while(tc == TC_UNKNOWN);
    return tc;
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
    #ifndef NO_RESET_ATEXIT
    atexit(reset_terminal_mode);
    #endif
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

