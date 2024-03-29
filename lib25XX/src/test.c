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
    const bool init_master_before_each_test; \
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

#define strRemoveVolumes "Remove all volumes/loads, cap all ports and close all valves on LSU"
#define strConnect60     "Connect 60 cubic inch volume to both Ps and Pt"
#define strConnect100    "Connect 100 cubic inch volume to both Ps and Pt."
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
      }, CTRL_UNITS_FK, 120000, "-2000", "50000", "1000", "800"
    },
    {
      {  "Control Pressure - Aeronautical Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,         
      }, CTRL_UNITS_FK, 200000, "-2000", "25000", "1000", "400"
    },
    {
      {  "Control Pressure - Aeronautical Units 100 Cubic Inch Volume",
         strConnect100,
         NULL,         
      }, CTRL_UNITS_FK, 390000, "-2000", "10000", "1000", "200"
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
      }, CTRL_UNITS_FK, 635000, "92000", "10000", "0", "200"
    },
    {
      {  "Control Vacuum - INHG Pressure Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,        
      }, CTRL_UNITS_INHG, 250000, "0.815", "15.000", "0.815", "15.000"
    },
    {
      {  "Control Pressure - INHG Pressure Units 60 Cubic Inch Volume",
         strConnect60,
         NULL,         
      }, CTRL_UNITS_INHG, 160000, "32.148", "30.000", "73.545", "50.000"
    },
}};
#define NUM_CONTROL_TESTS LENGTH_2D(ControlTests.tests)

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
         "Connect the PS and the PT units from unit to another on the LSU",
         NULL,         
      }, CTRL_UNITS_FK, 160000, CTRL_OP_PS, {.ps = "80000"}, {.ps_rate = "50000"}
    },
}};
#define NUM_MEAS_TESTS LENGTH_2D(SingleChannelTests.tests)

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
#define NUM_LSUV_TESTS LENGTH_2D(LSUValveTests.tests)


typedef struct LeakTestSet {
    _TEST_SET;
    LeakTest tests[4];
} LeakTestSet;

LeakTestSet LeakTests = {
{
    TEST_T_LEAK,
    (test_func)control_run_leak_test,
    "ADTS Leak Test",
    false    
}, {
    {{ 
      {  "Low Pressure Leak Test with CACD",
         "Connect CACD or volume to LSU straight-through",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "3.425", "40"
    }, 0.010, 0.010, "2", "0", false
    },
    {{ 
      {  "High Pressure Leak Test with CACD",
         "Connect CACD or volume with LSU straight-through",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "73.500", "40"
    }, 0.010, 0.020, "2", "0", false
    },
    {{ 
      {  "Low Pressure Leak Test with PSA",
         "Connect PSA or volume to LSU, turn on the valves in use on the LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "3.425", "40"
    }, 0.010, 0.012, "2", "0", true
    },
    {{ 
      {  "High Pressure Leak Test with PSA",
         "Connect PSA or volume to LSU, turn on the valves in use on the LSU",
         NULL,         
      }, CTRL_UNITS_INHG, 200000, "3.425", "40", "73.500", "40"
    }, 0.010, 0.025, "2", "0", true
    },
}};
#define NUM_LEAK_TESTS LENGTH_2D(LeakTests.tests)

static TEST_SET *TestSets[] = 
{
    (TEST_SET*)&ControlTests,
    (TEST_SET*)&LeakTests,
    (TEST_SET*)&SingleChannelTests,
    (TEST_SET*)&LSUValveTests,    
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
void test_run_all(UserFunc *user_func)
{   
    const tc_choice tc = user_func->tc;
    //Run the test sets
    uint passed_cnt = 0;      
    uint current_test = 0;
    bool at_ground = false;
    for(uint i = 0; i < NUM_TEST_SETS; i++)
    {
        //Setup the test set
        OUTPUT_PRINT("\nEntering test set: %s (%u/%u)", TestSets[i]->name, i+1,NUM_TEST_SETS); 
        if(!user_func->yes_no())
        {
            OUTPUT_PRINT("No - OK, skipping test set: %s (%u/%u)", TestSets[i]->name, i+1,NUM_TEST_SETS); 
            continue;
        }
        else
            OUTPUT_PRINT("Yes");
        uint test_set_passed_cnt = 0;
        int num_tests = testset_get_num_tests(TestSets[i]);

        //Run the test set tests
        for(int jsigned = 0; jsigned < num_tests; jsigned++)
        {
            uint j = (uint)jsigned; //jsigned will never be signed here
            //if the tests involve the master, we will GTG before each test
            if(TestSets[i]->init_master_before_each_test)
            {        
                if(!at_ground)
                {        
                    OUTPUT_PRINT("Test setup - Controlling to ground");
                    command_GTG_eventually(serial_get_SDM()->master.fd);
                }
                at_ground = false;

                //get rid of leftovers
                if(!serial_fd_do(serial_get_SDM()->master.fd, "*CLS", NULL, 0, NULL))
                    return;                
            }
            
            TEST *test;
            assert((test = testset_get_test(TestSets[i], j)) != NULL); 
            OUTPUT_PRINT("Test set %s - Test #%u: %s", TestSets[i]->name, j+1, test->test_name);            

            OUTPUT_PRINT("SETUP: %s", test->setup);
            if(test->user_task == NULL)
            {
                test->user_task = "Please wait for test to complete";
            }
      
            OUTPUT_PRINT("TASK: %s",test->user_task);
            TEST_CHOICE tcvar = tc();
            //OUTPUT_PRINT("tc is %u", tcvar);

            if(tcvar & TC_RUN)
            {
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
            }
            else if(tcvar & TC_SKIP)
            {
                OUTPUT_PRINT("Skipping test %u", j+1);                
            }
            
            if(tcvar & TC_PREV)
            {                
                if(current_test > 0)
                {
                    //can mess with test_set_passed_cnt, if a passed test is redone or testsets are switched
                    OUTPUT_PRINT("Going to previous test\n");
                    current_test--;
                    jsigned -= 2;
                    //if the previous test is outside of this test set
                    if(jsigned < -1)
                    {
                        //set to the last test of the last testset
                        i--;
                        test_set_passed_cnt = 0;
                        num_tests = testset_get_num_tests(TestSets[i]);
                        jsigned = num_tests - 1;                        
                    } 
                }
                else
                {
                    OUTPUT_PRINT("Can't go back, on first test!\n");
                }                
            }
            else
            {
                ++current_test;
            }            
        }
        

        OUTPUT_PRINT("\nTest set: %s complete, (%u/%u) tests PASSED", TestSets[i]->name, test_set_passed_cnt, (uint)num_tests); 
        passed_cnt += test_set_passed_cnt;

                
        //go to ground if the test set involved leaving ground
        if(TestSets[i]->init_master_before_each_test)
        {
            OUTPUT_PRINT("Test set complete - Controlling to ground");
            command_GTG_eventually(serial_get_SDM()->master.fd);
            at_ground = true;
        }
    }

    OUTPUT_PRINT("All test set tests: complete, (%u/%u) total tests PASSED", passed_cnt, NUM_TESTS); 
    
    //control to ground, remote mode is no longer needed
    if(!at_ground)
    {  
        serial_fd_do(serial_get_SDM()->master.fd, ":CONT:GTGR", NULL, 0, NULL);
        OUTPUT_PRINT("Sent Control to ground command, Exiting");
    }
    else
        OUTPUT_PRINT("Exiting");

    serial_fd_do(serial_get_SDM()->master.fd, ":SYST:REMOTE DISABLE", NULL, 0, NULL); 
    serial_fd_do(serial_get_SDM()->slave.fd, ":SYST:REMOTE DISABLE", NULL, 0, NULL); 
}

