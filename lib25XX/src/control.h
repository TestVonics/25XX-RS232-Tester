#pragma once
//Controlling
#include <stdint.h>
typedef enum {
    CTRL_UNITS_FK   = 1 << 0,
    CTRL_UNITS_INHG = 1 << 1,
} CTRL_UNITS;

struct ControlTest;
bool control_run_test(const struct ControlTest *test);
