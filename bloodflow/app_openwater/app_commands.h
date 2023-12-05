#ifndef _APP_COMMANDS_H
#define _APP_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CMD_OPEN_PATIENT,
    CMD_SCAN_PATIENT,
    CMD_GET_PATIENT_DATA,
    CMD_SAVE_PATIENT_NOTE,
    CMD_CLOSE_PATIENT,
    CMD_SYSTEM_STATE,
    CMD_SYSTEM_VERSION,
    CMD_SYSTEM_SHUTDOWN,
    CMD_SYSTEM_BACKUP,
    CMD_SYSTEM_BACKUP_LOGS,
    CMD_SYSTEM_UPDATE,
    CMD_SYSTEM_SET_CAMERA,
    CMD_SYSTEM_CLEANUP,
    CMD_SYSTEM_RESET,
    CMD_SYSTEM_TIME,
    CMD_PERSONALITY_BOARD_TEMP,
    CMD_MAX
} ow_app_commands_t;

static const char* const OW_APP_COMMAND_STRING[CMD_MAX] =
{
    "open_patient",
    "scan_patient",
    "get_patient_graph",
    "save_patient_note",
    "close_patient",
    "sys_state",
    "sys_version",
    "sys_shutdown",
    "sys_backup",
    "sys_backup_logs",
    "sys_update",
    "sys_set_camera",
    "sys_cleanup",
    "sys_reset",
    "sys_time",
    "sys_pers_board_temp"
};

typedef enum
{
    OW_CONTENT_INFO,
    OW_CONTENT_DATA,
    OW_CONTENT_STATE,
    OW_CONTENT_RESPONSE,
    OW_CONTENT_ERROR,
    OW_CONTENT_MAX
} ow_app_content_t;

static const char* const OW_APP_CONTENT_STRING[OW_CONTENT_MAX] =
{
    "info",
    "data",
    "set_state",
    "response",
    "error"
};

typedef enum
{
    OW_STATUS_SUCCESS,
    OW_STATUS_ERROR,
    OW_STATUS_MAX
} ow_app_status_t;

static const char* const OW_APP_STATUS_STRING[OW_STATUS_MAX] =
{
    "success",
    "error"
};

#ifdef __cplusplus
}
#endif

#endif // _APP_COMMANDS_H