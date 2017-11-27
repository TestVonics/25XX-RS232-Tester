#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "serial.h"
#include "status.h"
#include "test.h"

typedef bool (*test_func)(void);
typedef unsigned int uint;

typedef struct TEST {
    const char *test_name;
    const char *setup;
    const char *user_task;
    test_func test;    
} TEST;

bool test_0();
bool test_1();
bool test_2();
bool test_3();
bool test_4();
bool test_5();
bool test_6();
bool test_7();
bool test_8();
bool test_9();
bool test_10();
bool test_11();
bool test_12();
bool test_13();
bool test_14();
bool test_15();

static TEST Tests[] = {
    {
        "Control Pressure - Aeronautical Units No Volume",
        "Remove all volumes/loads",
        NULL,
        &test_1   
    },
};

#define NUM_TESTS (sizeof(Tests)/sizeof(Tests[0]))

bool test_0()
{
    int n;
    char buf[256];

    //clear any errors to start with 
    serial_write("*CLS");

    //dump this information to screen for the user
    serial_write("*IDN?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    //start out vented
    serial_write(":SYST:MODE VENT");

    /*
    serial_write(":SYST:MODE CTRL");

    serial_write(":CONT:PS:MAXSETP?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PS:MAXRATE?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PS:SETP?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PS:RATE?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;


    serial_write(":CONT:PT:MAXSETP?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PT:MAXRATE?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PT:SETP?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":CONT:PT:RATE?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    serial_write(":SYST:ERR?");    
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;    */

    return true;
}
bool test_1()
{
    char buf[256];
    if(!status_is_idle())
        return false;

    //put it in control
    serial_write(":SYST:MODE CTRL");
    if(!command_try_expect(":SYST:MODE?", "CTRL"))
        return false;

    //dual test
    serial_write(":CONT:MODE DUAL"); 
    if(!command_try_expect(":CONT:MODE?", "DUAL"))
        return false;

    //set PS
    serial_write(":CONT:PS:UNITS FT");
    if(!command_try_expect(":CONT:PS:UNITS?", "FT"))
        return false;
    serial_write(":CONT:PS:SETP -2000");
    if(!command_try_expect(":CONT:PS:SETP?", "-2000"))
        return false;
    serial_write(":CONT:PS:RATE 50000");
    if(!command_try_expect(":CONT:PS:RATE?", "50000"))
        return false;


    //set PT
    serial_write(":CONT:PT:UNITS KTS");
    if(!command_try_expect(":CONT:PT:UNITS?", "KTS"))
        return false;
    serial_write(":CONT:PT:SETP 1000");
    if(!command_try_expect(":CONT:PT:SETP?", "1000"))
        return false;
    serial_write(":CONT:PT:RATE 800");
    if(!command_try_expect_float(":CONT:PT:RATE?", 800))
        return false;

    //EXECUTE
    serial_write(":CONT:EXEC");
    sleep(1);
    StatOperEven soe;
    //While we are ramping, check every 5 seconds
    while(((soe = command_StatOperEven()).opr & (OPR_PS_RAMPING | OPR_PT_RAMPING)) != 0)
    { 
        printf("DUAL CHANNEL RAMPING\n");        
        sleep(5);
    }

    //When the loop exits we better be stable    
    if((soe.opr & OPR_STABLE) != 0)
        return false;    
    
    return true;
}


bool test_2()
{
    return true;
}

void test_run_all(wait_func waitfun)
{
    //if *IDN? doesn't work, exit
    if(!test_0())
        return;
    
    uint passed_cnt = 0;
    for(uint i = 0; i < NUM_TESTS; i++)
    {
        printf("\nTest #%u: %s\n", i+1, Tests[i].test_name);
        printf("SETUP: %s\n", Tests[i].setup);
        if(Tests[i].user_task == NULL)
        {
            Tests[i].user_task = "Please wait for test to complete";
        }
      
        printf("TASK: %s\n", Tests[i].user_task);
        waitfun();        
        if(Tests[i].test())
        {
            printf("Test #%u PASSED\n", i+1);
            passed_cnt++;          
        }
        else
        {
            printf("Test #%u FAILED\n", i+1);
        }

        //disconnect remote
        //serial_write(":SYST:REMOTE DISABLE");
        waitfun();  

        //go to ground
        serial_write(":CONT:GTGR");

        
    }

    printf("\nTests complete %u/%lu PASSED\n", passed_cnt, NUM_TESTS);

}