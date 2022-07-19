#pragma once

typedef enum {
    TC_UNKNOWN = 0,
    TC_RUN = 1 << 0,
    TC_PREV = 1 << 1,
    TC_SKIP = 1 << 2
} TEST_CHOICE;

typedef TEST_CHOICE (*tc_choice)(void);
typedef const char *(*get_buf_func)(void);
typedef bool (*yes_or_no_func)();

bool test25XX_init(get_buf_func master_sn, get_buf_func slave_sn, get_buf_func ask_name, yes_or_no_func yes_no, tc_choice choise);
void test25XX_close(void);
void test25XX_run_tests(void);