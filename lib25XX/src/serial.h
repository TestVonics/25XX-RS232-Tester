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


#define SERIAL_DEVICE_USB        1 << 0
#define SERIAL_DEVICE_COM        1 << 1
#define SERIAL_DEVICE_ETHERNET   1 << 2  //NOT TESTED OR IMPLEMENTED ON LINUX YET

/* To CYGWIN, all difference modes of serial appear as a COM port /dev/S* */
#define SERIAL_MODE_COM SERIAL_DEVICE_COM
#ifdef __CYGWIN__
    #define SERIAL_MODE_USB      SERIAL_DEVICE_COM
    #define SERIAL_MODE_ETHERNET SERIAL_DEVICE_COM
    #define DELAY_BEFORE_SERIAL_READ 300
    #define IGNORE_1BYTE_READ    
#else
    #define SERIAL_MODE_USB SERIAL_DEVICE_USB
    #define SERIAL_MODE_ETHERNET SERIAL_DEVICE_ETHERNET    
#endif 



/* Set your desired serial device when compiling here */
#define SERIAL_MODE SERIAL_MODE_USB

