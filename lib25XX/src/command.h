#pragma once
#include <stdbool.h>

//returned bytes
typedef enum {
    STB_BIT_0 = 1 << 0,
    STB_BIT_1 = 1 << 1,
    STB_BIT_2 = 1 << 2,
    STB_QUE   = 1 << 3,
    STB_BIT_4 = 1 << 4,
    STB_ESB   = 1 << 5,
    STB_BIT_6 = 1 << 6,
    STB_OPR   = 1 << 7,
    STB_ALL = (STB_QUE | STB_ESB | STB_OPR)
} STB;

typedef enum {
    QUE_BIT_0         = 1 << 0,
    QUE_BIT_1         = 1 << 1,
    QUE_BIT_2         = 1 << 2,
    QUE_BIT_3         = 1 << 3,
    QUE_BIT_4         = 1 << 4,
    QUE_BIT_5         = 1 << 5,
    QUE_BIT_6         = 1 << 6,
    QUE_BIT_7         = 1 << 7,
    QUE_BIT_8         = 1 << 8,
    QUE_PS_OVER       = 1 << 9,
    QUE_PT_OVER       = 1 << 10,
    QUE_PS_TRACK_LOSS = 1 << 11,
    QUE_PT_TRACK_LOSS = 1 << 12,
    QUE_PS_COEFF_ERR  = 1 << 13,
    QUE_PT_COEFF_ERR  = 1 << 14,
    QUE_BIT_15        = 1 << 15, 
    QUE_ALL           = (QUE_PS_OVER | QUE_PT_OVER | QUE_PS_TRACK_LOSS | QUE_PT_TRACK_LOSS | QUE_PS_COEFF_ERR | QUE_PT_COEFF_ERR)
} QUE;

typedef enum {
    ESB_OPC           = 1 << 0,
    ESB_RQC           = 1 << 1,
    ESB_QYE           = 1 << 2,
    ESB_DDE           = 1 << 3,
    ESB_EXE           = 1 << 4,
    ESB_CME           = 1 << 5,
    ESB_URQ           = 1 << 6,
    ESB_PON           = 1 << 7,
    ESB_ERR           = (ESB_DDE | ESB_EXE | ESB_CME)
} ESB;

typedef enum {
    OPR_BIT_0         = 1 << 0,
    OPR_STABLE        = 1 << 1,
    OPR_BIT_2         = 1 << 2,
    OPR_RAMPING       = 1 << 3,
    OPR_LEAKT_STABLE  = 1 << 4,
    OPR_VOLUMET_DONE  = 1 << 5,
    OPR_BIT_6         = 1 << 6,
    OPR_BIT_7         = 1 << 7,
    OPR_PS_STABLE     = 1 << 8,
    OPR_PS_RAMPING    = 1 << 9,
    OPR_PT_STABLE     = 1 << 10,
    OPR_PT_RAMPING    = 1 << 11,
    OPR_SELFT_DONE    = 1 << 12,
    OPR_GTG           = 1 << 13,
    OPR_BIT_14        = 1 << 14,
    OPR_BIT_15        = 1 << 15,
    OPR_ALL           = (OPR_STABLE | OPR_RAMPING | OPR_LEAKT_STABLE | OPR_VOLUMET_DONE | OPR_PS_STABLE | OPR_PS_RAMPING | OPR_PT_STABLE | OPR_PT_RAMPING | OPR_SELFT_DONE | OPR_GTG) 
} OPR;

//Interface/Abstract Class notes - below some interfaces/abstract classes are declared
//A macros are provided to IMPLEMENT them as an unnamed (anon) struct 
//The macros would be unneeded and could be cleaner if we enabled -fms-extensions
//but it is not standard c. References:
//https://gcc.gnu.org/onlinedocs/gcc/Unnamed-Fields.html
//https://modelingwithdata.org/arch/00000113.htm
//For convenience of constructing them at runtime, construct functions are provided

typedef enum {
    EXP_TYPE_STR      = 1 << 0,
    EXP_TYPE_FP       = 1 << 1,
} EXP_TYPE;

//Declare SetCommand abstract type
#define SetCommand_MEMBERS \
    EXP_TYPE   type; \
    char cmd[32]; \
    const char *cmd_verify;
typedef struct SetCommand {
    SetCommand_MEMBERS;
} SetCommand;  
#define IMPLEMENT_SetCommand struct { \
    SetCommand_MEMBERS; \
};

typedef struct SetCommandExpectString {
    IMPLEMENT_SetCommand;
    const char *expected;
} SetCommandExpectString;
SetCommandExpectString *SetCommandExpectString_construct(SetCommandExpectString *instance, const char *cmd, const char *cmd_data, const char *cmd_verify);

typedef struct SetCommandExpectFP {
    IMPLEMENT_SetCommand;
    double     expected;
} SetCommandExpectFP;
SetCommandExpectFP *SetCommandExpectFP_construct(SetCommandExpectFP *instance, const char *cmd, const char *cmd_data, const char *cmd_verify);

//Add your SetCommands here, so enough storage is created to store your SetCommand
//This contains an implementation of a SetCommand
typedef union SetCommandFull {
    SetCommandExpectString sces;
    SetCommandExpectFP scefp;
} SetCommandFull;
SetCommandFull *SetCommandFull_construct(SetCommandFull *instance, const EXP_TYPE type, const char *cmd, const char *cmd_data, const char *cmd_verify);

//Declare an interface for command results
#define ICommandResult_MEMBERS \
    int result; \
    int succeed;
typedef struct ICommandResult {
    ICommandResult_MEMBERS;
} ICommandResult;
#define IMPLEMENT_ICommandResult struct { \
    ICommandResult_MEMBERS; \
};

typedef struct StatQuesEven {
    union
    {
        IMPLEMENT_ICommandResult;
        QUE      que;
    };    
} StatQuesEven;

typedef struct StatOperEven {
    union
    {
        IMPLEMENT_ICommandResult;
        OPR      opr;
    };
} StatOperEven;

typedef struct STBCommand {
    union
    {
        IMPLEMENT_ICommandResult;
        STB      stb;
    };

} STBCommand;

typedef struct ESR {
    union
    {
       IMPLEMENT_ICommandResult;
       ESB      esb;
    };

} ESR;


StatOperEven command_StatOperEven();
StatQuesEven command_StatQuesEven();
ESR command_ESR();
STBCommand command_STB();

bool command_and_check_result_str(const char *msg, const char *expected_result);
bool command_and_check_result_float(const char *msg, double expected_result);

bool command_check_and_handle_ERROR();

bool command_GTG_eventually();