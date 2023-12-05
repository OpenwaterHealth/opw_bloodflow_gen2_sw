#define MAX_DEV_NAME_LEN 255
#define LASER_SAFETY_SETPOINTS 4

typedef struct
{
    char i2c_device[MAX_DEV_NAME_LEN];
    char gpio_device[MAX_DEV_NAME_LEN];
    uint16_t seed_warmup_time_seconds;
    uint16_t seed_preset;
    uint16_t ta_preset;
    uint16_t laser_safety_setpoints[LASER_SAFETY_SETPOINTS];
    uint16_t adc_ref_volt_mv;
    uint16_t dac_ref_volt_mv;
    uint8_t fiber_switch_channel_left;
    uint8_t fiber_switch_channel_right;
} lsw_laser_pdu_config_t;

// Call startup() during system initialization to configure and validate
// subsystem components.
bool lsw_laser_pdu_startup(lsw_laser_pdu_config_t *p_lsw_laser_pdu_config);

uint8_t lsw_remaining_startup_seconds();

// Call shutdown() during system shutdown to cleanly disable subsystem
// components before powering down.
bool lsw_laser_pdu_shutdown();

// Set the laser switch to the specified channel.
bool lsw_set_channel(uint8_t channel);

bool ta_enable(bool enable);
bool lsw_headset_poweron();
bool lsw_headset_poweroff();
