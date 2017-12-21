#pragma once

#include "command.h"

bool status_is_idle();

typedef enum {
    ST_AT_GOAL = 1 << 0,
    ST_ERR       = 1 << 1,
    ST_NOT_IDLE  = 1 << 2
} STATUS;

STATUS status_check_event_registers(const OPR opr_goal, const int adts_fd);

void status_dump_pressure_data_if_different(const char *ps_units, const char *pt_units, const int adts_fd);