#pragma once
#include <stdbool.h>

typedef void(*wait_func)(void);

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









