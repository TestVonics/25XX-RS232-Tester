#pragma once
#include <stdint.h>

bool serial_init();
void serial_close();
bool serial_do(const char *cmd, void *result, size_t result_size, int *num_result_read);
bool serial_integer_cmd(const char *cmd, int *result);

