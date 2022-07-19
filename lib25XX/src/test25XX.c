#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include "test25XX.h"
#include "serial.h"
#include "utility.h"
#include "test.h"

typedef struct SCPIDeviceManager SCPIDeviceManager;

static SCPIDeviceManager SDM;
static yes_or_no_func Yes_No;
static tc_choice Choice;

static inline bool build_filename_from_sn(char *filename, const char *sn)
{
    time_t now = time(NULL);
    struct tm *timenow;
    timenow = localtime(&now);
    
    char filetemp[256];
    strftime(filetemp, sizeof(filetemp), "_%Y-%m-%d_%H:%M:%S.log", timenow);
    snprintf(filename, 256, "SN_%s%s", sn, filetemp);
    DEBUG_PRINT("log filename: %s", filename);
    return true;
}

bool test25XX_init(get_buf_func master_sn, get_buf_func slave_sn, get_buf_func ask_name, yes_or_no_func yes_no, tc_choice choice)
{
    const char *name = ask_name();
    const char *master = master_sn();
    const char *slave = slave_sn();
    //make the main log file named after the master sn
    char filename[256];
    if(!build_filename_from_sn(filename, master))
        return false;
    if(!log_init(filename))
        return false; 

    OUTPUT_PRINT("ADC|ADTS|LSU RS232 Tester V0.1");
    OUTPUT_PRINT("Tester name: %s", name);
    OUTPUT_PRINT("Master unit S/N: %s", master);     
    OUTPUT_PRINT("Slave unit S/N: %s", slave);

    //Initialize serial   
    if(!serial_init(&SDM, master, slave))
        return false;

    Yes_No = yes_no;
    Choice = choice;
    return true;
}

void test25XX_run_tests(void)
{
    UserFunc user_func = {Choice, Yes_No};
    test_run_all(&user_func);
}

void test25XX_close(void)
{
    serial_close(&SDM);    
    log_close();
}