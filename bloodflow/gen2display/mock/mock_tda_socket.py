#!/usr/bin/env python3
import os
import sys
import json
import threading
import time
import subprocess
import socket
from ui_tda4_coms import scenarios, scenarios_nofail, scenarios_push, estop

#TDA_TTY = "/dev/ttyS5"
TDA_TTY = "/tmp/vserial1"
DISPLAY_TTY = "/tmp/vserial2"   # not used here -- for reference only

#subprocess.run(["stty", "-F", TDA_TTY, "115200"])

STATES = [
    "startup",
    "busy",
    "home",
    "patient",
    "shutdown"
]

MODES = [
    "success",
    "fail"
]
client_socket = None
quit_ = 0
state_ = STATES[0]
mode_ = MODES[0]
flash_drive_connected = False

input_buffer_ = ""


def listener_main():
    global input_buffer_
    while quit_ == 0:
        stream = None
        try:
            data = client_socket.recv(1).decode()  # receive response
        except BlockingIOError:
            time.sleep(0.25)
            continue
        if len(data) > 0:
            c = data
            input_buffer_ += c
            if c[0] == '\n':
                jsonMsg = json.loads(input_buffer_)
                send_response(jsonMsg)
                input_buffer_ = ""
        else:
            time.sleep(0.25)


def compare_commands(jsonMsg, expectedMsg):
    print("Received: ", str(jsonMsg))
    print("Expected: ", str(expectedMsg))


def execute_sequence(messages):
    for msg in messages:
        content = json.dumps(msg) + "\r\n"
        out_buf = content.encode('utf-8')
        n_bytes = len(out_buf)
        n_written = 0
        # while n_written < n_bytes:
        try: 
            client_socket.send(out_buf)  # send message
        except BlockingIOError:
            time.sleep(0.1)
        print("Sent (%d): %s" % (n_written, content))
        time.sleep(1)


# look up response sequence to send and send each message in it,
#   separated by a small delay
# scenario names start with command and is followed by _success or _fail
#   (or not, for nonfail)
def send_response(jsonMsg):
    print("Responding to " + str(jsonMsg))
    # Look up command and then send response(s)
    processed = False
    print(str(jsonMsg))
    cmd = jsonMsg["command"]
    if cmd == "scan_patient":
        if jsonMsg["params"] == "test":
            cmd = "test_scan"
        else:
            cmd = "full_scan"
    cmd_name = cmd
    if cmd.startswith("get_patient_graph"):
        cmd_name += jsonMsg["params"]
    # look for the non-fail commands (it's shorter and easier)
    for key in scenarios_nofail:
        print("Compare ", key, cmd) 
        if key == cmd:
            sequence = scenarios_nofail[key]
            compare_commands(jsonMsg, sequence[0])
            execute_sequence(sequence[1])
            processed = True
            break
    if not processed:
        # command not no-fail. check other scenarios
        print("Compare ", key, cmd) 
        for key in scenarios:
            if key.endswith(mode_) and key.startswith(cmd_name):
                sequence = scenarios[key]
                compare_commands(jsonMsg, sequence[0])
                execute_sequence(sequence[1])
                processed = True
                break
    if not processed:
        print("ERROR: no response found for %s" % cmd)


def show_usage():
    print("Commands:")
    print("     q       quit")
    print("     h       help (this message)")
    print("     start   TDA boot sequence")
    print("     success switch mode to 'success'")
    print("     fail    switch mode to 'fail'")
    print("     estop   send estop shutdown message")
    print("     flash   toggle flash drive status")



if __name__ == "__main__":
    # establish socket connection
    host = socket.gethostname()  # as both code is running on same pc
    port = 12325  # socket server port number

    client_socket = socket.socket()  # instantiate
    client_socket.connect((host, port))  # connect to the server
    if client_socket is None:
        print("Failed to open")
        sys.exit(1)
    print("Opened socket for writing")

    # Launch listener thread
    t = threading.Thread(target=listener_main, args=[])
    t.start()

    show_usage()

    while quit_ == 0:
        try:
            cmd = input().strip()
            print("CMD: ", cmd)
            if cmd == 'q':
                break
            elif cmd == 'h':
                show_usage()
            elif cmd == 'estop':
                execute_sequence(estop)
            elif cmd == 'start':
                k = cmd + "_" + mode_
                seq = scenarios[k][1]
                print("Executing start")
                execute_sequence(seq)
            elif cmd == 'flash':
                if not flash_drive_connected:
                    print("Plugging in flash drive")
                    execute_sequence(scenarios["plug_in_flash_drive"][0])
                    flash_drive_connected = True
                else:
                    print("Unplugging flash drive")
                    execute_sequence(scenarios["unplug_flash_drive"][0])
                    flash_drive_connected = False
                
            elif cmd in MODES:
                mode_ = cmd
                print("Mode set to %s" % mode_)
            else:
                handled = False
                for k in scenarios_push:
                    if cmd == k:
                        print("Scenario " + k)
                        messages = scenarios_push[k]
                        for msg in messages:
                            content = json.dumps(msg) + "\r\n"
                            os.write(tty_, contentencode('utf-8'))
                            print("Sent:", cmd)
                            time.sleep(1)
                        handled = True
                        break
                if not handled:
                    print("Unrecognized command")
        except Exception as e:
            print(e)
            break
    
    quit_ = 1
    sys.exit()
    client_socket.close()  # close the connection