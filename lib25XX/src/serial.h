#pragma once
#include <stdint.h>


typedef enum SCPIType {
    SCPIType_ADTS = 1 << 0,
    SCPIType_LSU  = 1 << 1
}SCPIType;

#define _SCPIDevice struct { \
    SCPIType type; \
    int fd; \
} 

typedef _SCPIDevice SCPIDevice;

typedef struct ADTS {
    _SCPIDevice;
} ADTS;

typedef struct SCPIDeviceManager
{
    ADTS master;
    ADTS slave;
    SCPIDevice lsu;
}SCPIDeviceManager;

SCPIDeviceManager *serial_get_SDM();

bool serial_init(SCPIDeviceManager *sdm, const char *master_sn, const char *slave_sn);

bool serial_fd_do(int fd, const char *cmd, void *result, size_t result_size, int *num_result_read);
bool serial_do(const char *cmd, void *result, size_t result_size, int *num_result_read);
bool serial_integer_cmd(const char *cmd, int *result);
void serial_close(SCPIDeviceManager *sdm);