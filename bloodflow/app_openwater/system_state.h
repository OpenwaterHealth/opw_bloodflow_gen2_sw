// Thread safe finite state machine

#ifndef _SYSTEM_STATE_H
#define _SYSTEM_STATE_H

#include <stdbool.h>

typedef enum
{
    OW_UNDEFINED,
    OW_STATE_STARTUP,
    OW_STATE_IDLE,
    OW_STATE_READY,
    OW_STATE_BUSY,
    OW_STATE_ERROR,
    OW_STATE_SHUTDOWN,
    OW_STATE_ESTOP,
    OW_MAX_STATES
} ow_system_state_t;

static const char* const OW_SYSTEM_STATE_STRING[OW_MAX_STATES] = {
    "OW_UNDEFINED",
    "startup",
    "home",
    "patient",
    "busy",
    "error",
    "shutdown",
    "estop",
};

extern uint8_t g_ignore_error_state;

// Must be called to initialize state machine and mutex
void init_state();

// Called to destroy mutex
void shutdown_state();

// Return current system state
ow_system_state_t current_state();

// Returns false if current state does not match given expected_current_state, otherwise
// current state changes to new_state and function returns true. To ignore current state
// pass OW_UNDEFINED as expected_current_state, but use caution when doing so since in
// general not all state transistions should be allowed.
bool change_state(ow_system_state_t new_state, ow_system_state_t expected_current_state);

// Enter error state and store error message.
bool error_state(const char *error_msg);

// Return last error message.
const char *error_msg();

#endif