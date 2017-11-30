#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "command.h"
#include "serial.h"

static_assert(sizeof(QUE) == sizeof(int), "que isn't the same size as int");
static_assert(sizeof(OPR) == sizeof(int), "opr isn't the same size as int");
static_assert(sizeof(STB) == sizeof(int), "stb isn't the same size as int");
static_assert(sizeof(ESB) == sizeof(int), "esb isn't the same size as int");

static inline void command_get_integer(const char *text_cmd, ICommandResult *command);
static inline void SetCommand_implement(SetCommand *instance, const char *cmd, const char *cmd_data, const char *cmd_verify, EXP_TYPE type);

void SetCommand_implement(SetCommand *instance, const char *cmd, const char *cmd_data, const char *cmd_verify, EXP_TYPE type)
{
    instance->type = type;
    instance->cmd_verify = cmd_verify;
    strcpy(instance->cmd, cmd);

    if(cmd_data != NULL)
    {
        strcat(instance->cmd, " ");
        strcat(instance->cmd, cmd_data);
    }    
}

SetCommandExpectString *SetCommandExpectString_construct(SetCommandExpectString *instance, const char *cmd, const char *cmd_data, const char *cmd_verify)
{
    SetCommand_implement((SetCommand*)instance, cmd, cmd_data, cmd_verify, EXP_TYPE_STR);
    instance->expected = cmd_data;
    return instance;
}

SetCommandExpectFP *SetCommandExpectFP_construct(SetCommandExpectFP *instance, const char *cmd, const char *cmd_data, const char *cmd_verify)
{
    SetCommand_implement((SetCommand*)instance, cmd, cmd_data, cmd_verify, EXP_TYPE_FP);
    instance->expected = strtod(cmd_data, NULL);
    return instance;
}

SetCommandFull *SetCommandFull_construct(SetCommandFull *instance, const EXP_TYPE type, const char *cmd, const char *cmd_data, const char *cmd_verify)
{
    if(type & EXP_TYPE_STR)
        return (SetCommandFull*)SetCommandExpectString_construct((SetCommandExpectString*)instance, cmd, cmd_data, cmd_verify);
    else if(type & EXP_TYPE_FP)
        return (SetCommandFull*)SetCommandExpectFP_construct((SetCommandExpectFP*)instance, cmd, cmd_data, cmd_verify);
    else
        return NULL;
} 

void command_get_integer(const char *text_cmd, ICommandResult *command)
{
    if(!serial_integer_cmd(text_cmd, &command->result))  
        command->succeed = false;    
    else
        command->succeed = true;
}

bool command_and_check_result_str(const char *msg, const char *expected_result)
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

bool command_and_check_result_float(const char *msg, double expected_result)
{
    int n;
    char buf[256];
    serial_write(msg);
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false;

    double recvD = strtod(buf, NULL);
    if(expected_result != recvD)
    {
        printf("expected_result was %f, but %f was recveived\n", expected_result, recvD);
        return false;
    }

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
inline StatQuesEven command_StatQuesEven()
{
    StatQuesEven sqe;   
    command_get_integer(":STAT:QUES:EVEN?", (ICommandResult*)&sqe);
    return sqe;
}

inline StatOperEven command_StatOperEven()
{
    StatOperEven soe;
    command_get_integer(":STAT:OPER:EVEN?", (ICommandResult*)&soe);
    return soe;
}

inline ESR command_ESR()
{
    ESR esr;
    
    command_get_integer("*ESR?", (ICommandResult*)&esr);
    
    return esr;
}

inline STBCommand command_STB()
{
    STBCommand stbc;

    command_get_integer("*STB?", (ICommandResult*)&stbc);

    return  stbc;
  
}


bool command_check_and_handle_ERROR()
{
    char buf[256];
    if(serial_read_or_timeout(buf, sizeof(buf), 500) > 0) 
    {
        if(strncmp(buf, "ERROR", strlen("ERROR")) == 0)
        {
            serial_write(":SYST:ERR?");  
            serial_read_or_timeout(buf, sizeof(buf), 500);  
            return true;  
        }
    }

    return false;
}
