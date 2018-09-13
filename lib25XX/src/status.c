#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "status.h"
#include "serial.h"
#include "command.h"
#include "utility.h"

typedef struct LastStatus {
    STB stb;
    QUE que;
    ESB esb;
    OPR opr;
} LastStatus;

// to do, list form for these macros?
#define CHECK_ERROR_BIT(INT, COND) \
do { if(INT & COND) \
         ERROR_PRINT("Error: " #COND); } \
while(0)           

#define CHECK_OPR_BIT(INT, COND) \
do { if(INT & COND) \
         OUTPUT_PRINT("OPR: " #COND); } \
while(0)
STATUS status_check_event_registers(const OPR opr_goal, const int adts_fd)
{
    static LastStatus lastStatus;
    STBCommand stbc = command_STB(adts_fd);
    STATUS status = ST_AT_GOAL;
    if(!stbc.succeed)
        return ST_ERR;

    //if STB isn't 0 we aren't idle in most cases
    //if(stbc.stb & STB_ALL)
    if(stbc.stb != 0)
    {
        status = ST_NOT_IDLE;
        if(stbc.stb != lastStatus.stb)
            OUTPUT_PRINT("________________________________________________");
    }

    StatQuesEven sqe = {{{0}}};
    if(stbc.stb & STB_QUE)
    {        
        sqe = command_StatQuesEven(adts_fd);
        if(!sqe.succeed || (sqe.que & QUE_ALL))
        {
            status = ST_ERR;
            if(sqe.que != lastStatus.que)
            {
                CHECK_ERROR_BIT(sqe.que, QUE_PS_OVER);
                CHECK_ERROR_BIT(sqe.que, QUE_PT_OVER);
                CHECK_ERROR_BIT(sqe.que, QUE_PS_TRACK_LOSS);
                CHECK_ERROR_BIT(sqe.que, QUE_PT_TRACK_LOSS);
                CHECK_ERROR_BIT(sqe.que, QUE_PS_COEFF_ERR);
                CHECK_ERROR_BIT(sqe.que, QUE_PT_COEFF_ERR); 
            }                                                                                                                                                                
        } 
    }

    ESR esr = {{{0}}};
    if(stbc.stb & STB_ESB)
    {        
        esr = command_ESR(adts_fd);        
        if(!esr.succeed || (esr.esb & ESB_ERR))
        {
            status = ST_ERR;
            //if(esr.esb != lastStatus.esb)
            {
                CHECK_ERROR_BIT(esr.esb, ESB_DDE);
                CHECK_ERROR_BIT(esr.esb, ESB_EXE);
                CHECK_ERROR_BIT(esr.esb, ESB_CME); 
                serial_fd_do(adts_fd, ":SYST:ERR?", NULL, 0, NULL);
            }            
        }
    }

    StatOperEven soe = {{{0}}};
    if(stbc.stb & STB_OPR)
    {        
        soe = command_StatOperEven(adts_fd);         
        if(!soe.succeed) 
        {
            status = ST_ERR;
            ERROR_PRINT("OPR READ ERROR\n");            
        } 
        else if(soe.opr & OPR_ALL)
        {
            if(soe.opr != lastStatus.opr)
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

            //if only OPR is set in STB and OPR only reports that it hit ground
            //we also qualify as idle
            if(soe.opr & opr_goal)
            {               
                status = ST_AT_GOAL;
            }
        }              
    }
    //printf("Exiting %s, status is %d\n", __func__, status);
    lastStatus.stb = stbc.stb;
    lastStatus.esb = esr.esb;
    lastStatus.opr = soe.opr;
    lastStatus.que = sqe.que;
    return status;    
}       


void status_dump_pressure_data_if_different(const char *ps_units, const char *pt_units, const int adts_fd)
{
    static char last_ps[12], last_pt[12];
    char ps[12], pt[12];

    char ps_cmd[16] = ":MEAS:PS? ";    
    strcpy(ps_cmd + 10, ps_units);   
    if(!serial_fd_do(adts_fd, ps_cmd, ps, LENGTH_2D(ps), NULL))
        return;

    char pt_cmd[16] = ":MEAS:PT? ";
    strcpy(pt_cmd + 10, pt_units);
    if(!serial_fd_do(adts_fd, pt_cmd, pt, LENGTH_2D(pt), NULL))
        return;    
    
    if((strcmp(ps, last_ps) != 0)|| (strcmp(pt, last_pt)!= 0))
        OUTPUT_PRINT("PS: %s %s PT: %s %s", ps, ps_units, pt, pt_units);

    memcpy(last_ps, ps, LENGTH_2D(last_ps));
    memcpy(last_pt, pt, LENGTH_2D(last_ps));
}

 
