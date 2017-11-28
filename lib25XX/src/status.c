#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "status.h"
#include "serial.h"

static_assert(sizeof(QUE) == sizeof(int), "que isn't the same size as int");
static_assert(sizeof(OPR) == sizeof(int), "opr isn't the same size as int");
static_assert(sizeof(STB) == sizeof(int), "stb isn't the same size as int");
static_assert(sizeof(ESB) == sizeof(int), "esb isn't the same size as int");

static bool control(CTRL_OP op);
static bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate);

//if there isn't data pass an empty string not NULL for cmd_data
SetCommandExpectString SetCommandExpectString_construct(const char *cmd, const char *cmd_data, const char *cmd_verify)
{
    SetCommandExpectString sces = {.type = EXP_TYPE_STR, .cmd_verify = cmd_verify, .expected = cmd_data};
    strcpy(sces.cmd, cmd);
    strcat(sces.cmd, " ");
    strcat(sces.cmd, cmd_data);
    return sces;
}

SetCommandExpectFP SetCommandExpectFP_construct(const char *cmd, const char *cmd_data, const char *cmd_verify, double expected)
{
    SetCommandExpectFP scefp = {.type = EXP_TYPE_FP, .cmd_verify = cmd_verify, .expected = expected};
    strcpy(scefp.cmd, cmd);
    strcat(scefp.cmd, " ");
    strcat(scefp.cmd, cmd_data);
    return scefp;
}



bool control_dual_FK(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate)
{
    if(!control_setup(CTRL_OP_DUAL, "FT", "KTS", ps, ps_rate, pt, pt_rate))
        return false;

    return control(CTRL_OP_DUAL);
}

bool control_setup(CTRL_OP op, const char *ps_units, const char *pt_units, const char *ps, const char *ps_rate, const char *pt, const char *pt_rate)
{
    SetCommand *commands[8];
    int command_index = 2;

    //control mode
    static const SetCommandExpectString sysmode = {EXP_TYPE_STR, ":SYST:MODE CTRL", ":SYST:MODE?", "CTRL"};
    commands[0] = &sysmode;

    //single or dual channel
    static const SetCommandExpectString mode_dual = {EXP_TYPE_STR, ":CONT:MODE DUAL", ":CONT:MODE?", "DUAL"};
    static const SetCommandExpectString mode_ps = {EXP_TYPE_STR, ":CONT:MODE PS", ":CONT:MODE?", "PS"};
    static const SetCommandExpectString mode_pt = {EXP_TYPE_STR, ":CONT:MODE PT", ":CONT:MODE?", "PT"};

    //units, setpoints, and rates    
    commands[1] = NULL;
    //ps
    if(op & CTRL_OP_PS)
    {
        commands[1] = &mode_ps;
        SetCommandExpectString ps_units_cmd = SetCommandExpectString_construct(":CONT:PS:UNITS", ps_units,  ":CONT:PS:UNITS?");     
        commands[command_index++] = (SetCommand*)&ps_units_cmd;
        if(ps)
        {
            SetCommandExpectString ps_setp_cmd = SetCommandExpectString_construct(":CONT:PS:SETP", ps, ":CONT:PS:SETP?");  
            commands[command_index++] = (SetCommand*)&ps_setp_cmd;              
        }

        if(ps_rate)
        {
            SetCommandExpectString ps_rate_cmd = SetCommandExpectString_construct(":CONT:PS:RATE", ps_rate, ":CONT:PS:RATE?");
            commands[command_index++] = (SetCommand*)&ps_rate_cmd;           
        }
 
    }
    //pt
    if(op & CTRL_OP_PT)
    {
        commands[1] = &mode_pt;
        SetCommandExpectString pt_units_cmd = SetCommandExpectString_construct(":CONT:PT:UNITS", pt_units, ":CONT:PT:UNITS?");
        commands[command_index++] = (SetCommand*)&pt_units_cmd;
        
        if(pt)
        {
            SetCommandExpectString pt_setp_cmd = SetCommandExpectString_construct(":CONT:PT:SETP", pt, ":CONT:PT:SETP?");
            commands[command_index++] = (SetCommand*)&pt_setp_cmd;                
        }

        if(pt_rate)
        {
            SetCommandExpectFP    pt_rate_cmd = SetCommandExpectFP_construct(":CONT:PT:RATE", pt_rate, ":CONT:PT:RATE?", strtod(pt_rate, NULL)); 
            commands[command_index++] = (SetCommand*)&pt_rate_cmd;           
        }        
    }
    //dual
    if(op == CTRL_OP_DUAL)
        commands[1] = &mode_dual; 
    //unknown channel
    if(commands[1] == NULL)
        return false;
   
    //loop through all of the commands and make sure they turn out as expected
    for(int i = 0; i < command_index; i++)
    {
        serial_write(commands[i]->cmd);
        if(commands[i]->type & EXP_TYPE_STR)
        {
            if(!command_try_expect(commands[i]->cmd_verify, ((SetCommandExpectString*)commands[i])->expected))
                return false;
        }
        else if(commands[i]->type & EXP_TYPE_FP)
        {            
            if(!command_try_expect_float(commands[i]->cmd_verify, ((SetCommandExpectFP*)commands[i])->expected))
                return false; 
        }        
    }  
    
    return true;
}


bool control(CTRL_OP op)
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
    while(!((soe = command_StatOperEven()).opr & success_mask))
    { 
        if(!(soe.opr & check_states[0]))
        {
            //printf("Error PS in INVALID STATE\n");
            return false;
        }
     
        if(check_states[1] != (OPR)NULL)
        {
            if(!(soe.opr & check_states[1]))
            {
               // printf("Error PT in INVALID STATE\n");
                return false;
            }
        }       
               
        sleep(5);
    }

    return true;
}

static inline void command_try_integer(const char *text_cmd, ICommandResult *command)
{
    if(!serial_integer_cmd(text_cmd, &command->result))  
        command->succeed = false;    
    else
        command->succeed = true;
}

bool command_try_expect(const char *msg, const char *expected_result)
{
    int n;
    char buf[256];
    serial_write(msg);
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;
    
    if((n-1) != (int)strlen(expected_result))
        return false;

    if(strncmp(expected_result, buf, n-1) != 0)
        return false;

    return true;
}

bool command_try_expect_float(const char *msg, double expected_result)
{
    int n;
    char buf[256];
    serial_write(msg);
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    if(expected_result != strtod(buf, NULL))
        return false;

    return true;
}

bool command_check_sys_err()
{
    serial_write(":SYST:ERR?"); 
    int n;
    char buf[256];   
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false; 
    return true;
}

//Returns false if an eror occurs
//otherwise 
static inline StatQuesEven command_StatQuesEven()
{
    StatQuesEven sqe;   
    command_try_integer(":STAT:QUES:EVEN?", (ICommandResult*)&sqe);
    return sqe;
}

inline StatOperEven command_StatOperEven()
{
    StatOperEven soe;
    command_try_integer(":STAT:OPER:EVEN?", (ICommandResult*)&soe);
    return soe;
}

static inline ESR command_ESR()
{
    ESR esr;
    
    command_try_integer("*ESR?", (ICommandResult*)&esr);
    
    return esr;
}

static inline STBCommand command_STB()
{
    STBCommand stbc;

    command_try_integer("*STB?", (ICommandResult*)&stbc);

    return  stbc;
  
}

bool status_is_idle()
{
    
    STBCommand stbc = command_STB();
    if(!stbc.succeed)
        return false;

    bool bRet = true;
    
      

    /*
    command_StatQuesEven();
    command_StatOperEven();
    command_ESR();
    */
    if(stbc.stb & STB_QUE)
    {
        printf("QUE\n");
        StatQuesEven sqe = command_StatQuesEven();
        if(!sqe.succeed || (sqe.que & QUE_ALL))
        {
            printf("QUE ERROR\n"); 
            bRet = false;
        } 
    }

    if(stbc.stb & STB_ESB)
    {
        printf("ESB\n");
        ESR esr = command_ESR();        
        if(!esr.succeed || (esr.esb & ESB_ERR))
        {
            printf("ESB ERROR\n");  
            bRet = false;
        }
    }

    if(stbc.stb & STB_OPR)
    {
        printf("OPR\n");
        StatOperEven soe = command_StatOperEven();
        if(!soe.succeed || (soe.opr & OPR_ALL))
        {
            printf("OPR ERROR\n");
            bRet = false;
        }  
    }

    if(!bRet)
        return false;

    if(!command_try_expect(":SYST:MODE?", "VENT"))
        return false;

    return true;
}
            
       

 
