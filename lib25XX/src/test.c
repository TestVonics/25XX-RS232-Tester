#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>

#include "serial.h"
#include "status.h"
#include "test.h"
#include "control.h"
#include "utility.h"
#include "command.h"

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

#define strRemoveVolumes "Remove all volumes/loads"
#define strConnect60     "Connect 60 cubic inch volume"
#define strConnect100    "Connect 100 cubic inch volume"
static TEST Tests[] = {
    /*{
        "Control Pressure - Aeronautical Units No Volume",
        strRemoveVolumes,
        NULL,
        &test_1   
    },
    {
        "Control Pressure - Aeronautical Units 60 Cubic Inch Volume",
        strConnect60,
        NULL,
        &test_2
    },
    {
        "Control Pressure - Aeronautical Units 100 Cubic Inch Volume",
        strConnect100,
        NULL,
        &test_3
    },*/
    {
        "Control Vacuum - Aeronautical Units No Volume",
        strRemoveVolumes,
        NULL,
        &test_4
    },
    {
        "Control Vacuum - Aeronautical Units 60 Cubic Inch Volume",
        strConnect60,
        NULL,
        &test_5
    },
    {
        "Control Vacuum - Aeronautical Units 100 Cubic Inch Volume",
        strConnect100,
        NULL,
        &test_6
    },
    {
        "Control Vacuum - INHG Pressure Units 60 Cubic Inch Volume",
        strConnect60,
        NULL,
        &test_7
    },
    {
        "Control Pressure - INHG Pressure Units 60 Cubic Inch Volume",
        strConnect60,
        NULL,
        &test_8
    },
};

#define NUM_TESTS (sizeof(Tests)/sizeof(Tests[0]))

bool test_0()
{
    //clear any errors to start with 
    if(!serial_do("*CLS", NULL, 0, NULL))
        return false;

    //dump this information to screen for the user
    char buf[256];
    if(!serial_do("*IDN?", buf, sizeof(buf), NULL))
         return false;      

    return true;
}
bool test_1()
{   
    return control_dual_FK("-2000", "50000", "1000", "800", 140000);
}


bool test_2()
{
    return control_dual_FK("-2000", "25000", "1000", "400", 240000);
}

bool test_3()
{
    return control_dual_FK("-2000", "10000", "1000", "200", 360000);
}

bool test_4()
{  
    return control_dual_FK("92000", "50000", "0", "800", 150000); 
}

bool test_5()
{
    return control_dual_FK("92000", "25000", "0", "400", 300000);  
}

bool test_6()
{
    return control_dual_FK("92000", "10000", "0", "200", 600000);  
}

bool test_7()
{
    return control_dual_INHG("0.465", "40.000", "0.465", "40.000", 360000);
}


bool test_8()
{
    return control_dual_INHG("32.148", "40.000", "73.545", "50.000", 360000);
}


void test_run_all(wait_func waitfun)
{    
    if(!test_0())
        return;
    
    struct timespec ts;
    uint passed_cnt = 0;
    for(uint i = 0; i < NUM_TESTS; i++)
    {   
        //start each test vented and idle
        command_GTG_eventually();        

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
    }
    serial_do(":CONT:GTGR", NULL, 0, NULL);
    printf("\nTests complete %u/%lu PASSED\n", passed_cnt, NUM_TESTS);

}