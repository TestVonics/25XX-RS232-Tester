#pragma once
#include <stdbool.h>
//#include <stdint.h>
#include "command.h"
#include "control.h"

typedef void(*wait_func)(void);
typedef bool(*yes_no_func)(void);
typedef double(*double_func)(void);

void test_run_all(wait_func waitfun);

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

bool control_run_test(const ControlTest *test);