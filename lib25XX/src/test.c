#include "test.h"
#include <stdbool.h>
#include <stdio.h>

#include "serial.h"

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
    char buf[256];
    serial_write("*IDN?");
    int n;
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) > 0)    
        return true;
    return false;
}
bool test_1()
{
    char buf[256];
    if(!status_check())
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
        waitfun();  
    }

    printf("\nTests complete %u/%lu PASSED\n", passed_cnt, NUM_TESTS);

}