#pragma once
//Controlling
#include <stdint.h>
#include "test.h"

typedef enum {
    CTRL_UNITS_FK   = 1 << 0,
    CTRL_UNITS_INHG = 1 << 1,
} CTRL_UNITS;

typedef enum {
    CTRL_OP_PS        = 1 << 0,
    CTRL_OP_PT        = 1 << 1,
    CTRL_OP_DUAL      = (CTRL_OP_PS | CTRL_OP_PT)
} CTRL_OP;

#define _ControlTest struct { \
    _TEST; \
    const CTRL_UNITS units; \
    const uint64_t   duration; \
    const char *ps; \
    const char *ps_rate; \
    const char *pt; \
    const char *pt_rate; \
}
typedef _ControlTest ControlTest;


typedef struct LeakTest {
    _ControlTest;
    const double ps_tolerance;
    const double pt_tolerance;
    const char *delay_minutes;
    const char *delay_seconds;
    bool testing_master_unit;
} LeakTest;


typedef struct SingleChannelTest {
    _TEST;
    const CTRL_UNITS units;
    const uint64_t duration;
    CTRL_OP op;
    union {
        const char *ps;
        const char *pt;
    };
    union {
        const char *ps_rate;
        const char *pt_rate;
    };    
} SingleChannelTest;


typedef bool (*Control_On_Error)(const int fd);
typedef bool (*Control_Start_Func)(const int fd);
typedef bool (*Control_EachCycle)(bool *result);
