#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdatomic.h>

#include "command.h"
#include "serial.h"
#include "status.h"
#include "utility.h"
#include "control.h"

static_assert(sizeof(QUE) == sizeof(int), "que isn't the same size as int");
static_assert(sizeof(OPR) == sizeof(int), "opr isn't the same size as int");
static_assert(sizeof(STB) == sizeof(int), "stb isn't the same size as int");
static_assert(sizeof(ESB) == sizeof(int), "esb isn't the same size as int");

static inline void command_get_integer(const int fd, const char *text_cmd, ICommandResult *command);
static inline void SetCommand_implement(SetCommand *instance, const char *cmd, const char *cmd_data, const char *cmd_verify, EXP_TYPE type);
static inline bool command_gtg(const int fd);

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

void command_get_integer(const int fd, const char *text_cmd, ICommandResult *command)
{
    if(!serial_integer_cmd(fd, text_cmd, &command->result))  
        command->succeed = false;    
    else
        command->succeed = true;
}

bool command_and_check_result_str_fd(const int fd, const char *msg, const char *expected_result)
{
    int n;
    char buf[256];
    if(!serial_fd_do(fd, msg, buf, sizeof(buf), &n))
        return false;
    
    if((n-1) != (int)strlen(expected_result))
        return false;

    if(strncmp(expected_result, buf, n-1) != 0)
        return false;

    return true;
}

bool command_and_check_result_float_fd(const int fd, const char *msg, double expected_result)
{
    int n;
    char buf[256];
    if(!serial_fd_do(fd, msg, buf, sizeof(buf), &n))
        return false;

    double recvD = strtod(buf, NULL);
    if(expected_result != recvD)
    {
        ERROR_PRINT("Command expected_result was %f, but %f was recieved\n", expected_result, recvD);
        return false;
    }

    return true;
}

/*
bool command_check_sys_err()
{
    serial_write(":SYST:ERR?"); 
    int n;
    char buf[256];   
    if((n = serial_read_or_timeout(buf, sizeof(buf), 5000)) <= 0)    
        return false; 
    return true;
}
*/

//Returns false if an eror occurs
//otherwise 
inline StatQuesEven command_StatQuesEven(const int adts_fd)
{
    StatQuesEven sqe;   
    command_get_integer(adts_fd, ":STAT:QUES:EVEN?", (ICommandResult*)&sqe);
    return sqe;
}

inline StatOperEven command_StatOperEven(const int adts_fd)
{
    StatOperEven soe;
    command_get_integer(adts_fd, ":STAT:OPER:EVEN?", (ICommandResult*)&soe);
    return soe;
}

inline ESR command_ESR(const int adts_fd)
{
    ESR esr;
    
    command_get_integer(adts_fd, "*ESR?", (ICommandResult*)&esr);
    
    return esr;
}

inline STBCommand command_STB(const int adts_fd)
{
    STBCommand stbc;

    command_get_integer(adts_fd, "*STB?", (ICommandResult*)&stbc);

    return  stbc;
  
}

bool command_gtg(const int fd)
{       
    serial_fd_do(fd, ":SYST:MODE CTRL", NULL, 0, NULL);
    serial_fd_do(fd, ":CONT:MODE DUAL", NULL, 0, NULL);
    serial_fd_do(fd, ":CONT:EXEC", NULL, 0, NULL);    
    serial_fd_do(fd, ":CONT:GTGR", NULL, 0, NULL);     
    return true;
}

bool command_gtg_on_error(const int fd)
{
    serial_fd_do(fd, ":CONT:EXEC", NULL, 0, NULL); 
    serial_fd_do(fd, ":CONT:GTGR", NULL, 0, NULL);    
    serial_fd_do(fd, "*CLS", NULL, 0, NULL);         
    return true;

}

bool command_GTG_eventually(const int fd)
{   
    if(status_check_event_registers(OPR_GTG, fd) == ST_AT_GOAL)
    {
        return true;
    }
    return control(0, "INHG", "INHG", OPR_GTG, command_gtg, command_gtg_on_error, NULL, fd);
}

/*
static atomic_flag command_global_lock = ATOMIC_FLAG_INIT;
static char buf[256];
void *command_main_loop(void *ptr)
{
    for(;;)
    {
        bool reading_error;
        while(atomic_flag_test_and_set_explicit(&command_global_lock, memory_order_acquire));
        
        int n;
        
        if((n = serial_read_or_timeout(buf, sizeof(buf), 500)) > 0)
        {
            if(reading_error)
            {
                reading_error = false;
            }
            else if(strncmp(buf, "ERROR", strlen("ERROR")) == 0)
            {
                serial_write(":SYST:ERR?");
                reading_error = true;                  
            }            
            else
                reading_error = false;
        }
        else
            reading_error = false;

        if(!reading_error)        
            atomic_flag_clear_explicit(&command_global_lock, memory_order_release);
    }
    return NULL;
}
*/