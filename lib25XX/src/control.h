#pragma once
//Controlling
#include <stdint.h>

bool control_dual_FK(const char *ps, const char *ps_rate, const char *pt, const char *pt_rate, uint64_t exp_time);