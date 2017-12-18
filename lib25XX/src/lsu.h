#pragma once
#include "test.h"

typedef struct LSUValveTest {
    _TEST;
    const char *valve_number;    
} LSUValveTest;
bool lsu_valve_test(const LSUValveTest *lsu_valve_test);