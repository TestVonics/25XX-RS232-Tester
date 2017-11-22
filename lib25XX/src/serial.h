#pragma once
#include <stdint.h>

bool serial_init();
void serial_write(const char *str);
void serial_close();
int serial_read_or_timeout(char *buf, size_t bufsize, uint64_t timeout);