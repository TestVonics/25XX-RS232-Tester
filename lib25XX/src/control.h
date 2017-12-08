#pragma once
//Controlling
#include <stdint.h>
typedef enum {
    CTRL_UNITS_FK   = 1 << 0,
    CTRL_UNITS_INHG = 1 << 1,
} CTRL_UNITS;

struct ControlTest;
bool control_run_test(const struct ControlTest *test);


typedef enum {
    CTRL_OP_PS        = 1 << 0,
    CTRL_OP_PT        = 1 << 1,
    CTRL_OP_DUAL      = (CTRL_OP_PS | CTRL_OP_PT)
} CTRL_OP;

typedef bool (*Control_On_Error)(void);
typedef bool (*Control_Start_Func)(void);
bool control(const uint64_t exp_time, const char *ps_units, const char *pt_units, OPR success_mask, Control_Start_Func start_func, Control_On_Error on_error);