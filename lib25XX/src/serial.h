#pragma once

bool serial_init();
int serial_try_read(char *buf, size_t bufsize);
void serial_write(const char *str);
void serial_close();