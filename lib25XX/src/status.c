#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "status.h"
#include "serial.h"
#include "command.h"

bool status_is_idle()
{    
    if(status_check_event_registers() == ST_IDLE_VENT)
    {
        //if the registers are idle and we are vented we are idle
        if(command_and_check_result_str(":SYST:MODE?", "VENT"))
             return true;      
    }

    return false;
}

#define CHECK_ERROR_BIT(INT, COND) \
do { if(INT & COND) \
         fprintf(stderr, "Error: " #COND "\n"); } \
while(0)           

#define CHECK_OPR_BIT(INT, COND) \
do { if(INT & COND) \
         fprintf(stderr, "OPR: " #COND "\n"); } \
while(0)
STATUS status_check_event_registers()
{
    STBCommand stbc = command_STB();
    STATUS status = ST_IDLE_VENT;
    if(!stbc.succeed)
        return ST_ERR;

    //if anything is set we are not idle
    if(stbc.stb & STB_ALL)
        status = ST_NOT_IDLE;

    if(stbc.stb & STB_QUE)
    {        
        StatQuesEven sqe = command_StatQuesEven();
        if(!sqe.succeed || (sqe.que & QUE_ALL))
        {
            status = ST_ERR;
            CHECK_ERROR_BIT(sqe.que, QUE_PS_OVER);
            CHECK_ERROR_BIT(sqe.que, QUE_PT_OVER);
            CHECK_ERROR_BIT(sqe.que, QUE_PS_TRACK_LOSS);
            CHECK_ERROR_BIT(sqe.que, QUE_PT_TRACK_LOSS);
            CHECK_ERROR_BIT(sqe.que, QUE_PS_COEFF_ERR);
            CHECK_ERROR_BIT(sqe.que, QUE_PT_COEFF_ERR);                                                                                                                                                                 
        } 
    }

    if(stbc.stb & STB_ESB)
    {        
        ESR esr = command_ESR();        
        if(!esr.succeed || (esr.esb & ESB_ERR))
        {
            status = ST_ERR;
            CHECK_ERROR_BIT(esr.esb, ESB_DDE);
            CHECK_ERROR_BIT(esr.esb, ESB_EXE);
            CHECK_ERROR_BIT(esr.esb, ESB_CME);            
        }
    }

    if(stbc.stb & STB_OPR)
    {        
        StatOperEven soe = command_StatOperEven();         
        if(!soe.succeed) 
        {
            status = ST_ERR;
            printf("OPR ERROR\n");            
        } 
        else if(soe.opr & OPR_ALL)
        {
            CHECK_OPR_BIT(soe.opr, OPR_STABLE);
            CHECK_OPR_BIT(soe.opr, OPR_RAMPING);
            CHECK_OPR_BIT(soe.opr, OPR_LEAKT_STABLE);
            CHECK_OPR_BIT(soe.opr, OPR_VOLUMET_DONE);
            CHECK_OPR_BIT(soe.opr, OPR_PS_STABLE);
            CHECK_OPR_BIT(soe.opr, OPR_PS_RAMPING);
            CHECK_OPR_BIT(soe.opr, OPR_PT_STABLE);
            CHECK_OPR_BIT(soe.opr, OPR_PT_RAMPING);
            CHECK_OPR_BIT(soe.opr, OPR_SELFT_DONE);
            CHECK_OPR_BIT(soe.opr, OPR_GTG);
        }              
    }

    return status;    
}       

 