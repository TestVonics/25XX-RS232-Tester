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

static inline void command_try_integer(const char *text_cmd, IBaseCommand *command)
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
    command_try_integer(":STAT:QUES:EVEN?", (IBaseCommand*)&sqe);
    return sqe;
}

inline StatOperEven command_StatOperEven()
{
    StatOperEven soe;
    command_try_integer(":STAT:OPER:EVEN?", (IBaseCommand*)&soe);
    return soe;
}

static inline ESR command_ESR()
{
    ESR esr;
    
    command_try_integer("*ESR?", (IBaseCommand*)&esr);
    
    return esr;
}

static inline STBCommand command_STB()
{
    STBCommand stbc;

    command_try_integer("*STB?", (IBaseCommand*)&stbc);

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
            
       

 
