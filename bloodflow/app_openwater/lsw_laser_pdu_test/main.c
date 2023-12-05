#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "../system_state.h"
#include "../lsw_laser_pdu.h"

#define SLEEP_LEN 1000000

bool enable_laser_safety_setpoints(bool enable);

int main()
{
    init_state();
    
    lsw_laser_pdu_config_t lsw_laser_pdu_config;
    strcpy(lsw_laser_pdu_config.i2c_device, "/dev/i2c-6");
    strcpy(lsw_laser_pdu_config.gpio_device, "/dev/gpiochip0");
    lsw_laser_pdu_config.seed_warmup_time_seconds = 3;
    lsw_laser_pdu_config.seed_preset = 315;
    lsw_laser_pdu_config.ta_preset = 1023;
    lsw_laser_pdu_config.laser_safety_setpoints[0] = 82;
    lsw_laser_pdu_config.laser_safety_setpoints[1] = 154;
    lsw_laser_pdu_config.laser_safety_setpoints[2] = 358;
    lsw_laser_pdu_config.laser_safety_setpoints[3] = 184;
    lsw_laser_pdu_config.adc_ref_volt_mv = 5000;
    lsw_laser_pdu_config.dac_ref_volt_mv = 5000;

    lsw_laser_pdu_startup(&lsw_laser_pdu_config);
    // simulate waiting for seed timer
    sleep(lsw_laser_pdu_config.seed_warmup_time_seconds + 1);
    if (!enable_laser_safety_setpoints(true))
    {
        perror("[OW_LSW_LASER_PDU_TEST] unable to enable laser_safety_setpoints");
    }

    if (!lsw_set_channel(2))
    {
        perror("[OW_LSW_LASER_PDU_TEST] unable to set lsw to position 2");
    }

    usleep(SLEEP_LEN);

    lsw_laser_pdu_shutdown();
}
