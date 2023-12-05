
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef OWCONNECTOR_H_
#define OWCONNECTOR_H_

#include "app_commands.h"
#include "system_state.h"

#define MAX_STR_LEN 1024

#define UART_CMD "cmd"
#define UART_ACK "ack"
#define UART_STATUS "status"
#define MAINTENANCE_SAVE_TO_USB "maintenanceSaveToUSB"

#define HOME_START_PATIENT_ID_ENTERED "homeStartPatientIdEntered"

#define PLACEMENT_SCREEN "placementScreen"
#define TEST_CONTACT "testContact"
#define START_SCAN "startScan"

#define SCAN_FINISHED_SAVE_NOTE "scanfinishedSaveNote"

extern const char* commandMessage[384];

struct CmdMessage {
    char command[24];
    char params[256];
};

struct RespMessage {
    char content[16];
    char state[16];
    char command[24];
    char params[256];
    char status[64];
    char msg[256];
    char data[16384];
};


int openSerialPort(const char *path);
int closeSerialPort(int portHandle);
int setInterfaceAttribs(int fd, int speed, int parity, int waitTime);
int sendResponseMessage(int fd, ow_app_content_t content, ow_system_state_t state, ow_app_commands_t cmd, const char *params, const char *msg, const char *data);
int readData(int fd, unsigned char *buffer, int buf_len);
int sendResponseMessage2(int fd, const char *msg);
struct CmdMessage* parseJson(const char* jsonString);
struct CmdMessage* getNextCommand(int fd);

#endif // OWCONNECTOR_H_