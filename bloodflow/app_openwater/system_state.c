#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h> /* POSIX Threads */
#include <stdint.h>
#include <syslog.h>

#include "uart.h"
#include "system_state.h"

#define MAX_ERROR_MSG_LEN 1024

pthread_mutex_t p_mutex_state;
uint8_t g_ignore_error_state = 0;

ow_system_state_t g_system_state = OW_STATE_STARTUP;
char g_last_error_msg[MAX_ERROR_MSG_LEN];

extern int fd_serial_port;

void init_state()
{
    pthread_mutex_init(&p_mutex_state, NULL);
    change_state(OW_STATE_STARTUP, OW_UNDEFINED);
    memset(g_last_error_msg, 0, sizeof(g_last_error_msg));
}

void shutdown_state()
{
    pthread_mutex_destroy(&p_mutex_state);
}

ow_system_state_t current_state()
{
    return g_system_state;
}

bool change_state(ow_system_state_t new_state, ow_system_state_t expected_current_state)
{
    pthread_mutex_lock(&p_mutex_state);
    if (new_state == OW_STATE_ESTOP || (g_system_state == expected_current_state))
    {
        g_system_state = new_state;
    }
    else if(g_system_state != OW_STATE_ESTOP && (new_state == OW_STATE_ERROR || expected_current_state == OW_UNDEFINED))
    {
        g_system_state = new_state;
    }
    pthread_mutex_unlock(&p_mutex_state);
    return g_system_state == new_state;
}

bool error_state(const char *error_msg)
{
    syslog(LOG_ERR, error_msg);
    snprintf(g_last_error_msg, sizeof(g_last_error_msg), "Internal error - please report to Openwater: %s", error_msg);
    if (!g_ignore_error_state)
    {
        change_state(OW_STATE_ERROR, OW_UNDEFINED);
    }

    return !g_ignore_error_state;
}

const char *error_msg()
{
    return g_last_error_msg;
}
