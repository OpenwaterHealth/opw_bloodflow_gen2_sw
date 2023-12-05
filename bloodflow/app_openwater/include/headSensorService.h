/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HEADSENSOR_SERVICE_H
#define __HEADSENSOR_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_HEAD_SENSOR_SIZE 264

typedef enum
{
    HS_CMD_UPDATE_SENSOR_STATE = 0,
    HS_CMD_HISTO_DONE_STATE,
    HS_CMD_HISTO_LEFT_DONE_STATE,
    HS_CMD_HISTO_RIGHT_DONE_STATE,
    HS_CMD_FILE_DONE_STATE,
    HS_CMD_FILE_LEFT_DONE_STATE,
    HS_CMD_FILE_RIGHT_DONE_STATE,
    HS_CMD_STAT_MSG,
    HS_CMD_CAMERA_STATUS
}HEAD_SENSOR_COMMAND;

#pragma pack(1)
typedef struct HEAD_SENSOR_MESSAGE
{
    HEAD_SENSOR_COMMAND cmd;
    uint32_t state;   // 0 = success 1 = failure
    uint32_t instanceID; 
    char msg[256];
    uint32_t imageMean[6];
}typeHEAD_SENSOR_MESSAGE;

#define HEAD_SENSOR_SERVICE_NAME "com.ti.remote_service_headsensor"

#ifdef __cplusplus
}
#endif

#endif /* __HEADSENSOR_SERVICE_H */
