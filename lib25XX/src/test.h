#pragma once

typedef void(*wait_func)(void);
typedef bool(*yes_no_func)(void);
typedef double(*double_func)(void);

void test_run_all(wait_func waitfun);