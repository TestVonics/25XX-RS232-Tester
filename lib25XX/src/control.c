#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "command.h"
#include "control.h"
#include "serial.h"
#include "utility.h"
#include "status.h"
#include "test.h"

static bool control_set_units(const CTRL_UNITS units, const char **ps_units, const char **pt_units, const char **ps_rate_units_part, const char **pt_rate_units_part);
static bool control_setup(const CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate);
static bool measure_setup(const ADTS *adts, const CTRL_OP op);
//bool control_dual_FK(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, const uint64_t exp_time);
//bool control_dual_INHG(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, const uint64_t exp_time);

bool control_set_units(const CTRL_UNITS units, const char **ps_units, const char **pt_units, const char **ps_rate_units_part, const char **pt_rate_units_part)
{
    if(units & CTRL_UNITS_FK)
    {
        *ps_units = "FT";
        *pt_units = "KTS";
        *ps_rate_units_part = "F";
        *pt_rate_units_part = "K";
    }
    else if(units & CTRL_UNITS_INHG)
    {
        *ps_units = "INHG";
        *ps_rate_units_part = *ps_units;
        *pt_units = "INHG";
        *pt_rate_units_part = *pt_units;
    }
    else
    {
        ERROR_PRINT("Invalid units, passed in units is %d", (int)units); 
        return false;
    }
    return true;
}
#define PS_FMT_PART           "PS TARGET: %s %s PS RATE: %s %sPM"
#define PT_FMT_PART           "PT TARGET: %s %s PT RATE %s %sPM"
#define SETTING_UP_ADTS_TEXT  "Setting up ADTS Control:\n"
#define CONTROL_NOW_TEXT      "System is NOW Controlling"
#define SETPOINT_REACHED_TEXT "Setpoint reached, verifying it remains STABLE for 30 seconds, checking every 5 seconds"

bool control_run_test(const ControlTest *test)
{
    const char *ps_units;
    const char *ps_rate_units_part;
    const char *pt_units;
    const char *pt_rate_units_part;

    if(!control_set_units(test->units, &ps_units, &pt_units, &ps_rate_units_part, &pt_rate_units_part))
        return false;

    OUTPUT_PRINT( SETTING_UP_ADTS_TEXT PS_FMT_PART " " PT_FMT_PART, test->ps, ps_units, test->ps_rate, ps_rate_units_part, test->pt, pt_units, test->pt_rate, pt_rate_units_part);   
    if(!control_setup(CTRL_OP_DUAL, ps_units, pt_units, test->ps, test->ps_rate, test->pt, test->pt_rate))
        return false;
    
    OUTPUT_PRINT("ADTS Control setup complete, " CONTROL_NOW_TEXT);    
    if(!control(test->duration, ps_units, pt_units, OPR_STABLE, NULL, NULL, NULL))
        return false;

    OUTPUT_PRINT(SETPOINT_REACHED_TEXT);  
    for(uint64_t start = time_in_ms(); (time_in_ms() - start) < 30000; )
    {
        if(status_check_event_registers(OPR_STABLE) != ST_AT_GOAL)
            return false;
        sleep(5);
    }
    return true;
}

static void measure_ps_test1()
{
    measure_rate(&serial_get_SDM()->slave, CTRL_OP_PS, "", 0, 0);
}

bool control_single_channel_test(const struct SingleChannelTest *test)
{
    const char *ps_units;
    const char *ps_rate_units_part;
    const char *pt_units;
    const char *pt_rate_units_part;

    if(!control_set_units(test->units, &ps_units, &pt_units, &ps_rate_units_part, &pt_rate_units_part))
        return false;
    
    //Setup the main unit to control
    OPR opr;    
    if(test->op == CTRL_OP_PS)
    {
        OUTPUT_PRINT(SETTING_UP_ADTS_TEXT PS_FMT_PART, test->ps, ps_units, test->ps_rate, ps_rate_units_part);   
        //a single channel ps test is really a dual channel test with 0 knots
        if(!control_setup(CTRL_OP_DUAL, ps_units, pt_units, test->ps, test->ps_rate, "0", "0"))
            return false;
        opr = OPR_PS_STABLE;
    }
    else if(test->op == CTRL_OP_PT)
    {
        OUTPUT_PRINT(SETTING_UP_ADTS_TEXT PT_FMT_PART, test->pt, pt_units, test->pt_rate, pt_rate_units_part);
        if(!control_setup(test->op, NULL, pt_units, NULL, NULL, test->pt, test->pt_rate))
            return false;
        opr = OPR_PT_STABLE;
    }
    else
    {
        DEBUG_PRINT("INVALID CHANNEL");
        return false;
    }

    //Put the other unit in measure mode
    if(!measure_setup(&serial_get_SDM()->slave, test->op))
        return false;
    
    //Start controling
    OUTPUT_PRINT("ADTS Single Channel Control setup complete, " CONTROL_NOW_TEXT);    
    if(!control(test->duration, ps_units, pt_units, opr, NULL, NULL, measure_ps_test1))
        return false;

    OUTPUT_PRINT(SETPOINT_REACHED_TEXT);  
    for(uint64_t start = time_in_ms(); (time_in_ms() - start) < 30000; )
    {
        if(status_check_event_registers(opr) != ST_AT_GOAL)
            return false;
        sleep(5);
    }
    return true;
}

/*
bool control_dual_FK(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, uint64_t exp_time)
{
    if(!control_setup(CTRL_OP_DUAL, "FT", "KTS", ps, ps_rate, pt, pt_rate))
        return false;

    return control(CTRL_OP_DUAL, exp_time);
}

bool control_dual_INHG(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, uint64_t exp_time)
{
    if(!control_setup(CTRL_OP_DUAL, "INHG", "INHG", ps, ps_rate, pt, pt_rate))
        return false;

    return control(CTRL_OP_DUAL, exp_time);
}
*/

bool control_setup(const CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate)
{
    const SetCommandFull *commands[8];
    int command_index = 2;

    //system mode to control mode
    static const SetCommandExpectString sysmode = {{EXP_TYPE_STR, ":SYST:MODE CTRL", ":SYST:MODE?"}, "CTRL"};
    commands[0] = (SetCommandFull*)&sysmode;     

    //control channel
    static const SetCommandExpectString mode_dual = {{EXP_TYPE_STR, ":CONT:MODE DUAL", ":CONT:MODE?"}, "DUAL"};
    static const SetCommandExpectString mode_ps = {{EXP_TYPE_STR, ":CONT:MODE PS", ":CONT:MODE?"}, "PS"};
    static const SetCommandExpectString mode_pt = {{EXP_TYPE_STR, ":CONT:MODE PT", ":CONT:MODE?"}, "PT"};
    commands[1] = NULL;    

    //Units commands, we must do this before trying to set setpoints and rates
    SetCommandExpectString ps_units_cmd;
    if(op & CTRL_OP_PS)
    {        
        commands[command_index++] = (SetCommandFull*)SetCommandExpectString_construct(&ps_units_cmd, ":CONT:PS:UNITS", ps_units,  ":CONT:PS:UNITS?"); 
    }

    SetCommandExpectString pt_units_cmd;
    if(op & CTRL_OP_PT)
    {
        commands[command_index++] = (SetCommandFull*)SetCommandExpectString_construct(&pt_units_cmd, ":CONT:PT:UNITS", pt_units, ":CONT:PT:UNITS?");
    }

    //Mode, setpoints, and rates    
    //ps    
    SetCommandFull         ps_setp_cmd;
    SetCommandFull         ps_rate_cmd; 
    if(op & CTRL_OP_PS)
    {        
        commands[1] = (SetCommandFull*)&mode_ps;   
        EXP_TYPE ps_setp_exp_type, ps_rate_exp_type;
        if(strncmp(ps_units, "FT", strlen("FT")+1) == 0)
        {
            ps_setp_exp_type = EXP_TYPE_STR;
            ps_rate_exp_type = EXP_TYPE_STR;        
        }
        else if(strncmp(ps_units, "INHG", strlen("INHG")+1) == 0)
        {
            ps_setp_exp_type = EXP_TYPE_FP;
            ps_rate_exp_type = EXP_TYPE_FP;
        }
        else
            return false;      
        if(ps)
        {               
            commands[command_index++] = SetCommandFull_construct(&ps_setp_cmd, ps_setp_exp_type, ":CONT:PS:SETP", ps, ":CONT:PS:SETP?");             
        }

        if(ps_rate)
        {           
            commands[command_index++] = SetCommandFull_construct(&ps_rate_cmd, ps_rate_exp_type, ":CONT:PS:RATE", ps_rate, ":CONT:PS:RATE?");           
        }
 
    }
    //pt       
    SetCommandFull     pt_setp_cmd;
    SetCommandFull     pt_rate_cmd;
    if(op & CTRL_OP_PT)
    {
        commands[1] = (SetCommandFull*)&mode_pt;   
        EXP_TYPE pt_setp_exp_type, pt_rate_exp_type;
        if(strncmp(pt_units, "KTS", strlen("KTS")+1) == 0)
        {
            //pt_setp_exp_type = EXP_TYPE_STR;
            //pt_rate_exp_type = EXP_TYPE_FP;
            pt_setp_exp_type = EXP_TYPE_FP;
            pt_rate_exp_type = EXP_TYPE_FP;        
        }
        else if(strncmp(pt_units, "INHG", strlen("INHG")+1) == 0)
        {
            pt_setp_exp_type = EXP_TYPE_FP;
            pt_rate_exp_type = EXP_TYPE_FP;
        }
        else
            return false;     
        if(pt)
        {            
            commands[command_index++] = SetCommandFull_construct(&pt_setp_cmd, pt_setp_exp_type, ":CONT:PT:SETP", pt, ":CONT:PT:SETP?");               
        }

        if(pt_rate)
        {            
            commands[command_index++] = SetCommandFull_construct(&pt_rate_cmd, pt_rate_exp_type, ":CONT:PT:RATE", pt_rate, ":CONT:PT:RATE?");           
        }        
    }

    //dual channel
    if(op == CTRL_OP_DUAL)
        commands[1] = (SetCommandFull*)&mode_dual; 
    else if(commands[1] == NULL) //unknown channel
        return false;    
   
    //loop through all of the commands and make sure they turn out as expected
    const SetCommand *currentCommand;
    for(int i = 0; i < command_index; i++)
    {
        currentCommand = (SetCommand*)commands[i];
        if(!serial_do(currentCommand->cmd, NULL, 0, NULL))
            return false;
        if(currentCommand->type & EXP_TYPE_STR)
        {
            if(!command_and_check_result_str(currentCommand->cmd_verify, ((SetCommandExpectString*)currentCommand)->expected))
            {
                /*
                if(!serial_do(currentCommand->cmd, NULL, 0, NULL))
                    return false;
                if(!command_and_check_result_str(currentCommand->cmd_verify, ((SetCommandExpectString*)currentCommand)->expected))
                    return false;
                */
            }
        }
        else if(currentCommand->type & EXP_TYPE_FP)
        {            
            if(!command_and_check_result_float(currentCommand->cmd_verify, ((SetCommandExpectFP*)currentCommand)->expected))
            {   /*
                if(!serial_do(currentCommand->cmd, NULL, 0, NULL))
                    return false;
                if(!command_and_check_result_float(currentCommand->cmd_verify, ((SetCommandExpectFP*)currentCommand)->expected))
                    return false;
                */ 
            }
            
        }        
    }  
    
    return true;
}


bool control(uint64_t exp_time, const char *ps_units, const char *pt_units, OPR success_mask, Control_Start_Func start_func, Control_On_Error on_error, Control_EachCycle cycle_func)
{
    if(exp_time == 0)
        exp_time = UINT64_MAX;    
    
    //EXECUTE
    if(start_func == NULL)
        serial_do(":CONT:EXEC", NULL, 0, NULL); 
    else
        start_func();   
    uint64_t start = time_in_ms();
    STATUS st;
    //While we are not stable, check every 5 seconds    
    for(;;)
    {
        if((st = status_check_event_registers(success_mask)) == ST_ERR)
        {
            if((on_error == NULL) || (!on_error()))
                return false; 
         
        }      
 
        status_dump_pressure_data_if_different(ps_units, pt_units);

        if(st == ST_AT_GOAL)
            break;

        if((time_in_ms()- start) > exp_time)
        {
            OUTPUT_PRINT("Timeout, control not performed in %llu\n", (long long unsigned)exp_time);
            return false;
        } 

        if(cycle_func != NULL)
            cycle_func();   
               
        sleep(5);
    }

    return true;
}

bool measure_setup(const ADTS *adts, const CTRL_OP op)
{
    //set the channel
    const char *set_channel;
    const char *expected;

    //has to be dual
    if(op & CTRL_OP_PS)
    {
        set_channel = ":MEAS:MODE DUAL";
        expected = "DUAL";
    }
    else if(op & CTRL_OP_PT)
    {
        set_channel = ":MEAS:MODE PT";
        expected = "PT";
    }
    DEBUG_PRINT("ADTS fd is %d", adts->fd);
    if(!serial_fd_do(adts->fd, set_channel, NULL, 0, NULL))
        return false;         
    if(!command_and_check_result_str_fd(adts->fd, ":MEAS:MODE?", expected))
        return false;

    //put in measure mode
    if(!serial_fd_do(adts->fd, ":SYST:MODE MEAS", NULL, 0, NULL))
        return false;
    if(!command_and_check_result_str_fd(adts->fd, ":SYST:MODE?", "MEAS"))
        return false; 

    //start climb test
    if(!serial_fd_do(adts->fd, ":MEAS:CLIMB:TEST ON", NULL, 0, NULL))
        return false;     
    if(!command_and_check_result_str_fd(adts->fd, ":MEAS:CLIMB:TEST?", "ON"))
        return false;

    return true;
}

bool measure_rate(const ADTS *adts, const CTRL_OP op, const char *units,  const double expected_rate, const double max_difference)
{
    //read the rate of climb    
    char buf[256];
    if(!serial_fd_do(adts->fd, ":MEAS:CLIMB:RATE? FPM", buf, sizeof(buf), NULL))
        return false;

    return true;
}










