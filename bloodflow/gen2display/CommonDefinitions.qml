import QtQuick 2.12

Item {
    id: konsts
    enum ScreensState {
        Splash,            //0
        Home,              //1
        Placement,         //2
        ScanComplete,      //3
        PasswordScreen,    //4
        MaintainanceHome,  //5
        LaserEnergyScreen, //6
        CameraCalibration, //7
        PatientDataView,   //8
        FileManager,       //9
        None               //10
    }

    enum ModuleSequence {
        RFissure,          //0
        LFissure,          //1
        RForehead,         //2
        LForehead          //3
    }

    property string msg_set_state : "set_state"
    property string msg_response : "response"   // see cmd_ values below
    property string msg_data : "data"
    property string msg_info : "info"
    property string msg_error : "error"

    property string state_busy : "busy"
    property string state_error : "error"
    property string state_estop : "estop"
    property string state_home : "home"
    property string state_patient : "patient"
    property string state_shutdown : "shutdown"
    property string state_startup : "startup"
    property string flash_drive_connected : "connected"
    property string flash_drive_disconnected : "disconnected"
    property string flash_drive_status : "sys_flash_drive_status"

    property string cmd_open_patient : "open_patient"
    property string cmd_scan_patient : "scan_patient"
    property string cmd_patient_data : "get_patient_graph"
    property string cmd_save_note : "save_patient_note"
    property string cmd_close_patient : "close_patient"
    property string cmd_backup : "sys_backup"
    property string cmd_backup_logs : "sys_backup_logs"
    property string cmd_get_state : "sys_state"
    property string cmd_get_version : "sys_version"
    property string cmd_shutdown : "sys_shutdown"
    property string cmd_update : "sys_update"
    property string cmd_cleanup : "sys_cleanup"
    property string cmd_get_time : "sys_time"
    property string cmd_get_temp : "sys_pers_board_temp"
    property string cmd_get_files : "sys_list_files"
    property string cmd_delete_files : "sys_delete_files"
    property string cmd_transfer_files : "sys_transfer_files"
    property string cmd_get_free_internal_storage : "sys_get_internal_storage"


    // camera module status
    // 'ready' means that the module checks out
    // 'ambient' means there's too much ambient light
    // 'laser' means there's a problem w/ the laser
    // 'unknown' means that the module status is not known

    property string cam_module_ready_color          : "#b0ede0"
    property string cam_module_ambient_color        : "#d7c648"
    property string cam_module_laser_color          : "#bf8339"
    property string cam_module_ambient_laser_color  : "#a3321c"
    property string cam_module_unknown_color        : "black"

    property int cam_module_ready_num               : 0
    property int cam_module_ambient_num             : 1
    property int cam_module_laser_num               : 2
    property int cam_module_ambient_laser_num       : 3
    property int cam_module_unknown_num             : 4

    property int response_pending_timeout_sec       : 120
}
