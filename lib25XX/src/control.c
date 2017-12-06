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

typedef enum {
    CTRL_OP_PS        = 1 << 0,
    CTRL_OP_PT        = 1 << 1,
    CTRL_OP_DUAL      = (CTRL_OP_PS | CTRL_OP_PT)
} CTRL_OP;



static bool control(CTRL_OP op, uint64_t exp_time);
static bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate);
bool control_dual_FK(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, uint64_t exp_time);
bool control_dual_INHG(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, uint64_t exp_time);

bool control_run_test(const ControlTest *test)
{
    const char *ps_units;
    const char *ps_rate_units_part;
    const char *pt_units;
    const char *pt_rate_units_part;
    if(test->units & CTRL_UNITS_FK)
    {
        ps_units = "FT";
        pt_units = "KTS";
        ps_rate_units_part = "F";
        pt_rate_units_part = "K";
    }
    else if(test->units & CTRL_UNITS_INHG)
    {
        ps_units = "INHG";
        ps_rate_units_part = ps_units;
        pt_units = "INHG";
        pt_rate_units_part = pt_units;
    }
    else
    {
        ERROR_PRINT("Invalid units, passed in units is %d", (int)test->units); 
    }

    OUTPUT_PRINT("Setting up ADTS Control:\n PS TARGET: %s %s PS RATE: %s %sPM PT TARGET: %s %s PT RATE %s %sPM", test->ps, ps_units, test->ps_rate, ps_rate_units_part, test->pt, pt_units, test->pt_rate, pt_rate_units_part);   
    if(!control_setup(CTRL_OP_DUAL, ps_units, pt_units, test->ps, test->ps_rate, test->pt, test->pt_rate))
        return false;
    
    OUTPUT_PRINT("ADTS Control setup complete, System is NOW Controlling");    
    return control(CTRL_OP_DUAL, test->duration);
}

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

bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate)
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


bool control(CTRL_OP op, uint64_t exp_time)
{
    OPR success_mask;
    OPR check_states[2] = {(OPR)NULL, (OPR)NULL};
    static const OPR ps_good = (OPR_PS_RAMPING | OPR_PS_STABLE);
    static const OPR pt_good = (OPR_PT_RAMPING | OPR_PT_STABLE);
    if(op == CTRL_OP_DUAL)
    {
        success_mask = OPR_STABLE;
        check_states[0] = ps_good;
        check_states[1] = pt_good;
    }
    else if(op & CTRL_OP_PS)
    {
        check_states[0] = ps_good;
        success_mask = OPR_PS_STABLE;
    }
    else if(op & CTRL_OP_PT)
    {
        check_states[0] = pt_good;
        success_mask = OPR_PT_STABLE;
    }
    
    
    //EXECUTE
    serial_do(":CONT:EXEC", NULL, 0, NULL);   
    StatOperEven soe;
    uint64_t start = time_in_ms();

    //While we are not stable, check every 5 seconds    
    while(!((soe = command_StatOperEven()).opr & success_mask))
    { 
        /*
        //This strategy doesn't work because sometimes it doesn't report
        //one of the channels
        if(!(soe.opr & check_states[0]) )
        {
            printf("Error: PS is in INVALID STATE\n");
            status_check_event_registers();
            return false;
        }
     
        if(check_states[1] != (OPR)NULL)
        {
            if(!(soe.opr & check_states[1]))
            {
                printf("Error: PT is in INVALID STATE\n");
                status_check_event_registers();
                return false;
            }
        }*/

        if(status_check_event_registers() == ST_ERR)
            return false;   

        if((time_in_ms()- start) > exp_time)
        {
            OUTPUT_PRINT("Timeout, control not performed in %llu\n", (long long unsigned)exp_time);
            return false;
        }    
               
        sleep(5);
    }

    return true;
}
