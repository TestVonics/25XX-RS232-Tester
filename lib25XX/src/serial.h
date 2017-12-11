#pragma once
#include <stdint.h>

typedef struct
{
    int fd;


} SCPIDevice;

typedef struct SCPIDeviceManager
{
    SCPIDevice master;
    SCPIDevice slave;
    SCPIDevice lsu;
}SCPIDeviceManager;

bool serial_init(SCPIDeviceManager *sdm, const char *master_sn, const char *slave_sn);

bool serial_fd_do(int fd, const char *cmd, void *result, size_t result_size, int *num_result_read);
bool serial_do(const char *cmd, void *result, size_t result_size, int *num_result_read);
bool serial_integer_cmd(const char *cmd, int *result);
void serial_close(SCPIDeviceManager *sdm);