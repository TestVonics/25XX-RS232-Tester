#pragma once
#include <stdbool.h>
//#include <stdint.h>
#include "command.h"
#include "control.h"

typedef void(*wait_func)(void);
typedef bool(*yes_no_func)(void);
typedef double(*double_func)(void);

typedef enum {
    IN_DATA_CTRL_ADTS_SN = 1 << 0,
    IN_DATA_MEAS_ADTS_SN = 1 << 1,
} IN_DATA_ID;

typedef int(*get_data_func)(const IN_DATA_ID data_id, char *buf, const size_t bufsize);

void test_run_all(wait_func waitfunc);

#define _TEST struct { \
    const char *test_name; \
    const char *setup; \
    const char *user_task; \
}

typedef struct ControlTest {
    _TEST;
    const CTRL_UNITS units;
    const uint64_t   duration;
    const char *ps;
    const char *ps_rate;
    const char *pt;
    const char *pt_rate;
} ControlTest;

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

typedef bool (*yes_or_no_func)();
typedef struct LSUValveTest {
    _TEST;
    const char *valve_number;
    //const yes_or_no_func yes_no;
} LSUValveTest;

