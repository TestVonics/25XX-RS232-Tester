#include <string.h>
#include <stdio.h>

#include "utility.h"
#include "test.h"
#include "serial.h"
#include "command.h"
#include "lsu.h"

static inline bool lsu_check_valve_state(const char *valve, const char *expected_state)
{
    char valve_state_cmd[32];
    snprintf(valve_state_cmd, sizeof(valve_state_cmd), "OUTP:VALV:STAT? %s", valve);
    if(!command_and_check_result_str_fd(serial_get_SDM()->lsu.fd, valve_state_cmd, expected_state))
    {
         ERROR_PRINT("Valve %s is in unexpected state",  valve);
         return false;
    }
    return true;
}

static inline bool lsu_check_err(const char *valve)
{        
    char valve_err_cmd[32]; 
    snprintf(valve_err_cmd, sizeof(valve_err_cmd), "OUTP:VALV:ERR? %s", valve);
    if(!command_and_check_result_str_fd(serial_get_SDM()->lsu.fd, valve_err_cmd, "0"))
    {
         ERROR_PRINT("Valve %s has a possible error",  valve);
         return false;
    }
    return true;
}

static inline bool lsu_check_POST()
{        
    char cmd[32]; 
    snprintf(cmd, sizeof(cmd), "*TST?");
    if(!command_and_check_result_str_fd(serial_get_SDM()->lsu.fd, cmd, "1"))
    {
         char buf[256];
         serial_fd_do(serial_get_SDM()->lsu.fd, "SYST:ERR?", buf, sizeof(buf), NULL);
         ERROR_PRINT("LSU POST failed with: %s", buf);
         return false;
    }
    return true;
}

static inline bool lsu_check_num_valves()
{        
    char cmd[32]; 
    snprintf(cmd, sizeof(cmd), "OUTP:VALV:MAX?");
    if(!command_and_check_result_str_fd(serial_get_SDM()->lsu.fd, cmd, "8"))
    {
         ERROR_PRINT("Unexpected amount of output valves are fitted to the LSU instead of %s valves", "8");
         return false;
    }
    return true;
}

static inline bool lsu_run_selftest()
{
    if(!command_and_check_result_float_fd(serial_get_SDM()->lsu.fd, "TEST:SWIT:ACT?", 1))
    {
         ERROR_PRINT("SELF TEST FAILED");
         return false;
    }
    return true;
}

static inline bool lsu_set_valve_state(const char *valve, const char *state)
{
    char set_valve_state[32];
    snprintf(set_valve_state, sizeof(set_valve_state), "OUTP:VALV:STAT %s %s", valve, state);
    if(!serial_fd_do(serial_get_SDM()->lsu.fd, set_valve_state, NULL, 0, NULL))
        return false;

    return lsu_check_valve_state(valve, state);
}

bool lsu_valve_test(const LSUValveTest *lsu_valve_test)
{
    if(!serial_fd_do(serial_get_SDM()->lsu.fd, "*CLS", NULL, 0, NULL))
        return false;

    //If valve_number is ALL, check values pertaining to all valves
    if(strncmp("ALL", lsu_valve_test->valve_number, strlen("ALL")+1) == 0)
    {
        if(!lsu_check_POST())
            return false;

        if(!lsu_check_num_valves())
            return false;
        
        if(!lsu_run_selftest())
            return false;
        
        //open all the valves
        if(!serial_fd_do(serial_get_SDM()->lsu.fd, "OUTP:ALL OPEN", NULL, 0, NULL))
            return false;
        
        //close all the valves
        if(!serial_fd_do(serial_get_SDM()->lsu.fd, "OUTP:ALL CLOSE", NULL, 0, NULL))
            return false;
        
        
        return true;
    }
    
    OUTPUT_PRINT("Verifying valve %s status", lsu_valve_test->valve_number);    

    char cmd[32], result[32];
    //check to see if the valve is fitted
    snprintf(cmd, sizeof(cmd), "OUTP:VALV:CONF? %s", lsu_valve_test->valve_number);
    if(!serial_fd_do(serial_get_SDM()->lsu.fd, cmd, result, sizeof(result), NULL))
        return false;
    if(strncmp(result, "*** Not Fitted***", strlen("*** Not Fitted***")+1) == 0)
    {
        ERROR_PRINT("Valve %s is not fitted",  lsu_valve_test->valve_number);
        return false;
    }   
    
    //assert it reports that it is closed because we closed all the valves to start
    if(!lsu_check_valve_state(lsu_valve_test->valve_number, "CLOSE"))
        return false;
 
    //check if the valve is working correctly
    if(!lsu_check_err(lsu_valve_test->valve_number))
        return false;

    //Attempt to open the valve
    OUTPUT_PRINT("Opening valve %s", lsu_valve_test->valve_number);
    if(!lsu_set_valve_state(lsu_valve_test->valve_number, "OPEN"))
        return false;

    //check if the valve is working correctly
    if(!lsu_check_err(lsu_valve_test->valve_number))
        return false;

    //Confirm it actually opened
    OUTPUT_PRINT("Is the indicator light for valve %s turned on? ", lsu_valve_test->valve_number);    
    if(!Yes_No())
    {
        OUTPUT_PRINT("No");
        return false;
    }    
    OUTPUT_PRINT("Yes");


    //Attempt to close the valve
    OUTPUT_PRINT("Close valve %s", lsu_valve_test->valve_number);
    if(!lsu_set_valve_state(lsu_valve_test->valve_number, "CLOSE"))
        return false;

    //check if the valve is working correctly
    if(!lsu_check_err(lsu_valve_test->valve_number))
        return false;

    //Confirm it actually closed
    OUTPUT_PRINT("Is the indicator light for valve %s turned off? ", lsu_valve_test->valve_number);    
    if(!Yes_No())
    {
        OUTPUT_PRINT("No");
        return false;
    }    
    OUTPUT_PRINT("Yes");

    return true;
}