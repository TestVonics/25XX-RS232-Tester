#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

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

bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate)
{
    SetCommand *commands[8];
    int command_index = 2;

    //control mode
    static const SetCommandExpectString sysmode = {{EXP_TYPE_STR, ":SYST:MODE CTRL", ":SYST:MODE?"}, "CTRL"};
    commands[0] = (SetCommand*)&sysmode;

    //single or dual channel
    static const SetCommandExpectString mode_dual = {{EXP_TYPE_STR, ":CONT:MODE DUAL", ":CONT:MODE?"}, "DUAL"};
    static const SetCommandExpectString mode_ps = {{EXP_TYPE_STR, ":CONT:MODE PS", ":CONT:MODE?"}, "PS"};
    static const SetCommandExpectString mode_pt = {{EXP_TYPE_STR, ":CONT:MODE PT", ":CONT:MODE?"}, "PT"};

    //units, setpoints, and rates    
    commands[1] = NULL;
    //ps
    SetCommandExpectString ps_units_cmd;
    SetCommandExpectString ps_setp_cmd;
    SetCommandExpectString ps_rate_cmd;
    if(op & CTRL_OP_PS)
    {
        commands[1] = (SetCommand*)&mode_ps;            
        commands[command_index++] = (SetCommand*)SetCommandExpectString_construct(&ps_units_cmd, ":CONT:PS:UNITS", ps_units,  ":CONT:PS:UNITS?");  
        if(ps)
        {               
            commands[command_index++] = (SetCommand*)SetCommandExpectString_construct(&ps_setp_cmd, ":CONT:PS:SETP", ps, ":CONT:PS:SETP?");             
        }

        if(ps_rate)
        {           
            commands[command_index++] = (SetCommand*)SetCommandExpectString_construct(&ps_rate_cmd, ":CONT:PS:RATE", ps_rate, ":CONT:PS:RATE?");           
        }
 
    }
    //pt
    SetCommandExpectString pt_units_cmd;
    SetCommandExpectString pt_setp_cmd;
    SetCommandExpectFP     pt_rate_cmd;
    if(op & CTRL_OP_PT)
    {
        commands[1] = (SetCommand*)&mode_pt;        
        commands[command_index++] = (SetCommand*)SetCommandExpectString_construct(&pt_units_cmd, ":CONT:PT:UNITS", pt_units, ":CONT:PT:UNITS?");
        
        if(pt)
        {            
            commands[command_index++] = (SetCommand*)SetCommandExpectString_construct(&pt_setp_cmd, ":CONT:PT:SETP", pt, ":CONT:PT:SETP?");               
        }

        if(pt_rate)
        {            
            commands[command_index++] = (SetCommand*)SetCommandExpectFP_construct(&pt_rate_cmd, ":CONT:PT:RATE", pt_rate, ":CONT:PT:RATE?", strtod(pt_rate, NULL));           
        }        
    }
    //dual
    if(op == CTRL_OP_DUAL)
        commands[1] = (SetCommand*)&mode_dual; 
    //unknown channel
    if(commands[1] == NULL)
        return false;
   
    //loop through all of the commands and make sure they turn out as expected
    for(int i = 0; i < command_index; i++)
    {
        serial_write(commands[i]->cmd);
        if(commands[i]->type & EXP_TYPE_STR)
        {
            if(!command_and_check_result_str(commands[i]->cmd_verify, ((SetCommandExpectString*)commands[i])->expected))
                return false;
        }
        else if(commands[i]->type & EXP_TYPE_FP)
        {            
            if(!command_and_check_result_float(commands[i]->cmd_verify, ((SetCommandExpectFP*)commands[i])->expected))
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
    serial_write(":CONT:EXEC");
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
