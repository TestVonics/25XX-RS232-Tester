#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "status.h"
#include "serial.h"


static inline bool STAT_QUES_EVEN()
{
    serial_write(":STAT:QUES:EVEN?");
    char buf[256];
    int n = serial_read_or_timeout(buf, sizeof(buf), 5000);
    if( n <= 0)
        return false; 

    QUE que = atoi(buf);
    
    if(que & QUE_ALL)
    {
        printf("QUE ERROR\n");
        return false;
    }    

    return true;
}

static inline bool STAT_OPER_EVEN()
{
    serial_write(":STAT:OPER:EVEN?");
    char buf[256];
    int n = serial_read_or_timeout(buf, sizeof(buf), 5000);
    if( n <= 0)
        return false; 

    OPR opr = atoi(buf);
    
    if(opr & OPR_ALL)
    {
        printf("OPR ERROR\n");
        return false;
    }    

    return true;

}

static inline bool ESR()
{
    serial_write("*ESR?");
    char buf[256];
    int n = serial_read_or_timeout(buf, sizeof(buf), 5000);
    if( n <= 0)
        return false; 

    ESB esb = atoi(buf);
    
    if(esb & ESB_ERR)
    {
        printf("ESB ERROR\n");
        return false;
    }    

    return true;

}


bool status_check()
{
    serial_write("*STB?");
    char buf[256];
    int n = serial_read_or_timeout(buf, sizeof(buf), 5000);
    if( n <= 0)
        return false;    
    
    STB stb = atoi(buf);

    //STAT_QUES_EVEN();
    //STAT_OPER_EVEN();
    //ESR();

    if(stb & STB_QUE)
    {
        printf("QUE\n");
        if(!STAT_QUES_EVEN())
            return false;
    }

    if(stb & STB_ESB)
    {
        printf("ESB\n");
        if(!ESR())
            return false;
    }

    if(stb & STB_OPR)
    {
        printf("OPR\n");
        if(!STAT_OPER_EVEN())
            return false;
    }

    return true;    
}