/*
    Interface between TDA4 and DiCon "VX Stepper Motor Switch Module"

    HW:  I2C bus, labelled as "I2C3",  actual "/dev/i2c-6" channel, upto 400kbps
               slave device 7-bit address 0x73

    SW:  i2c-tools from github opensource base
             Open Water customized integration

    Document:  DiCon VX Stepper Motor Switch MOdule Operation Manual
                              I2C payload protocol in section 5.7

    Date:
    2021.09.30  leoh@openwater.cc
    2022.6.15   steve@openwater.cc

    How to build this separated system app on TDA4 Linux user space:
        - cd into lsw_laser_pdu_test directory
        - run make
        - run executable ./lsw_laser_pdu_test
*/

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/gpio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <syslog.h>

#include <utils/iss/include/app_iss.h>

#include "app_messages.h"
#include "uart.h"
#include "system_state.h"
#include "lsw_laser_pdu.h"

/*   Laser System has seed laser,  transmission amplifier
 */
typedef enum dac_cmd
{
    DAC_CMD_WR_DACREG, // these are actual codes per Table6-2
    DAC_CMD_WR_DACREG1,
    DAC_CMD_WR_RAM,
    DAC_CMD_WR_ALL,
    DAC_CMD_WR_VOLATILECFG_ONLY,
    DAC_CMD_R5,
    DAC_CMD_R6,
    DAC_CMD_R7,
    DAC_CMD_READ = 100, //  these are not actual codes
    DAC_CMD_GC_RESET,
    DAC_CMD_GC_WAKEUP
} dac_cmd_e;

typedef enum dac_cfg_power_down
{
    DAC_CFG_POWERON = 0,
    DAC_CFG_PD_1K,
    DAC_CFG_PD_100K,
    DAC_CFG_PD_500K,
    DAC_CFG_PD_TOTAL
} dac_pd_e;

typedef enum dac_cfg_ref
{
    DAC_CFG_VREF_UNBUFFERED_VDD,
    DAC_CFG_VREF_VDD,
    DAC_CFG_VREF_UNBUFFERED_REF,
    DAC_CFG_VREF_BUFFERED_REF
} dac_ref_e;

typedef enum dac_cfg_gain
{
    DAC_CFG_GAIN_1X,
    DAC_CFG_GAIN_2X
} dac_gain_e;

typedef enum dac_status_op
{
    DAC_STATUS_BUSY,
    DAC_STATUS_READY
} dac_status_op_e;

typedef enum dac_status_por
{
    DAC_STATUS_PWROFF,
    DAC_STATUS_PWRON
} dac_status_por_e;

/*  Laser Switch Interface at TDA4 SoC side is written in classic C99 on purpose, instead of modern c++,
    in order to support TI SDK and build configuraiton
*/
typedef enum lswcmds
{
    LSW_POLLING_STATUS = 0X30,
    LSW_GET_DEV_INFO,
    LSW_GET_FW_VER,
    LSW_GET_SN,
    LSW_GET_FW_PART_NO = 0X35,
    LSW_GET_HW_PART_NO,
    LSW_SET_I2C_ADDR,
    LSW_RESET,
    LSW_GET_DEV_DIMENSION = 0X70,
    LSW_SET_IN_OUT = 0X76,
    LSW_GET_OUT_FOR_IN,
    LSW_SET_OUT_FOR_CH1,
    LSW_GET_OUT_FOR_CH1
} lswcmds_e;

typedef struct dac_object
{
    uint8_t vol_gain : 1;
    uint8_t vol_pd : 2;
    uint8_t vol_ref : 2;
    uint8_t vol_r0 : 1;
    uint8_t vol_por : 1;
    uint8_t vol_rdy : 1;

    uint16_t vol_data; // only lsb 10 bits are valid

    uint8_t nv_gain : 1;
    uint8_t nv_pd : 2;
    uint8_t nv_ref : 2;
    uint8_t nv_r1 : 1;
    uint8_t nv_por : 1;
    uint8_t nv_rdy : 1;

    uint16_t nv_data; // only lsb 10 bits are valid
} dac_object_t;

#define I2C_ADDR_PDU_GPIO 0x27
#define I2C_ADDR_LSW 0X73
#define I2C_ADDR_BRIDGE 0X71

#define I2C_ADDR_MCP_47FVB 0x61
#define I2C_ADDR_MCP_4716 0x62
#define I2C_ADDR_SEED_CURRENT 0x48
#define I2C_ADDR_PCA9554 0x24

#define LSW_CMD_MAX_LENGTH 124

#define ADC_LENGTH (10) // bits
#define DAC_LENGTH (10) // bits

//#define OW_LASER_CHK_STEP (15)                                            // in seconds
//#define OW_GEN2_LASER_CHK_DURATION (60 * 60 * 24 * 7 / OW_LASER_CHK_STEP) // 7 days
#define LASER_SEED_CH (0X08)
#define LASER_TA_CH (0X04)
#define LASER_PCA9535_CH (0x80)
#define LASER_SEED_CURRENT_CH (0X40)
#define LASER_SAFETY_SETPOINT_CH (0x01)

#define TA_ENABLE_PIN (1)
#define ERROR_RESET_PIN (2)
#define Uxx_D_PIN (4)

#define FAN_PIN "312"
#define MAX_PATH_LENGTH 256

#define SEED_ENABLE_PIN_OFFSET 10
#define ESTOP_PIN_OFFSET 9

#define PRINT_BUF_SIZE 256

extern int fd_serial_port;
int fd_i2c = -1;
int fd_gpio = -1;
uint8_t current_dev; // valid address
uint8_t remaining_startup_seconds = 60;
bool estop_triggered = true;

pthread_t p_id_startup;
pthread_mutex_t p_mutex_i2c;

lsw_laser_pdu_config_t *gp_lsw_laser_pdu_config = NULL;

void i2c_error(const char *error_msg)
{
    pthread_mutex_unlock(&p_mutex_i2c);
    error_state(error_msg);
}

bool i2c_addr_select(uint8_t addr)
{
    if (fd_i2c < 0)
    {
        fd_i2c = open(gp_lsw_laser_pdu_config->i2c_device, O_RDWR);

        if (fd_i2c < 0)
        {
            error_state("[OW_LSW_LASER_PDU] unable to open this i2c bus\n");
            return false;
        }
        printf("[OW_LSW_LASER_PDU] the fd for i2c6 is %d\n", fd_i2c);
    }
    if (ioctl(fd_i2c, I2C_SLAVE, addr) < 0)
    {
        char errstr[128];
        snprintf(errstr, sizeof(errstr), "[OW_LSW_LASER_PDU] io ctl  generic i2c slave address 0x%02X failed", addr);
        error_state(errstr);
        return false;
    }
    else
    {
        printf("[OW_LSW_LASER_PDU] io ctl  generic i2c slave address 0x%02X ok\n", addr);
        current_dev = addr;
    }
    return true;
}

bool i2c_switch_channel(uint8_t channel)
{
    char print_buf[PRINT_BUF_SIZE];
    uint8_t data = 0;
    if (!i2c_addr_select(I2C_ADDR_BRIDGE))
    {
        i2c_error("[OW_LSW_LASER_PDU] Unable to select bridge address");
        return false;
    }
    int n = write(fd_i2c, &channel, 1);
    if (n == 1)
    {
        n = read(fd_i2c, &data, 1);
        printf("[OW_LSW_LASER_PDU] BRIDGE channel read %02x\n", data);
        return true;
    }

    sprintf(print_buf, "[OW_LSW_LASER_PDU] Unable to switch bridge to channel %d", channel);
    i2c_error(print_buf);
    return false;
}

bool pca9535_init(uint8_t channel, uint8_t address, uint8_t port0_config, uint8_t port1_config)
{
    uint8_t buf[4];
    int bytes;

    memset(buf, 0, sizeof(buf));

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9535_init");
        return false;
    }

    // Set port output values to zero except for port 0 outputs 0 and 5 (TDA4 and Touch Display)
    buf[0] = 0x02;
    buf[1] = 0x21;
    buf[2] = 0x00;
    bytes = write(fd_i2c, buf, 3);
    if (bytes != 3)
    {
        printf("[OW_LSW_LASER_PDU] pca9535_init wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9535_init write 3 bytes expected");
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9535_init");
        return false;
    }

    // Configures ports
    buf[0] = 0x06;
    buf[1] = port0_config;
    buf[2] = port1_config;
    bytes = write(fd_i2c, buf, 3);
    if (bytes != 3)
    {
        printf("[OW_LSW_LASER_PDU] pca9535_init wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9535_init write 3 port config bytes expected");
        return false;
    }

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    return true;
}

bool pca9535_set_pin(uint8_t channel, uint8_t address, uint8_t port, uint8_t pin, bool high)
{
    uint8_t buf[4];
    int bytes;

    memset(buf, 0, sizeof(buf));

    if (port > 1)
    {
        error_state("[OW_LSW_LASER_PDU] invalid port in pca9535_set_pin, must be 0 or 1");
        return false;
    }

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9535_set_pin");
        return false;
    }

    buf[0] = 0x02 + port;
    bytes = write(fd_i2c, buf, 1);
    if (bytes != 1)
    {
        printf("[OW_LSW_LASER_PDU] pca9535_set_pin wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9535_set_pin write 1 byte expected");
        return false;
    }

    bytes = read(fd_i2c, buf + 1, 1);
    if (bytes != 1)
    {
        printf("[OW_LSW_LASER_PDU] pca9535_set_pin read %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9535_set_pin read 1 byte expected");
        return false;
    }

    if (high)
    {
        buf[1] |= (1 << pin);
    }
    else
    {
        buf[1] &= (0xFF ^ (1 << pin));
    }

    bytes = write(fd_i2c, buf, 2);
    if (bytes != 2)
    {
        printf("[OW_LSW_LASER_PDU] pca9535_set_pin wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9535_set_pin write 2 bytes expected");
        return false;
    }

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    return true;
}

bool pca9554_init(uint8_t channel, uint8_t address, uint8_t config, uint8_t output_data)
{
    uint8_t buf[4];
    int bytes;

    memset(buf, 0, sizeof(buf));

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9554_init");
        return false;
    }

    // Set port initial values
    buf[0] = 0x01;
    buf[1] = output_data;
    bytes = write(fd_i2c, buf, 2);
    if (bytes != 2)
    {
        printf("[OW_LSW_LASER_PDU] pca9554_init wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9554_init write 2 bytes expected");
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9535_init");
        return false;
    }

    // Configures port
    buf[0] = 0x03;
    buf[1] = config;
    bytes = write(fd_i2c, buf, 2);
    if (bytes != 2)
    {
        printf("[OW_LSW_LASER_PDU] pca9554_init wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9554_init write 2 port config bytes expected");
        return false;
    }

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    return true;
}

bool pca9554_set_pin(uint8_t channel, uint8_t address, uint8_t pin, bool high)
{
    uint8_t buf[4];
    int bytes;

    memset(buf, 0, sizeof(buf));

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address in pca9554_set_pin");
        return false;
    }

    buf[0] = 0x01;
    bytes = write(fd_i2c, buf, 1);
    if (bytes != 1)
    {
        printf("[OW_LSW_LASER_PDU] pca9554_set_pin wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9554_set_pin write 1 byte expected");
        return false;
    }

    bytes = read(fd_i2c, buf + 1, 1);
    if (bytes != 1)
    {
        printf("[OW_LSW_LASER_PDU] pca9554_set_pin read %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9554_set_pin read 1 byte expected");
        return false;
    }

    if (high)
    {
        buf[1] |= pin;
    }
    else
    {
        buf[1] &= (0xFF ^ pin);
    }

    bytes = write(fd_i2c, buf, 2);
    if (bytes != 2)
    {
        printf("[OW_LSW_LASER_PDU] pca9554_set_pin wrote %d bytes\n", bytes);
        i2c_error("[OW_LSW_LASER_PDU] pca9554_set_pin write 2 bytes expected");
        return false;
    }

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    return true;
}

bool ta_enable(bool enable)
{
    return (pca9554_set_pin(LASER_TA_CH, I2C_ADDR_PCA9554, TA_ENABLE_PIN, enable));
}

bool init_fd_gpio()
{
    struct gpiohandle_request rq;

    fd_gpio = open(gp_lsw_laser_pdu_config->gpio_device, O_RDONLY);
    if (fd_gpio < 0)
    {
        error_state("[OW_LSW_LASER_PDU] unabled to open gpio");
        return false;
    }
    printf("[OW_LSW_LASER_PDU] the fd for gpio is %d\n", fd_gpio);

    // set estop pin to input
    rq.lineoffsets[0] = ESTOP_PIN_OFFSET;
    rq.flags = GPIOHANDLE_REQUEST_INPUT;
    rq.lines = 1;
    if (ioctl(fd_gpio, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0)
    {
        printf("[OW_LSW_LASER_PDU] Unable to line handle from ioctl : %s", strerror(errno));
        close(rq.fd);
        return false;
    }

    close(rq.fd);
    return true;
}

bool enable_seed(bool enable)
{
    struct gpiohandle_request rq;
    struct gpiohandle_data data;

    rq.lineoffsets[0] = SEED_ENABLE_PIN_OFFSET;
    rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
    rq.lines = 1;
    if (ioctl(fd_gpio, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0)
    {
        printf("[OW_LSW_LASER_PDU] Unable to line handle from ioctl : %s\n", strerror(errno));
        return false;
    }

    data.values[0] = (enable ? 1 : 0);
    if (ioctl(rq.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0)
    {
        printf("[OW_LSW_LASER_PDU] Unable to set line value using ioctl : %s\n", strerror(errno));
        close(rq.fd);
        return false;
    }
    close(rq.fd);
    return true;
}

void estop_poll()
{
    struct gpiohandle_request rq;
    struct gpiohandle_data data;

    rq.lineoffsets[0] = ESTOP_PIN_OFFSET;
    rq.flags = GPIOHANDLE_REQUEST_INPUT;
    rq.lines = 1;
    
    if (ioctl(fd_gpio, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0)
    {
        syslog(LOG_ERR, "[OW_LSW_LASER_PDU] Unable to get line handle from ioctl");
        error_state("[OW_LSW_LASER_PDU] Unable to get line handle from ioctl");
        return;
    }

    if (ioctl(rq.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0)
    {
        syslog(LOG_ERR, "[OW_LSW_LASER_PDU] Unable to get line value using ioctl");
        error_state("[OW_LSW_LASER_PDU] Unable to get line value using ioctl");
    }
    else
    {
        // printf("[OW_LSW_LASER_PDU] estop gpio: %d\n", data.values[0]);
        if (!estop_triggered && data.values[0] == 0)
        {
            printf("[OW_LSW_LASER_PDU] Estop triggered\n");
            estop_triggered = true;
            
            syslog(LOG_ERR, "[OW_LSW_LASER_PDU] Estop triggered");

            change_state(OW_STATE_ESTOP, OW_UNDEFINED);
            if(fd_serial_port > 0){
                sendResponseMessage(fd_serial_port, OW_CONTENT_STATE, current_state(), CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_ESTOP], NULL);
                usleep(250000);
                sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_ESTOP], NULL);     
            }
        }
        else if (estop_triggered && data.values[0] == 1)
        {
            printf("[OW_LSW_LASER_PDU] Estop reset\n");
            syslog(LOG_NOTICE, "[OW_LSW_LASER_PDU] Estop reset");
            estop_triggered = false;

            // TODO: what to do when estop is reset?
        }
    }
    close(rq.fd);
}

bool mcp47fvb_write(uint8_t *data, uint8_t data_len)
{
    char print_buf[PRINT_BUF_SIZE];
    uint8_t bytes;

    bytes = write(fd_i2c, data, data_len);
    printf("[OW_LSW_LASER_PDU] mcp47fvb wrote %d bytes\n", bytes);
    if (bytes != data_len)
    {
        sprintf(print_buf, "[OW_LSW_LASER_PDU] write %d bytes to mcp47fvb expected for command %d", data_len, data[0]);
        i2c_error(print_buf);
        return false;
    }
    return true;
}

bool enable_laser_safety_setpoints(bool enable)
{
    char print_buf[PRINT_BUF_SIZE];
    int print_buf_len = 0;
    uint8_t buf[4];
    int bytes;

    memset(buf, 0, sizeof(buf));

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(LASER_SAFETY_SETPOINT_CH))
    {
        return false;
    }

    if (!i2c_addr_select(I2C_ADDR_MCP_47FVB))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address I2C_ADDR_MCP_47FVB");
        return false;
    }

    // Set DAC reference voltages
    buf[0] = 0x08 << 3;
    buf[1] = 0xFF;
    buf[2] = 0xFF;
    if (!mcp47fvb_write(buf, 3))
    {
        return false;
    }

    // Set gain
    buf[0] = 0x0A << 3;
    buf[1] = 0xFF;
    buf[2] = 0x00;
    if (!mcp47fvb_write(buf, 3))
    {
        return false;
    }

    for (int i = 0; i < LASER_SAFETY_SETPOINTS; i++)
    {
        buf[0] = (i << 3);
        buf[1] = (enable ? gp_lsw_laser_pdu_config->laser_safety_setpoints[i] >> 8 : 0);
        buf[2] = (enable ? gp_lsw_laser_pdu_config->laser_safety_setpoints[i] & 0xFF : 0);
        if (!mcp47fvb_write(buf, 3))
        {
            return false;
        }
    }

    for (int i = 0; i < LASER_SAFETY_SETPOINTS; i++)
    {
        buf[0] = (i << 3) + 0x06;
        if (!mcp47fvb_write(buf, 1))
        {
            return false;
        }

        bytes = read(fd_i2c, buf, 2);
        printf("[OW_LSW_LASER_PDU] mcp47fvb read %d bytes\n", bytes);
        if (bytes != 2)
        {
            i2c_error("[OW_LSW_LASER_PDU] read 2 bytes expected");
            return false;
        }

        print_buf_len = sprintf(print_buf, "[OW_LSW_LASER_PDU] RAW read safety setpoint %d data= ", i);
        for (int k = 0; k < 2; k++)
        {
            print_buf_len += sprintf(print_buf + print_buf_len, " %02x", buf[k]);
        }
        printf("%s\n", print_buf);
    }

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    return true;
}

bool mcp4716_read(uint8_t channel, dac_object_t *result)
{
    char print_buf[PRINT_BUF_SIZE];
    int print_buf_len = 0;
    uint8_t buf[16];
    int bytes;

    memset(buf, 0, sizeof(buf));

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(I2C_ADDR_MCP_4716))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address I2C_ADDR_MCP_4716 in mcp4716_read");
        return false;
    }

    bytes = read(fd_i2c, buf, 12);

    pthread_mutex_unlock(&p_mutex_i2c);

    if (bytes < 12)
    {
        printf("[OW_LSW_LASER_PDU] mcp4716_read DAC-check read %d bytes, expected 12\n", bytes);
        return false;
    }

    result->vol_gain = buf[0] & 0x01;
    result->vol_pd = (buf[0] & 0x06) >> 1;
    result->vol_ref = (buf[0] & 0x18) >> 3;
    result->vol_por = (buf[0] & 0x40) >> 6;
    result->vol_rdy = (buf[0] & 0x80) >> 7;
    result->vol_data = buf[1];
    result->vol_data <<= 2;
    result->vol_data += (buf[2] >> 6);

    result->nv_gain = buf[3] & 0x01;
    result->nv_pd = (buf[3] & 0x06) >> 1;
    result->nv_ref = (buf[3] & 0x18) >> 3;
    result->nv_por = (buf[3] & 0x40) >> 6;
    result->nv_rdy = (buf[3] & 0x80) >> 7;
    result->nv_data = buf[4];
    result->nv_data <<= 2;
    result->nv_data += (buf[5] >> 6);

    print_buf_len = sprintf(print_buf, "[OW_LSW_LASER_PDU] mcp4716_read RAW read data= ");
    for (int k = 0; k < 12; k++)
    {
        print_buf_len += sprintf(print_buf + print_buf_len, " %02x", buf[k]);
    }
    printf("%s\n", print_buf);

    printf("[OW_LSW_LASER_PDU]   READ decoded buf  vola memo gain=%d pd=%d ref=%d por=%d rdy=%d data=%d %d mV\n", result->vol_gain,
           result->vol_pd, result->vol_ref, result->vol_por, result->vol_rdy,
           result->vol_data, result->vol_data * gp_lsw_laser_pdu_config->dac_ref_volt_mv / (1 << DAC_LENGTH));
    printf("[OW_LSW_LASER_PDU]   READ decoded buf  EEPROM NV gain=%d pd=%d ref=%d por=%d rdy=%d data=%d %d mV\n", result->nv_gain,
           result->nv_pd, result->nv_ref, result->nv_por, result->nv_rdy,
           result->nv_data, result->nv_data * gp_lsw_laser_pdu_config->dac_ref_volt_mv / (1 << DAC_LENGTH));

    return true;
}

bool mcp4716_run_cmd(uint8_t channel, dac_cmd_e cmd, dac_ref_e ref, dac_pd_e pd, dac_gain_e gain, uint16_t data)
{
    uint8_t buf[16];
    int bytes;
    uint8_t n;

    memset(buf, 0, sizeof(buf));

    switch (cmd)
    {
    case DAC_CMD_WR_DACREG:
        buf[0] = (DAC_CMD_WR_DACREG << 6) | (pd << 4) | (data >> 6);
        buf[1] = (data & 0x3F) << 2;
        printf("[OW_LSW_LASER_PDU]  DAC_CMD_WR_DACREG WR buf %02x  %02x \n", buf[0], buf[1]);
        n = 2;
        break;
    case DAC_CMD_WR_RAM:
    case DAC_CMD_WR_ALL:
        buf[0] = (cmd << 5) | (ref << 3) | (pd << 1) | gain;
        buf[1] = (data >> 2);
        buf[2] = (data & 0x03) << 6;
        printf("[OW_LSW_LASER_PDU]  %s WR buf %02x  %02x  %0x2\n", (cmd == DAC_CMD_WR_RAM ? "DAC_CMD_WR_RAM" : "DAC_CMD_WR_ALL"), buf[0], buf[1], buf[2]);
        n = 3;
        break;
    case DAC_CMD_WR_VOLATILECFG_ONLY:
        buf[0] = (DAC_CMD_WR_VOLATILECFG_ONLY << 5) | (ref << 3) | (pd << 1) | gain;
        printf("[OW_LSW_LASER_PDU]  DAC_CMD_WR_VOLATILECFG_ONLY WR buf %02x \n", buf[0]);
        n = 1;
        break;

    default:
        return false;
        break;
    }

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(I2C_ADDR_MCP_4716))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to select address I2C_ADDR_MCP_4716");
        return false;
    }

    bytes = write(fd_i2c, buf, n);

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    printf("[OW_LSW_LASER_PDU] wrote %d bytes ... executed cmd=%d  pd=%d\n", bytes, cmd, pd);
    if (bytes < 0)
    {
        error_state("[OW_LSW_LASER_PDU] mcp4716 write error");
        return false;
    }

    return true;
}

bool mcp4716_check_nv_state(dac_ref_e ref, dac_pd_e pd, dac_gain_e gain, uint16_t data, dac_object_t *state)
{
    return (state->nv_ref == ref && state->nv_pd == pd && state->nv_gain == gain && state->nv_data == data);
}

bool mcp4716_check_and_set(uint8_t channel, uint16_t data)
{
    dac_object_t dac_object;
    dac_cmd_e mcp4716_cmd = DAC_CMD_WR_RAM;

    if (!mcp4716_read(channel, &dac_object))
    {
        return false;
    }

    if (!mcp4716_check_nv_state(0, 0, 0, data, &dac_object))
    {
        printf("[OW_LSW_LASER_PDU] updating LASER_SEED_CH nv\n");
        mcp4716_cmd = DAC_CMD_WR_ALL;
    }

    return (mcp4716_run_cmd(channel, mcp4716_cmd, 0, 0, 0, data));
}

bool ads7828_read(uint8_t channel, uint8_t address, int *pResult)
{
    uint8_t buf[8];
    int n;

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_switch_channel(channel))
    {
        return false;
    }

    if (!i2c_addr_select(address))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to switch ads7828 address");
        return false;
    }

    // TODO - what is the right value here?
    buf[0] = 0x9C;
    n = write(fd_i2c, buf, 1);
    if (n != 1)
    {
        i2c_error("ads7828 write error");
        return false;
    }

    memset(buf, 0, sizeof(buf));
    n = read(fd_i2c, buf, 6);

    // Disable i2c bridge
    if (!i2c_switch_channel(0))
    {
        return false;
    }

    pthread_mutex_unlock(&p_mutex_i2c);

    *pResult = (((buf[0] << 8) + buf[1]) >> 2) * gp_lsw_laser_pdu_config->adc_ref_volt_mv / (1 << ADC_LENGTH);
    printf("[OW_LSW_LASER_PDU] periodic seed ADC read  %d bytes  %02x %02x %02x %02x %02x %02x ... %d mV\n", n,
           buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], *pResult);
    return (n > 0);
}

uint8_t lsw_get_tx_param_count(lswcmds_e cmd)
{
    switch (cmd)
    {
    case LSW_SET_I2C_ADDR:
    case LSW_GET_OUT_FOR_IN:
    case LSW_SET_OUT_FOR_CH1:
        return 1;
        break;

    case LSW_SET_IN_OUT:
        return 2;
        break;

    default:
        return 0;
    }
}

uint8_t lsw_get_rx_byte_count(lswcmds_e cmd)
{
    uint8_t len = 0; // expected number of bytes
    switch (cmd)
    { // no param needed for such cmd ... group 1
    case LSW_POLLING_STATUS:
        len = 2;
        break;
    case LSW_GET_DEV_INFO:
        len = 59;
        break; // to test print LSW  cmd=31 read 66 bytes 31 39 44 69 43 6F 6E 20 46 69 62 65 72 6F 70 74 69 63 73 20 49 6E 63 2C 56 58 20 31 78 4E 2C 46 57 20 39 37 33 37 38 20 52 65 76 2E 41 31 2C 31 34 41 30 4C 56 32 44 30 30 31 33 3A E2 FF FF FF FF FF
    case LSW_GET_FW_VER:
        len = 5;
        break;
    case LSW_GET_SN:
        len = 14;
        break; // to test print LSW  cmd=33 read 18 bytes 33 0C 31 34 41 30 4C 56 32 44 30 30 31 33 5E 09 FF FF
    case LSW_GET_FW_PART_NO:
        len = 9;
        break; // to test print LSW  cmd=35 read 11 bytes 35 07 39 37 33 37 38 41 31 41 38
    case LSW_GET_HW_PART_NO:
        len = 12;
        break; // to test print
    case LSW_SET_I2C_ADDR:
        len = 2;
        break; // LSW  cmd=37 read 2 bytes 37 00
    case LSW_RESET:
        len = 0;
        break;
    case LSW_GET_DEV_DIMENSION:
        len = 3;
        break;               // 0x70 ok
    case LSW_SET_IN_OUT:     // 0x76 quiet
    case LSW_GET_OUT_FOR_IN: // 0x77 ok
    case LSW_SET_OUT_FOR_CH1:
        len = 2;
        break; // 0x77  quiet
    case LSW_GET_OUT_FOR_CH1:
        len = 3; // 0x79 ok
    }
    len += 2; // plus 2 bytes for the CRC16
    return len;
}

unsigned short int CRC16(const unsigned char *nData,
                         int wLength // in bytes
                         )           //  from https://www.modbustools.com/modbus_crc16.html, only change types
{
    static const unsigned short int wCRCTable[] = {
        0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
        0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
        0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
        0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
        0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
        0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
        0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
        0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
        0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
        0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
        0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
        0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
        0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
        0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
        0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
        0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
        0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
        0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
        0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
        0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
        0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
        0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
        0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
        0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
        0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
        0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
        0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
        0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
        0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
        0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
        0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
        0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040}; // 256 word16

    unsigned char nTemp;
    unsigned short int wCRCWord = 0xFFFF;

    while (wLength--)
    {
        nTemp = *nData++ ^ wCRCWord; // byte array  ^ LSB byte
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    return wCRCWord;
}

// Stores response data in provided outbuffer
// Returns number of bytes stored in outbuffer, -1 on error
int lsw_api_run_cmd(lswcmds_e cmd, uint8_t param1, uint8_t param2, uint8_t *outbuffer, size_t outbuffer_size)
{
    uint8_t idx = 0;
    uint16_t crc16;
    uint8_t data[8];
    uint8_t tx_param_count = lsw_get_tx_param_count(cmd);
    uint8_t rx_byte_count = lsw_get_rx_byte_count(cmd);
    char print_buf[PRINT_BUF_SIZE];
    int print_buf_len = 0;

    printf("outbuffer_size = %d, rx_byte_count = %d\n", (int)outbuffer_size, rx_byte_count);

    // outbuffer_size needs an extra byte for including the i2c address in CRC calculation
    if (outbuffer_size + 1 < rx_byte_count)
    {
        error_state("[OW_LSW_LASER_PDU] outbuffer too small for given command");
        return -1;
    }

    pthread_mutex_lock(&p_mutex_i2c);

    if (!i2c_addr_select(I2C_ADDR_LSW))
    {
        i2c_error("[OW_LSW_LASER_PDU] unable to communicate with lsw\n");
        return -1;
    }

    data[idx++] = I2C_ADDR_LSW * 2;
    data[idx++] = cmd;
    if (tx_param_count > 0)
    {
        data[idx++] = param1;
        if (tx_param_count > 1)
        {
            data[idx++] = param2;
        }
    }

    crc16 = CRC16(data, idx);

    data[idx++] = (unsigned char)(crc16 >> 8);
    data[idx++] = (unsigned char)(crc16 & 0xFF);

    int bytes = write(fd_i2c, &data[1], idx - 1);

    print_buf_len = sprintf(print_buf, "[OW_LSW_LASER_PDU] LSW cmd=%02X write %d bytes addr=%02X  content=", cmd, idx - 1, data[0]);
    for (int i = 1; i < idx; i++)
    {
        print_buf_len += sprintf(print_buf + print_buf_len, " %02x", data[i]);
    }
    printf("%s\n", print_buf);

    printf("[OW_LSW_LASER_PDU]  wrote %d bytes ... done\n", bytes);
    if (bytes != (idx - 1))
    {
        i2c_error("[OW_LSW_LASER_PDU] write command bytes failed");
        return -1;
    }

//    usleep(50000);
    usleep(350000);     // this seems high, but necessary

    memset(outbuffer, 0, outbuffer_size);
    outbuffer[0] = I2C_ADDR_LSW * 2 + 1;
    bytes = read(fd_i2c, outbuffer + 1, rx_byte_count);

    pthread_mutex_unlock(&p_mutex_i2c);

    print_buf_len = sprintf(print_buf, "[OW_LSW_LASER_PDU] LSW cmd=%02X read %d bytes", outbuffer[1], bytes);
    bytes++;
    for (int i = 0; i < bytes; i++)
    {
        print_buf_len += sprintf(print_buf + print_buf_len, " %02x", outbuffer[i + 1]);
    }
    printf("%s\n", print_buf);

    if (bytes < 4 || cmd != outbuffer[1])
    {
        error_state("[OW_LSW_LASER_PDU] invalid response");
        return -1;
    }

    crc16 = CRC16(outbuffer, bytes - 2); // exclude the last 2  bytes of read-CRC16
    if ((crc16 & 0xFF) == outbuffer[bytes - 1] && ((crc16 & 0xFF00) >> 8) == outbuffer[bytes - 2])
    {
        printf("[OW_LSW_LASER_PDU] %02X:%04X ---read check ok ---  \n\n", cmd, crc16);
        return bytes;
    }

    printf("[OW_LSW_LASER_PDU] %02X:%04X ---read check failed ---target %04X  \n\n", cmd, crc16, *(unsigned short int *)&outbuffer[bytes - 2]);
    return -1;
}

bool lsw_set_channel(uint8_t channel)
{
    uint8_t data[8];
    return lsw_api_run_cmd(LSW_SET_OUT_FOR_CH1, channel, 0, data, sizeof(data)) > 0;
}

void enable_fan()
{
    char path[MAX_PATH_LENGTH];

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to open /sys/class/gpio/export"))
        {
            return;
        }
    }

    if (write(fd, FAN_PIN, strlen(FAN_PIN)) != strlen(FAN_PIN))
    {
        if (error_state("[OW_LSW_LASER_PDU] Error writing to /sys/class/gpio/export"))
        {
            close(fd);
            return;
        }
    }

    close(fd);

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", FAN_PIN);

    fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to open /sys/class/gpio/gpioXX/direction"))
        {
            return;
        }
    }

    if (write(fd, "out", 3) != 3)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to write to /sys/class/gpio/gpioXX/direction"))
        {
            close(fd);
            return;
        }
    }

    close(fd);

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", FAN_PIN);

    fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to open to /sys/class/gpio/gpioXX/value"))
        {
            return;
        }
    }

    if (write(fd, "1", 1) != 1)
    {
        error_state("[OW_LSW_LASER_PDU] Error writing to /sys/class/gpio/gpioXX/value");
    }

    close(fd);
}

void disable_fan()
{
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1)
    {
        error_state("[OW_LSW_LASER_PDU] Unable to open /sys/class/gpio/unexport");
        return;
    }

    if (write(fd, FAN_PIN, strlen(FAN_PIN)) != strlen(FAN_PIN))
    {
        error_state("[OW_LSW_LASER_PDU] Error writing to /sys/class/gpio/unexport");
    }

    close(fd);
}

// Handles system startup. Communicates with peripherals in sequence primarily through i2c.
// Runs in its own thread due to timed events.
void *thread_startup(void *param)
{
    int adc_result;
    //uint8_t data[LSW_CMD_MAX_LENGTH];

    // UNDONE: the additional 29 seconds is from the hard coded delays below
    //         should do something different
    remaining_startup_seconds = gp_lsw_laser_pdu_config->seed_warmup_time_seconds + 29;

    if (!init_fd_gpio())
    {
        if (error_state("[OW_LSW_LASER_PDU] unabled to open fd_gpio"))
        {
            return NULL;
        }
    }

    // Fan currently enabled in hardware
    // enable_fan();

    if (!pca9535_init(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0x00, 0xff))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable I2C_ADDR_PDU_GPIO"))
        {
            return NULL;
        }
    }

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 5, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 5 to high"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 2, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 2 to high"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 3, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 3 to high"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 4, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 4 to high"))
        {
            return NULL;
        }
    }

    // Sleep to give laser switch time to startup after enabling PDU
    // TODO: Length of delay should be added as a configuration parameter
    sleep(5);
    remaining_startup_seconds -= 5;

    /* Removing laser switch startup for simultaneous measurement
    if (lsw_api_run_cmd(LSW_GET_DEV_INFO, 0, 0, data, sizeof(data)) < 0)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to communicate with laser switch"))
        {
            return NULL;
        }
    }
    printf("[OW_LSW_LASER_PDU] lsw info: %s\n", data + 3);
    */

    sleep(2);
    remaining_startup_seconds -= 2;

    // Ensure that laser is in disbled state during startup
    if (appDisableLaser() != 0)
    {
        if (error_state("[OW_LSW_LASER_PDU] Unable to disable laser during startup"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!mcp4716_check_and_set(LASER_SEED_CH, gp_lsw_laser_pdu_config->seed_preset))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to initialize LASER_SEED_CH"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!mcp4716_check_and_set(LASER_TA_CH, gp_lsw_laser_pdu_config->ta_preset))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to initialize LASER_TA_CH"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 6, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 6 to high"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 7, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 7 to high"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!enable_laser_safety_setpoints(true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable laser_safety_setpoints"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9554_init(LASER_TA_CH, I2C_ADDR_PCA9554, 0xFF ^ (TA_ENABLE_PIN | ERROR_RESET_PIN | Uxx_D_PIN), (ERROR_RESET_PIN | Uxx_D_PIN)))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to initialize I2C_ADDR_PCA9554"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    // Important: this cannot be enabled within 10 seconds of pin 2 & 3 going high
    if (!enable_seed(true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable seed"))
        {
            return NULL;
        }
    }

    sleep(2);
    remaining_startup_seconds -= 2;

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 1, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 1 to high"))
        {
            return NULL;
        }
    }

    printf("[OW_LSW_LASER_PDU] Seed warmup timer start, %d seconds...\n", gp_lsw_laser_pdu_config->seed_warmup_time_seconds);
    remaining_startup_seconds = gp_lsw_laser_pdu_config->seed_warmup_time_seconds;
    while (remaining_startup_seconds-- > 0)
    {
        sleep(1);
    }

    if (!ads7828_read(LASER_SEED_CURRENT_CH, I2C_ADDR_SEED_CURRENT, &adc_result))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to read I2C_ADDR_SEED_CURRENT"))
        {
            return NULL;
        }
    }
    printf("[OW_LSW_LASER_PDU] Seed current: %d\n", adc_result);

    usleep(1000);

    if (!pca9554_set_pin(LASER_TA_CH, I2C_ADDR_PCA9554, ERROR_RESET_PIN, false))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to set ERROR_RESET_PIN low"))
        {
            return NULL;
        }
    }

    usleep(1000);

    if (!pca9554_set_pin(LASER_TA_CH, I2C_ADDR_PCA9554, ERROR_RESET_PIN, true))
    {
        if (error_state("[OW_LSW_LASER_PDU] unable to set ERROR_RESET_PIN high"))
        {
            return NULL;
        }
    }

    if (change_state(OW_STATE_IDLE, OW_STATE_STARTUP)) 
    {
        printf("[OW_LSW_LASER_PDU] lsw_laser_pdu startup complete\n");
    }
    else
    {
        error_state("[OW_LSW_LASER_PDU] unexpected state");
        return NULL;
    }

    // TODO: Polling the estop should be a different thread and not part of the startup or shutdown of the laser
    while (current_state() != OW_STATE_SHUTDOWN && current_state() != OW_STATE_ERROR)
    {
        usleep(500000);
        estop_poll();
    }

    // NOTE: Uncleare if fan should ever be disabled
    // disable_fan();

    return NULL;
}

bool lsw_headset_poweron()
{

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 6, true))
    {
        printf("[OW_LSW_LASER_PDU] failed to turn on headset power");
        syslog(LOG_ERR, "[OW_LSW_LASER_PDU::lsw_camera_poweron] failed to turn on camera power");
        return false;
    }

    return true;
}

bool lsw_headset_poweroff()
{    

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 6, false))
    {
        printf("[OW_LSW_LASER_PDU] failed to turn off headset power");
        syslog(LOG_ERR, "[OW_LSW_LASER_PDU::lsw_camera_poweroff] failed to turn off camera power");
        return false;
    }
    
    return true;
}

bool lsw_laser_pdu_startup(lsw_laser_pdu_config_t *p_lsw_laser_pdu_config)
{
    gp_lsw_laser_pdu_config = p_lsw_laser_pdu_config;
    pthread_mutex_init(&p_mutex_i2c, NULL);

    if (!gp_lsw_laser_pdu_config)
    {
        error_state("[OW_LSW_LASER_PDU] p_lsw_laser_pdu_config cannot be null");
        return false;
    }

    if (current_state() != OW_STATE_STARTUP)
    {
        error_state("[OW_LSW_LASER_PDU] startup must be called while in OW_STATE_STARTUP state");
        return false;
    }

    pthread_create(&p_id_startup, NULL, thread_startup, NULL);
    return true;
}

uint8_t lsw_remaining_startup_seconds()
{
    return remaining_startup_seconds;
}

bool lsw_laser_pdu_shutdown()
{
    if (appDisableLaser() != 0 || !ta_enable(false))
    {
        error_state("[OW_LSW_LASER_PDU] Unable to disable laser during shutdown");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 1, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 1 to low");
    }

    usleep(500000);

    if (!enable_seed(false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to disable seed");
    }

    usleep(500000);

    if (!enable_laser_safety_setpoints(false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to disable laser_safety_setpoints");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 7, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 7 to low");
    }

    usleep(500000);

    if (!mcp4716_run_cmd(LASER_TA_CH, DAC_CMD_WR_RAM, 0, 0, 0, 0))
    {
        error_state("[OW_LSW_LASER_PDU] unable to uninitialize LASER_TA_CH");
    }

    usleep(500000);

    if (!mcp4716_run_cmd(LASER_SEED_CH, DAC_CMD_WR_RAM, 0, 0, 0, 0))
    {
        error_state("[OW_LSW_LASER_PDU] unable to uninitialize LASER_SEED_CH");
    }

    usleep(500000);

    if (!lsw_set_channel(0))
    {
        error_state("[OW_LSW_LASER_PDU] unable to park lsw");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 4, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 4 to low");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 3, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 3 to low");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 2, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 2 to low");
    }

    usleep(500000);

    if (!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 6, false))
    {
        error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 6 to low");
    }

    usleep(500000);

/*
    if (!estop_triggered)
    {
        if(!pca9535_set_pin(LASER_PCA9535_CH, I2C_ADDR_PDU_GPIO, 0, 5, false))
        {
            error_state("[OW_LSW_LASER_PDU] unable to enable to set I2C_ADDR_PDU_GPIO, port 0, pin 5 to low");
        }
    }
*/
    pthread_mutex_destroy(&p_mutex_i2c);
    return true;
}
