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

typedef enum {
    CTRL_OP_PS        = 1 << 0,
    CTRL_OP_PT        = 1 << 1,
    CTRL_OP_DUAL      = (CTRL_OP_PS | CTRL_OP_PT)
} CTRL_OP;



static bool control(CTRL_OP op, uint64_t exp_time);
static bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate);

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

    //control mode
    static const SetCommandExpectString sysmode = {{EXP_TYPE_STR, ":SYST:MODE CTRL", ":SYST:MODE?"}, "CTRL"};
    commands[0] = (SetCommandFull*)&sysmode;

    //single or dual channel
    static const SetCommandExpectString mode_dual = {{EXP_TYPE_STR, ":CONT:MODE DUAL", ":CONT:MODE?"}, "DUAL"};
    static const SetCommandExpectString mode_ps = {{EXP_TYPE_STR, ":CONT:MODE PS", ":CONT:MODE?"}, "PS"};
    static const SetCommandExpectString mode_pt = {{EXP_TYPE_STR, ":CONT:MODE PT", ":CONT:MODE?"}, "PT"};

    //units, setpoints, and rates    
    commands[1] = NULL;
   
    
    //ps
    SetCommandExpectString ps_units_cmd;
    SetCommandFull         ps_setp_cmd;
    SetCommandFull         ps_rate_cmd;
    if(op & CTRL_OP_PS)
    {
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
        commands[1] = (SetCommandFull*)&mode_ps;            
        commands[command_index++] = (SetCommandFull*)SetCommandExpectString_construct(&ps_units_cmd, ":CONT:PS:UNITS", ps_units,  ":CONT:PS:UNITS?");  
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
    SetCommandExpectString pt_units_cmd;
    SetCommandFull     pt_setp_cmd;
    SetCommandFull     pt_rate_cmd;
    if(op & CTRL_OP_PT)
    {
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
        commands[1] = (SetCommandFull*)&mode_pt;        
        commands[command_index++] = (SetCommandFull*)SetCommandExpectString_construct(&pt_units_cmd, ":CONT:PT:UNITS", pt_units, ":CONT:PT:UNITS?");
        
        if(pt)
        {            
            commands[command_index++] = SetCommandFull_construct(&pt_setp_cmd, pt_setp_exp_type, ":CONT:PT:SETP", pt, ":CONT:PT:SETP?");               
        }

        if(pt_rate)
        {            
            commands[command_index++] = SetCommandFull_construct(&pt_rate_cmd, pt_rate_exp_type, ":CONT:PT:RATE", pt_rate, ":CONT:PT:RATE?");           
        }        
    }
    //dual
    if(op == CTRL_OP_DUAL)
        commands[1] = (SetCommandFull*)&mode_dual; 
    //unknown channel
    if(commands[1] == NULL)
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
                return false;
        }
        else if(currentCommand->type & EXP_TYPE_FP)
        {            
            if(!command_and_check_result_float(currentCommand->cmd_verify, ((SetCommandExpectFP*)currentCommand)->expected))
                return false; 
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
    sleep(1);
    StatOperEven soe;
    
    //While we are not stable, check every 5 seconds
    uint64_t start = time_in_ms();
    while(!((soe = command_StatOperEven()).opr & success_mask))
    { 
        if(!(soe.opr & check_states[0]))
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
        }   

        if((time_in_ms()- start) > exp_time)
        {
            printf("Timeout, control not performed in %llu\n", (long long unsigned)exp_time);
            return false;
        }    
               
        sleep(5);
    }

    return true;
}
