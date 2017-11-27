#pragma once
typedef enum {
    STB_BIT_0 = 1 << 0,
    STB_BIT_1 = 1 << 1,
    STB_BIT_2 = 1 << 2,
    STB_QUE   = 1 << 3,
    STB_BIT_4 = 1 << 4,
    STB_ESB   = 1 << 5,
    STB_BIT_6 = 1 << 6,
    STB_OPR   = 1 << 7
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


bool status_is_idle();

bool command_try_expect(const char *msg, const char *expected_result);
bool command_try_expect_float(const char *msg, double expected_result);


typedef struct IBaseCommand {
    int result;
    bool succeed;
} IBaseCommand;
//declare as unnamed struct so we can access the members of IBaseCommand directly in structs that implement it
#define IBASECOMMAND struct { \
int result; \
bool succeed; \
};


/* Command StatQuesEven implements IBaseCommand */
typedef struct StatQuesEven {
    union
    {
        IBASECOMMAND;
        QUE      que;
    };    
} StatQuesEven;

/* Command StatOperEven implements IBaseCommand */
typedef struct StatOperEven {
    union
    {
        IBASECOMMAND;
        OPR      opr;
    };
} StatOperEven;

typedef struct STBCommand {
    union
    {
        IBASECOMMAND;
        STB      stb;
    };

} STBCommand;

typedef struct ESR {
    union
    {
       IBASECOMMAND;
       ESB      esb;
    };

} ESR;

StatOperEven command_StatOperEven();