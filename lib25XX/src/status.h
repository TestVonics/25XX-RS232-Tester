#pragma once

bool status_is_idle();

typedef enum {
    ST_IDLE_VENT = 1 << 0,
    ST_ERR       = 1 << 1,
    ST_NOT_IDLE  = 1 << 2
} STATUS;

STATUS status_check_event_registers();