import graph_responses

# Sequences of communication between UI and App

# System Startup
## TDA4 -> UI   


# Successful startup path
start_success = [
{"content":"info","state":"startup","command":"sys_state","params":"a","msg":"Starting Openwater System","data":[]},
{"content":"info","state":"startup","command":"sys_state","params":"b","msg":"Warming up sensors","data":[]},
{"content":"info","state":"startup","command":"sys_state","params":"c","msg":"System Ready","data":[]},
{"content":"set_state","state":"home","command":"sys_state","params":"d","msg":"System Ready","data":[]}
]

# Error startup path
start_fail = [
    { 
        "content": "info",
        "state": "startup",
        "msg": "Starting Openwater System"
    },
    
    { 
        "content": "info",
        "state": "startup",
        "msg": "Warming up Laser"
    },
    
    { 
        "content": "info",
        "state": "startup",
        "msg": "Laser warmup 10 seconds remaining"
    },
    
    { 
        "content": "error",
        "state": "shutdown",
        "msg": "Error starting sensor system",
        "params": "code 0x31c0"
    }
]

########################################################################
# Example Open Patient

## UI -> TDA4
#Send Command provide patient ID

open_patient_cmd = {
    "command":"open_patient",
    "params":"patient001"
}


## TDA4 -> UI   

open_patient_success = [
{"content":"info","state":"busy","command":"open_patient","params":"","msg":"Creating patient data store","data":[]},
{"content":"info","state":"patient","command":"open_patient","params":"","msg":"Created Patient data collection store","data":[]},
{"content":"response","state":"patient","command":"open_patient","params":"test001","msg":"J Doe 123","data":[]}
]

open_patient_fail = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "creating data store"
    },
        
    { 
        "content": "error",
        "state": "patient",
        "msg": "Error disk is full",
        "params": "code 0x31c0"
    }
]

########################################################################
# Example Patient Scan Test
## UI -> TDA4
# Send Command provide scan type

test_scan_cmd = {
    "command":"scan_patient",
    "params":"test"
}

## TDA4 -> UI   
test_scan_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Starting Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 1 Scan"
    },
            
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 2 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 3 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 4 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Scan Complete"
    },

    {
        "content": "data",
        "state": "patient",
        "command": "scan_patient",
        "params": "test",
        "data": [1, 2, 3, 0]
    }, 

    {
        "content": "response",
        "state": "patient",
        "command": "scan_patient",
        "params": "test"
    }
]

test_scan_fail = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Starting Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 1 Scan"
    },

    {
        "content": "error",
        "state": "patient",
        "command": "scan_patient",
        "msg": "Sensor contact issue",
        "params": "code 0x31c0"
    }
]


########################################################################
# Example Patient Scan Full
## UI -> TDA4

# Send Command provide scan type
full_scan_cmd = {
    "command":"scan_patient",
    "params":"full"
}

## TDA4 -> UI   

full_scan_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Starting Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 1 Scan"
    },
            
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 2 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 3 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 4 Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Scan Complete"
    },

    { 
        "content": "response",
        "state": "patient",
        "command": "scan_patient",
        "params": "full"
    }
]

full_scan_fail = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Starting Scan"
    },
    
    { 
        "content": "info",
        "state": "busy",
        "msg": "Performing Position 1 Scan"
    },

    {
        "content": "error",
        "state": "patient",
        "command": "scan_patient",
        "msg": "Sensor contact issue",
        "params": "code 0x31c0"
    }
]

# Example Patient Save Note

## UI -> TDA4
# Send Command provide patient notes
save_note_cmd = {
    "command":"save_patient_note",
    "params":"Notes for the patient"
}


## TDA4 -> UI   
save_note_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Saved patient notes"
    },

    { 
        "content": "response",
        "state": "patient",
        "command": "save_patient_note"
    }
]

save_note_fail = [
    {
        "content": "error",
        "state": "patient",
        "command": "save_patient_note",
        "msg": "Failed to save patient notes",
        "params": "code 0x31c0"
    }
]

# Example Patient Get Graph Data
## UI -> TDA4
# Send Command provide scan pair for graph data to retrieve

patient_graph1_cmd = {
    "command":"get_patient_graph",
    "params":"1"
}
patient_graph2_cmd = {
    "command":"get_patient_graph",
    "params":"2"
}
patient_graph3_cmd = {
    "command":"get_patient_graph",
    "params":"3"
}
patient_graph4_cmd = {
    "command":"get_patient_graph",
    "params":"4"
}


## TDA4 -> UI   
#Success Path replace data with float array
patient_graph1_success = [
    {"content":"info","state":"patient","command":"get_patient_graph","params":"","msg":"Retrieving graph data","data":[]},
    graph_responses.graph1_chan0,
    graph_responses.graph1_chan1,
    { 
        "content": "response",
        "state": "patient",
        "command": "get_patient_graph",
        "params": "1"
    }
]

patient_graph2_success = [
    {"content":"info","state":"patient","command":"get_patient_graph","params":"","msg":"Retrieving graph data","data":[]},
    graph_responses.graph2_chan0,
    graph_responses.graph2_chan1,
    { 
        "content": "response",
        "state": "patient",
        "command": "get_patient_graph",
        "params": "2"
    }
]

patient_graph3_success = [
    {"content":"info","state":"patient","command":"get_patient_graph","params":"","msg":"Retrieving graph data","data":[]},
    graph_responses.graph3_chan0,
    graph_responses.graph3_chan1,
    { 
        "content": "response",
        "state": "patient",
        "command": "get_patient_graph",
        "params": "3"
    }
]

patient_graph4_success = [
    {"content":"info","state":"patient","command":"get_patient_graph","params":"","msg":"Retrieving graph data","data":[]},
    graph_responses.graph4_chan0,
    graph_responses.graph4_chan1,
    { 
        "content": "response",
        "state": "patient",
        "command": "get_patient_graph",
        "params": "4"
    }
]


patient_graph_fail = [
    {
        "content": "error",
        "state": "patient",
        "command": "get_patient_graph",
        "msg": "Failed to retrieve patient data",
        "params": "code 0x31c0"
    }
]

# Example Patient Close
## UI -> TDA4
close_patient_cmd = {
    "command":"close_patient"
}


## TDA4 -> UI   
close_patient_success = [
    { 
        "content": "response",
        "state": "home",
        "command": "close_patient"
    }
]

close_patient_fail = [
    {
        "content": "error",
        "state": "patient",
        "command": "close_patient",
        "msg": "Failed to close patient data collection",
        "params": "code 0x31c0"
    }
]


# Example Retrieve System State
## UI -> TDA4
sys_state_cmd = {
    "command":"sys_state"
}


## TDA4 -> UI   
sys_state_success = [
    { 
        "content": "response",
        "state": "startup",
        "command": "sys_state"
    }
]


# Retrieve device info
## UI -> TDA4
sys_device_info_cmd = {
    "command":"sys_device_info",
    "value": ""
}


## TDA4 -> UI
sys_device_info_success = [
    { 
        "content": "response",
        "state": "home",
        "command": "sys_device_info",
        "msg": "Device version 1.2.3\nOther device info"
    }
]

################################################
# Version

## UI -> TDA4
sys_version_cmd = {
    "command":"sys_version",
    "value": ""
}


## TDA4 -> UI   
sys_version_success = [
    { 
        "content": "response",
        "state": "home",
        "command": "sys_version",
        "msg": "0.2.0"
    }
]

## UI -> TDA4
sys_cfg_version_cmd = {
    "command":"cfg_version",
    "value": ""
}


## TDA4 -> UI   
sys_cfg_version_success = [
    { 
        "content": "response",
        "state": "startup",
        "command": "cfg_version",
        "msg": "1.2.3"
    }
]

## UI -> TDA4
sys_fs_version_cmd = {
    "command":"fs_version",
    "value": ""
}


## TDA4 -> UI   
sys_fs_version_success = [
    { 
        "content": "response",
        "state": "startup",
        "command": "fs_version",
        "msg": "3.0.1"
    }
]

################################################

# Example System Shutdown
## UI -> TDA4
sys_shutdown_cmd = {
    "command":"sys_shutdown"
}


## TDA4 -> UI   
sys_shutdown_success = [
    { 
        "content": "response",
        "state": "shutdown",
        "command": "sys_shutdown",
        "msg": "You may safely power off the device."
    }
]


# Delete archived files from device
## UI -> TDA4
sys_backup_logs_cmd = {
    "command":"sys_backup_logs"
}

sys_time_cmd = {
    "command" : "sys_time"
}
## TDA4 -> UI   
sys_backup_logs_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Backing up logs..."
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "Logs successfully backed up"
    },

    { 
        "content": "response",
        "state": "home",
        "command": "sys_backup_logs",
        "msg": "Backup complete"
    }
]

sys_time_success = [
    { 
        "content": "response",
        "state": "home",
        "command": "sys_time",
        "msg": "now!"
    }
]


# Delete archived files from device
## UI -> TDA4
sys_cleanup_cmd = {
    "command":"sys_cleanup"
}


## TDA4 -> UI   
sys_cleanup_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Deleting files..."
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "Files successfully deleted from device"
    },

    { 
        "content": "response",
        "state": "home",
        "command": "sys_cleanup",
        "msg": "Purge complete"
    }
]


# Example Retrieve System Backup
## UI -> TDA4
sys_backup_cmd = {
    "command":"sys_backup"
}

## TDA4 -> UI   
sys_backup_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Backing up system"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "10% complete",
        "params": "Compressing data for patient J Doe 1234"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "Finished"
    },

    { 
        "content": "response",
        "state": "home",
        "command": "sys_backup",
        "msg": "Backup complete"
    }
]

sys_backup_fail = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Backing up system"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "10% complete"
    },
    
    {
        "content": "error",
        "state": "home",
        "command": "sys_backup",
        "msg": "Failed to backup data",
        "params": "code 0x31c0"
    }
]

# Example Retrieve System Firmware Update
## UI -> TDA4
sys_update_cmd = {
    "command":"sys_update",
    "value": ""
}

## TDA4 -> UI   
sys_update_success = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "Firmware update began please don't power down system"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "10% complete"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "Finished"
    },

    { 
        "content": "response",
        "state": "home",
        "command": "sys_update",
        "msg": "Firmware updated. Please restart device."
    }
]

sys_update_fail = [
    { 
        "content": "info",
        "state": "busy",
        "msg": "10% complete"
    },

    { 
        "content": "info",
        "state": "busy",
        "msg": "Finished"
    },
    
    {
        "content": "error",
        "state": "home",
        "command": "sys_update",
        "msg": "Failed to complete firmware update",
        "params": "code 0x31c0"
    }
]


# Example Retrieve System Scan File List
## UI -> TDA4
sys_get_files_list = {
    "command":"sys_get_files_list",
    "value": ""
}

## TDA4 -> UI
file_list_json_string = '{"data" : [{ \"output\": [{ \"patient1\": [\"scan1\", \"scan2\", \"scan3\"] }, { \"patient2\": [\"scan1\", \"scan2\", \"scan3\"] }, { \"patient3\": [\"scan1\", \"scan2\", \"scan3\"] } ] }, { \"storage\": [{ \"patient4\": [\"scan1\", \"scan2\", \"scan3\"] }, { \"patient5\": [\"scan1\", \"scan2\", \"scan3\"] }, { \"patient6\": [\"scan1\", \"scan2\", \"scan3\"] } ] } ] }'

# file_list_json_string = '{"data": ["out","out2","out3"]}'

# file_list_json_string = '{ "data": [[1, 2, 3], [4, 5, 6]] }'

# file_list_json_string = '[{ "output": [{ "patient1": ["scan1", "scan2", "scan3"] }, { "patient2": ["scan1", "scan2", "scan3"] }, { "patient3": ["scan1", "scan2", "scan3"] } ] }, { "storage": [{ "patient4": ["scan1", "scan2", "scan3"] }, { "patient5": ["scan1", "scan2", "scan3"] }, { "patient6": ["scan1", "scan2", "scan3"] } ] } ]'
sys_get_files_list_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_get_files_list",
        "msg": file_list_json_string
    }
]

# Example Retrieve System Scan File List
## UI -> TDA4
sys_delete_files = {
    "command":"sys_delete_files",
    "value": ""
}

## TDA4 -> UI   
sys_delete_files_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_delete_files",
        "msg": "Files deleted."
    }
]
## TDA4 -> UI   
sys_delete_files_fail = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_delete_files",
        "msg": "Unable to delete files."
    }
]

# Example Retrieve System Scan File List
## UI -> TDA4
sys_transfer_files = {
    "command":"sys_transfer_files",
    "value": ""
}

## TDA4 -> UI   
sys_transfer_files_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_transfer_files",
        "msg": "Files transferred."
    }
]
## TDA4 -> UI   
sys_transfer_files_fail = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_transfer_files",
        "msg": "Unable to transfer files."
    }
]


# Example Retrieve System Scan File List
## UI -> TDA4
sys_pers_board_temp = {
    "command":"sys_pers_board_temp",
    "value": ""
}

## TDA4 -> UI   
sys_pers_board_temp_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_pers_board_temp",
        "msg": "25 C"
    }
]

# Example Retrieve System Scan File List
## UI -> TDA4
sys_get_internal_storage = {
    "command":"sys_get_internal_storage",
    "value": ""
}

## TDA4 -> UI   
sys_get_internal_storage_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_get_internal_storage",
        "msg": "10.2 GB"
    }
]

# Example Flash Drive status message
## UI -> TDA4
sys_get_internal_storage = {
    "command":"sys_get_internal_storage",
    "value": ""
}

## TDA4 -> UI   
sys_get_internal_storage_success = [

    { 
        "content": "response",
        "state": "home",
        "command": "sys_get_internal_storage",
        "msg": "10.2 GB"
    }
]


# Example Retrieve EStop

## TDA4 -> UI   
estop = [
    {
        "content":"set_state",
        "state":"estop",
        "command":"sys_state",
        "msg":"Emergency stop",
        "data":[]
    },

    {
        "content": "error",
        "state": "estop",
        "msg": "Emergency stop button pressed. Lasers off."
    }
]

unplug_flash_drive = [
    {
        "content":"set_state",
        "state":"flash_drive_disconnected"
    }
]

plug_in_flash_drive = [
    {
        "content":"set_state",
        "state":"flash_drive_connected"
    }
]

# scenarios are stored by <command>_<success|fail>
# stored object is array of [command, response sequence]
scenarios = {
    "start_success": [ {}, start_success ],
    "start_fail": [ {}, start_fail ],
    "open_patient_success": [ open_patient_cmd, open_patient_success ],
    "open_patient_fail": [ open_patient_cmd, open_patient_fail ],
    "test_scan_success": [ test_scan_cmd, test_scan_success ],
    "test_scan_fail": [ test_scan_cmd, test_scan_fail ],
    "full_scan_success": [ full_scan_cmd, full_scan_success ],
    "full_scan_fail": [ full_scan_cmd, full_scan_fail ],
    "save_patient_note_success": [ save_note_cmd, save_note_success ],
    "save_patient_note_fail": [ save_note_cmd, save_note_fail ],
    "get_patient_graph1_success": 
            [ patient_graph1_cmd, patient_graph1_success ],
    "get_patient_graph2_success": 
            [ patient_graph2_cmd, patient_graph2_success ],
    "get_patient_graph3_success": 
            [ patient_graph3_cmd, patient_graph3_success ],
    "get_patient_graph4_success": 
            [ patient_graph4_cmd, patient_graph4_success ],
    "get_patient_graph1_fail": [ patient_graph1_cmd, patient_graph_fail ],
    "get_patient_graph2_fail": [ patient_graph2_cmd, patient_graph_fail ],
    "get_patient_graph3_fail": [ patient_graph3_cmd, patient_graph_fail ],
    "get_patient_graph4_fail": [ patient_graph4_cmd, patient_graph_fail ],

    "close_patient_success": [ close_patient_cmd, close_patient_success ],
    "close_patient_fail": [ close_patient_cmd, close_patient_fail ],
    "sys_backup_success": [ sys_backup_cmd, sys_backup_success ],
    "sys_backup_fail": [ sys_backup_cmd, sys_backup_fail ],
    "sys_update_success": [ sys_update_cmd, sys_update_success ],
    "sys_update_fail": [ sys_update_cmd, sys_update_fail ],
    "sys_get_files_list": [ sys_get_files_list, sys_get_files_list_success ],
    "sys_delete_files": [ sys_delete_files, sys_update_success ],
    "sys_transfer_files" : [ sys_transfer_files, sys_update_success],
    "sys_pers_board_temp" : [sys_pers_board_temp, sys_pers_board_temp_success ],
    "unplug_flash_drive" : [unplug_flash_drive],
    "plug_in_flash_drive" : [plug_in_flash_drive]
}

scenarios_nofail = {
    "sys_state": [ sys_state_cmd, sys_state_success ],
    "sys_version": [ sys_version_cmd, sys_version_success ],
    "cfg_version": [ sys_cfg_version_cmd, sys_cfg_version_success ],
    "fs_version": [ sys_fs_version_cmd, sys_fs_version_success ],
    "sys_device_info": [ sys_device_info_cmd, sys_device_info_success ],
    "sys_shutdown": [ sys_shutdown_cmd, sys_shutdown_success],
    "sys_cleanup": [ sys_cleanup_cmd, sys_cleanup_success],
    "sys_backup_logs": [ sys_backup_logs_cmd, sys_backup_logs_success],
    "sys_time" : [sys_time_cmd, sys_time_success],
    "sys_get_files_list": [ sys_get_files_list, sys_get_files_list_success ],
    "sys_delete_files": [ sys_delete_files, sys_delete_files_success ],
    "sys_transfer_files" : [ sys_transfer_files, sys_transfer_files_success],
    "sys_pers_board_temp" : [sys_pers_board_temp, sys_pers_board_temp_success ],
    "sys_get_internal_storage" : [sys_get_internal_storage, sys_get_internal_storage_success],


}

scenarios_push = [
    { "estop": estop }
]

