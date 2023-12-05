// Adapted from http://gaiger-programming.blogspot.com/2015/03/simple-linux-serial-port-programming-in.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h> /*termio.h for serial IO api*/
#include <arpa/inet.h>

#include <pthread.h> /* POSIX Threads */

#include "app_common.h"
#include "jsmn.h"
#include "owconnector.h"

pthread_mutex_t sendMessageLock;

const char* commandMessage[384] = {0};
const char* responseMessage[17408] = {0};
struct sockaddr_in server_addr;

int openSerialPort(const char *path)
{
    syslog(LOG_NOTICE, "[OW_MAIN] Opened Socket port: %s",path);            
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        syslog(LOG_NOTICE, "[OW_MAIN] Socket connection failed");            
        return -1;
    }
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12325); // Server port
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_NOTICE, "[OW_MAIN] Server connection failed");            
        
        close(client_socket);
        return -1;
    }
    return client_socket;

}

int closeSerialPort(int portHandle)
{
    close(portHandle);
    return 0;
}

int sendResponseMessage2(int fd, const char *msg)
{

    int result = 0;
    size_t bytes_sent = 0;
    size_t msg_len = 0;
    size_t chunk_size = 8192; // send 8K bytes at a time

    if (fd < 0) {
        printf("[OW_MAIN] Error serial port is not valid\n");   
        return 0;
    }

    pthread_mutex_lock(&sendMessageLock);

    msg_len = strlen(msg);

    while (bytes_sent < msg_len) {
        size_t remaining_bytes = msg_len - bytes_sent;
        size_t n = remaining_bytes < chunk_size ? remaining_bytes : chunk_size;

        size_t bytes_written = send(fd, msg + bytes_sent, n,0);

        if (bytes_written <= 0) {
            result = -1;
            syslog(LOG_ERR, "[UART::sendResponseMessage] UART port write failed\n");
            break;
        }

        bytes_sent += bytes_written;
        tcdrain(fd);
    }

    pthread_mutex_unlock(&sendMessageLock);

    return result;
}

int sendResponseMessage(int fd, ow_app_content_t content, ow_system_state_t state, ow_app_commands_t cmd, const char *params, const char *msg, const char *data)
{
    // printf("enter sendMessage handle: %d msg: %s\n", fd, message);
    int result;
    size_t msg_len = 0;
    size_t bytes_sent = 0;

    if(fd<0){
        syslog(LOG_ERR, "[OW_MAIN] Error serial port is not valid\n");
        return 0;
    }

    pthread_mutex_lock(&sendMessageLock);
    result = 0;
    char writeBuff[17408] = {0};

    sprintf(writeBuff, "{\"content\":\"%s\",\"state\":\"%s\",\"command\":\"%s\",\"params\":\"%s\",\"msg\":\"%s\",\"data\":%s}\r\n",
        OW_APP_CONTENT_STRING[content], OW_SYSTEM_STATE_STRING[state], OW_APP_COMMAND_STRING[cmd], params != NULL?params:"", msg != NULL?msg:"", content == OW_CONTENT_DATA?data:"[]");

    msg_len = strlen(writeBuff);
    while (bytes_sent < msg_len) {
        size_t n = send(fd, writeBuff, strlen(writeBuff),0);
        if (n <= 0) {
            result = -1;
            syslog(LOG_ERR, "[UART::sendResponseMessage] UART port write failed\n");
            break;
        }

        bytes_sent += n;
        tcdrain(fd);
    }
    pthread_mutex_unlock(&sendMessageLock);
    return result;
}



int readData(int fd, unsigned char *buffer, int buf_len)
{
    int len = 0;
    memset(buffer, '\0', sizeof(unsigned char)*buf_len);
    pthread_mutex_lock(&sendMessageLock);
    while (len >= 0 && len < buf_len)
    {
        len += recv(fd, buffer + len, buf_len - len,0);
        if (len > 0 && (buffer[len - 1] == '\0' || buffer[len - 1] == '\n'))
        {
            break;
        }
    }

    pthread_mutex_unlock(&sendMessageLock);
    return len;
}

struct CmdMessage* parseJson(const char* jsonString)
{
    int i, r;
    jsmn_parser p;
    jsmntok_t t[128];

    struct CmdMessage* pMsgRet = (struct CmdMessage*)commandMessage;
    memset(pMsgRet, 0, sizeof(struct CmdMessage));

    jsmn_init(&p);
    r = jsmn_parse(&p, jsonString, strlen(jsonString), t, sizeof(t) / sizeof(t[0]));
    if (r < 0)
    {
        printf("[ParseJSON] Failed to parse JSON: %d\n", r);
        strcpy(pMsgRet->command, "error");
        strcpy(pMsgRet->params, "Failed to parse JSON");
        return pMsgRet;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT)
    {
        printf("[ParseJSON] failed Object expected\n");
        strcpy(pMsgRet->command, "error");
        strcpy(pMsgRet->params, "Failed to parse JSON Object expected");
        return pMsgRet;
    }

    for (i = 1; i < r; i += 2)
    {
        char prop[128];

        sprintf(prop, "%.*s", t[i].end - t[i].start,
            (const char*)jsonString + t[i].start);

        if (strcmp(prop, "command") == 0) {
            sprintf(pMsgRet->command, "%.*s", t[i + 1].end - t[i + 1].start,
                (const char*)jsonString + t[i + 1].start);
        }
        else if (strcmp(prop, "params") == 0) {
            sprintf(pMsgRet->params, "%.*s", t[i + 1].end - t[i + 1].start,
                (const char*)jsonString + t[i + 1].start);
        }
    }

    return pMsgRet;
}

struct CmdMessage* getNextCommand(int fd)
{
    int len = 0;
    int buf_len = 1024;
    unsigned char buffer[1024];
    memset(buffer, '\0', sizeof(unsigned char)*buf_len);
    pthread_mutex_lock(&sendMessageLock);
    while (len >= 0 && len < buf_len)
    {
        len += recv(fd, buffer + len, buf_len - len,0);
        if (len > 0 && (buffer[len - 1] == '\0' || buffer[len - 1] == '\n'))
        {
            break;
        }
    }
    pthread_mutex_unlock(&sendMessageLock);
    if (len > 0 && strlen((char *)buffer) > 0)
    {
        printf("[UART] Message Received: %s\n", (const char*)buffer);
        return parseJson((const char*)buffer);
    }
    return NULL;
}
