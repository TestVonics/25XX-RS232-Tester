#pragma once
#include <stdbool.h>

typedef enum {
    TC_UNKNOWN = 0,
    TC_RUN = 1 << 0,
    TC_PREV = 1 << 1,
    TC_SKIP = 1 << 2
} TEST_CHOICE;

typedef TEST_CHOICE (*tc_choice)(void);
typedef bool (*yes_or_no_func)();

typedef struct UserFunc {
    const tc_choice tc;
    const yes_or_no_func yes_no;
} UserFunc;

typedef enum {
    IN_DATA_CTRL_ADTS_SN = 1 << 0,
    IN_DATA_MEAS_ADTS_SN = 1 << 1,
} IN_DATA_ID;

typedef int(*get_data_func)(const IN_DATA_ID data_id, char *buf, const size_t bufsize);

void test_run_all(UserFunc *user_func);

#define _TEST struct { \
    const char *test_name; \
    const char *setup; \
    const char *user_task; \
}









