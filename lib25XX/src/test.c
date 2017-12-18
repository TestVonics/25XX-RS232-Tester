#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

#include "serial.h"
#include "status.h"
#include "test.h"
#include "control.h"
#include "utility.h"
#include "command.h"
#include "lsu.h"

typedef _TEST TEST;
typedef bool (*test_func)(const TEST *test);
typedef enum TEST_T{
    TEST_T_CTRL,
    TEST_T_MEAS,
    TEST_T_LSUV,
    TEST_T_LEAK
} TEST_T;

#define _TEST_SET struct { \
    const TEST_T type; \
    test_func test_func; \
    const char *name; \
    const bool master_gtg_before_each_test; \
}
typedef _TEST_SET TEST_SET;
static inline int testset_get_num_tests(const TEST_SET *test_set);
static inline TEST *testset_get_test(const TEST_SET *test_set, const uint index);

typedef struct ControlTestSet{
    _TEST_SET;
    //It is awful we have to hardcode this here, but gcc doesn't let us take the _sizeof_ a flexible array in a struct
    //even after creating one statically, so the choices were make the TEST array a pointer to a tests array
    //or put the whole array in a macro so we can take the _sizeof_ the macro for the calculation of array elements
    //or just specify the number of elements as a constant here ... 
    ControlTest tests[8]; 
} ControlTestSet;

#define strRemoveVolumes "Remove all volumes/loads"
#define strConnect60     "Connect 60 cubic inch volume"
#define strConnect100    "Connect 100 cubic inch volume"
static ControlTestSet ControlTests = {
{
    TEST_T_CTRL,
    (test_func)(control_run_test),
    "ADTS Control",
    true
}, 
{
    { 
      {  "Control Pressure - Aeronautical Units No Volume",
         strRemoveVolumes,
         NULL,         
      }, CTRL_UNITS_FK, 160000, "-2000", "50000", "1000", "800"
    },
    {
      {  "Control Pressure - Aeronautical Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,         
      }, CTRL_UNITS_FK, 240000, "-2000", "25000", "1000", "400"
    },
    {
      {  "Control Pressure - Aeronautical Units 100 Cubic Inch Volume",
         strConnect100,
         NULL,         
      }, CTRL_UNITS_FK, 360000, "-2000", "10000", "1000", "200"
    },
    {
      {  "Control Vacuum - Aeronautical Units No Volume",
         strRemoveVolumes,
         NULL,         
      }, CTRL_UNITS_FK, 150000, "92000", "50000", "0", "800" 
    },
    {
      {  "Control Vacuum - Aeronautical Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,        
      }, CTRL_UNITS_FK, 300000, "92000", "25000", "0", "400"
    },
    {
      {  "Control Vacuum - Aeronautical Units 100 Cubic Inch Volume",
         strConnect100,
         NULL,         
      }, CTRL_UNITS_FK, 600000, "92000", "10000", "0", "200"
    },
    {
      {  "Control Vacuum - INHG Pressure Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,        
      }, CTRL_UNITS_INHG, 360000, "0.465", "40.000", "0.465", "40.000"
    },
    {
      {  "Control Pressure - INHG Pressure Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,         
      }, CTRL_UNITS_INHG, 360000, "32.148", "40.000", "73.545", "50.000"
    },
}};
#define NUM_CONTROL_TESTS 0 //LENGTH_2D(ControlTests.tests)

typedef struct SingleChannelTestSet {
    _TEST_SET;
    SingleChannelTest tests[1];
} SingleChannelTestSet;

static SingleChannelTestSet SingleChannelTests = {
{
    TEST_T_MEAS,
    (test_func)(control_single_channel_test),
    "ADTS Control and Measure",
    true
}, .tests =
{
    { 
      {  "Control Rate of Climb - Aeronautical Units",
         "Connect the PS and the PT units from unit to another.",
         NULL,         
      }, CTRL_UNITS_FK, 160000, CTRL_OP_PS, {.ps = "80000"}, {.ps_rate = "50000"}
    },
}};
#define NUM_MEAS_TESTS 0//LENGTH_2D(SingleChannelTests.tests)

typedef struct LSUValveTestSet {
    _TEST_SET;
    LSUValveTest tests[9];
} LSUValveTestSet;

#define strLSUOpenAndClose "LSU Valve Open and Close"
#define strLSUSetup        "No additional setup required"
#define strLSUTask         "Answer the prompts"
static LSUValveTestSet LSUValveTests = {
{
    TEST_T_LSUV,
    (test_func)(lsu_valve_test),
    strLSUOpenAndClose,
    false
}, .tests = 
{
    {
      {  strLSUOpenAndClose " All Valves",
         strLSUSetup,
         strLSUTask,
      }, "ALL"
    },
    {
      {  strLSUOpenAndClose " PS #1",
         strLSUSetup,
         strLSUTask,
      }, "1"
    },
    {
      {  strLSUOpenAndClose " PS #2",
         strLSUSetup,
         strLSUTask,
      }, "2"
    },
    {
      {  strLSUOpenAndClose " PS #3",
         strLSUSetup,
         strLSUTask,
      }, "3"
    },
    {
      {  strLSUOpenAndClose " PS #4",
         strLSUSetup,
         strLSUTask,
      }, "4"
    },
    {
      {  strLSUOpenAndClose " PT #1",
         strLSUSetup,
         strLSUTask,
      }, "5"
    },
    {
      {  strLSUOpenAndClose " PT #2",
         strLSUSetup,
         strLSUTask,
      }, "6"
    },
    {
      {  strLSUOpenAndClose " PT #3",
         strLSUSetup,
         strLSUTask,
      }, "7"
    },
    {
      {  strLSUOpenAndClose " PT #4",
         strLSUSetup,
         strLSUTask,
      }, "8"
    },
}};
#define NUM_LSUV_TESTS 0//LENGTH_2D(LSUValveTests.tests)


typedef struct LeakTestSet {
    _TEST_SET;
    LeakTest tests[4];
} LeakTestSet;

LeakTestSet LeakTests = {
{
    TEST_T_LEAK,
    (test_func)control_run_leak_test,
    "ADTS Leak Test",
    true    
}, {
    {{ 
      {  "Low Pressure Leak Test with CACD",
         "Connect CACD with LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "3.425", "40"
    }, 0.010, 0.010, "2", "0"
    },
    {{ 
      {  "High Pressure Leak Test with CACD",
         "Connect CACD with LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "73.500", "40"
    }, 0.010, 0.020, "2", "0"
    },
    {{ 
      {  "Low Pressure Leak Test with PSA",
         "Connect PSA with LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "3.425", "40"
    }, 0.010, 0.012, "2", "0"
    },
    {{ 
      {  "High Pressure Leak Test with PSA",
         "Connect PSA with LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "73.500", "40"
    }, 0.010, 0.025, "2", "0"
    },
}};
#define NUM_LEAK_TESTS LENGTH_2D(LeakTests.tests)

static TEST_SET *TestSets[] = 
{
    (TEST_SET*)&ControlTests,
    (TEST_SET*)&SingleChannelTests,
    (TEST_SET*)&LSUValveTests,
    (TEST_SET*)&LeakTests
};
#define NUM_TEST_SETS LENGTH_2D(TestSets)

#define NUM_TESTS (NUM_CONTROL_TESTS + NUM_MEAS_TESTS + NUM_LSUV_TESTS + NUM_LEAK_TESTS)

static inline int testset_get_num_tests(const TEST_SET *test_set)
{
    if(test_set->type == TEST_T_CTRL)
        return NUM_CONTROL_TESTS;
    else if(test_set->type == TEST_T_MEAS)
        return NUM_MEAS_TESTS;
    else if(test_set->type == TEST_T_LSUV)
        return NUM_LSUV_TESTS;
    else if(test_set->type == TEST_T_LEAK)
        return NUM_LEAK_TESTS;
    else return -1;
}

static inline TEST *testset_get_test(const TEST_SET *test_set, const uint index)
{    
    if(test_set->type == TEST_T_CTRL)
        return (TEST*)&(((ControlTestSet*)test_set)->tests[index]);
    else if(test_set->type == TEST_T_MEAS)
        return (TEST*)&(((SingleChannelTestSet*)test_set)->tests[index]);
    else if(test_set->type == TEST_T_LSUV)
        return (TEST*)&(((LSUValveTestSet*)test_set)->tests[index]);
    else if(test_set->type == TEST_T_LEAK)
        return (TEST*)&(((LeakTestSet*)test_set)->tests[index]);
    else return NULL;
}

//Run all the tests, pass in a callback of your waiting function
void test_run_all(wait_func waitfunc)
{   
    //Run the test sets
    uint passed_cnt = 0;      
    uint current_test = 0;
    for(uint i = 0; i < NUM_TEST_SETS; i++)
    {
        //Setup the test set
        OUTPUT_PRINT("\nEntering test set: %s (%u/%u)", TestSets[i]->name, i+1,NUM_TEST_SETS); 
        uint test_set_passed_cnt = 0;
        int num_tests = testset_get_num_tests(TestSets[i]);

        //Run the test set tests
        for(uint j = 0; (int)j < num_tests; j++)
        {    
            //if the tests involve the master, we will GTG before each test
            if(TestSets[i]->master_gtg_before_each_test)
            {                
                OUTPUT_PRINT("Test setup - Controlling to ground");
                command_GTG_eventually();
            }

            //get rid of leftovers
            if(!serial_do("*CLS", NULL, 0, NULL))
                return;    

            TEST *test;
            assert((test = testset_get_test(TestSets[i], j)) != NULL); 
            OUTPUT_PRINT("Test set %s - Test #%u: %s", TestSets[i]->name, j+1, test->test_name);
            OUTPUT_PRINT("SETUP: %s", test->setup);
            if(test->user_task == NULL)
            {
                test->user_task = "Please wait for test to complete";
            }
      
            OUTPUT_PRINT("TASK: %s",test->user_task);
            waitfunc();        
            
            //Finally run the test function
            if(TestSets[i]->test_func(test))
            {
                OUTPUT_PRINT("Test set %s - Test #%u PASSED\n", TestSets[i]->name, j+1);
                test_set_passed_cnt++;                         
            }
            else
            {
                ERROR_PRINT("Test set %s - Test #%u FAILED\n", TestSets[i]->name, j+1);                
            }
            ++current_test;
        }
        OUTPUT_PRINT("\nTest set: %s complete, (%u/%u) tests PASSED", TestSets[i]->name, test_set_passed_cnt, (uint)num_tests); 
        passed_cnt += test_set_passed_cnt;
    }

    OUTPUT_PRINT("All test set tests: complete, (%u/%u) total tests PASSED", passed_cnt, NUM_TESTS); 
    
    //control to ground, remote mode is no longer needed    
    serial_do(":CONT:GTGR", NULL, 0, NULL);
    OUTPUT_PRINT("Sent Control to ground command, Exiting");
    serial_do(":SYST:REMOTE DISABLE", NULL, 0, NULL);   
}

