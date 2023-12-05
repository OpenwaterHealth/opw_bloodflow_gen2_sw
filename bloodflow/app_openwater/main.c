/*
 *
 * Copyright (c) 2022 Openwater
 *
 */

#include <TI/tivx.h>
#include <TI/j7.h>
#include <tivx_utils_file_rd_wr.h>

#include <utils/draw2d/include/draw2d.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/console_io/include/app_get.h>
#include <utils/grpx/include/app_grpx.h>
#include <utils/iss/include/app_iss.h>
#include <utils/console_io/include/app_log.h>
#include <VX/vx_khr_pipelining.h>
#include <utils/remote_service/include/app_remote_service.h>

#include <TI/tivx_ow_algos.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include <libtar.h>
#include <signal.h>

#include <fcntl.h>
#include <pthread.h> /* POSIX Threads */
#include "owconnector.h"
#include "pymodule_lib.h"

#include "app_common.h"
#include "app_sensor_module.h"
#include "app_cust_cap_module.h"
#include "app_viss_module.h"
#include "app_aewb_module.h"
#include "app_ldc_module.h"
#include "app_histo_module.h"
#include "app_streaming_module.h"
#include "app_commands.h"
#include "app_messages.h"
#include "include/ow_common.h"
#include "network.h"
#include "lsw_laser_pdu.h"
#include "system_state.h"
#include "include/headSensorService.h"
#include "encrypt_data.h"
#include "../../../git_version.h"

#define ENABLE_VERBOSE 1
#define MAX_JSON_TOKENS 32



#define NUM_CHANNELS        (3U)


#define APP_BUFFER_Q_DEPTH   (4)
#define APP_PIPELINE_DEPTH   (7)

typedef struct {

    SensorObj     left_sensor;
    SensorObj     right_sensor;
    CaptureObj    left_capture;
    CaptureObj    right_capture;

    /* Histogram object */
    HistoObj      left_histo;
    HistoObj      right_histo;

    vx_char output_file_path[APP_MAX_FILE_PATH];
    vx_char archive_file_path[APP_MAX_FILE_PATH];

    /* OpenVX references */
    vx_context context;
    vx_graph   graph;
    vx_uint32  en_ignore_graph_errors;
    vx_uint32  en_ignore_error_state;

    vx_int32 test_mode;

    vx_uint32 is_interactive;
    vx_int32 input_streaming_type;

    vx_uint32 num_histo_bins;

    vx_uint32 num_frames_to_run;

    vx_uint32 num_frames_to_write;
    vx_uint32 num_frames_to_skip;

    vx_uint32 num_histo_frames_to_write;
    vx_uint32 num_histo_frames_to_skip;

    tivx_task task;
    vx_uint32 stop_task;
    vx_uint32 stop_task_done;

    app_perf_point_t total_perf;
    app_perf_point_t fileio_perf;
    app_perf_point_t draw_perf;

    int32_t pipeline;

    int32_t enqueueCnt;
    int32_t dequeueCnt;

    int32_t write_file;
    int32_t write_histo;
    int32_t scan_type;
    int32_t wait_histo;
    int32_t wait_file;
    int32_t wait_err;
    char wait_msg[256];
    char g_patient_path[64];
    char g_sub_path[32];
    char g_deviceID[24];

    uint32_t frame_sync_period;
    uint32_t frame_sync_duration;
    uint32_t strobe_delay_start;
    uint32_t strobe_duration;
    uint32_t timer_resolution;

    uint32_t dark_frame_count;
    uint32_t light_frame_count;
    uint32_t test_scan_frame_count;
    uint32_t full_scan_frame_count;

    uint32_t contact_thr_low_no_gain;
    uint32_t contact_thr_low_16_gain;
    uint32_t contact_thr_room_light;

    bool camera_pair_contact_result[4];

    uint32_t screendIdx; // Can be 0 or 1. 0 to show the first 4 cameras to the monitor and 1 to show the next 4 cameras

    int32_t currentActivePair; // 0,1,2,3 or -1 for all cameras active
    int32_t previousActivePair; // 0,1,2,3 or -1 for all cameras active

    int32_t currentActiveScanType; 
    int32_t previousActiveScanType;

    uint32_t num_histo_captures_long_scan;

    uint8_t camera_config_set[4];
    uint8_t camera_config_test;
    uint8_t camera_config_long;

    lsw_laser_pdu_config_t lsw_laser_pdu_config;

} AppObj;

AppObj gAppObj;

static void app_parse_cmd_line_args(AppObj *obj, vx_int32 argc, vx_char *argv[]);
static vx_status app_init(AppObj *obj);
static void app_deinit(AppObj *obj);
static vx_status app_create_graph(AppObj *obj);
static vx_status app_verify_graph(AppObj *obj);
static vx_status app_run_graph(AppObj *obj, uint64_t phys_ptr, void *virt_ptr);
static vx_status app_run_graph_uart_mode(AppObj *obj);
static vx_status app_run_graph_interactive(AppObj *obj);
static void app_delete_graph(AppObj *obj);
static void app_default_param_set(AppObj *obj);
static void app_update_param_set(AppObj *obj);

static void app_run_task_delete(AppObj *obj);
static int32_t app_run_task_create(AppObj *obj);
static void app_run_task(void *app_var);
volatile sig_atomic_t sigkill_active = 0;

static void app_pipeline_params_defaults(AppObj *obj);
static void set_sensor_defaults(SensorObj *sensorObj);
static void add_graph_parameter_by_node_index(vx_graph graph, vx_node node, vx_uint32 node_parameter_index);
static vx_status app_run_graph_for_one_frame_pipeline(AppObj *obj, vx_int32 frame_id);
static void app_parse_cfg_file(AppObj *obj, vx_char *cfg_file_name);
static void app_set_cfg_default(AppObj *obj);

static int32_t app_switch_camera_pair(AppObj *obj, int32_t pair_num, int32_t scan_type);
static uint32_t app_get_pair_mask(AppObj *obj, int32_t pair_num, int32_t scan_type);

static bool set_patient_id(AppObj *obj, const char* patientID);
static bool set_cam_pair(AppObj *obj, int pair);
static bool set_patient_note(AppObj *obj, const char* patientNote);
static bool perform_scan(AppObj *obj, uint32_t num_frames, int32_t scan_type);
static bool retrieve_patient_data(AppObj *obj, const char* val);

static bool run_scan_sequence(AppObj *obj, uint32_t dark_frames, uint32_t light_frames, uint32_t histo_frames, int32_t scan_type, uint32_t camera_pair);
int32_t appRemoteServiceHeadSensorHandler(char *service_name, uint32_t cmd, void *prm, uint32_t prm_size, uint32_t flags);
// static float fx(float f) { return isnan(f) ? 0 : f; }

static uint32_t goodSensorContact[8];
static uint32_t imageMean[4];

char usbPath[128];
char deviceName[512];
char s_generic_message[2048];

time_t current_time;
char time_string[50]={0};
char temp_string[50]={0};
struct tm* time_info;

static uint32_t cam_temp_lower_output = 0;  
static uint32_t cam_temp_upper_output = 0;  

// TODO: consider putting this back in uart.c and abstracting the file descriptor away from uart calls
int fd_serial_port = -1;

static void app_show_usage(vx_int32 argc, vx_char* argv[])
{
    printf("\n");
    printf(" Openwater Usecase - (c) Openwater 2022\n");
    printf(" ========================================================\n");
    printf("\n");
    printf(" Usage,\n");
    printf("  %s --cfg <config file>\n", argv[0]);
    printf("\n");
}

static char usecase_menu[] = {
    "\n"
    "\n ========================="
    "\n Openwater : Use Case"
    "\n Build Date: "
    GIT_BUILD_DATE
    "\n Version: "
    GIT_BUILD_VERSION
    "\n ========================="
    "\n"
    "\n 1-4: Switch to camera pair 1 through 4."
    "\n     Cameras are paired adjacently based on port index. Histogram enabled for the selected pair only."
    "\n"
    "\n t: Test Patient Scan"
    "\n f: Full Patient Scan"
    "\n h: Save Histogram"
    "\n s: Save Raw Image"
    "\n a: Save Raw Image and Histogram"
    "\n l: Enable Laser GPO"
    "\n k: Disable Laser GPO"
    "\n w: Change Laser Switch Channel"
    "\n"
    "\n v: view temp of camera sensors"
    "\n c: Read personality board temperature"
    "\n p: Print performance statistics"
    "\n u: D3 Utility Menu"
    "\n"
    "\n x: Exit"
    "\n"
};

static char utils_menu0[] = {
    "\n"
    "\n =================================="
    "\n I2C Utilities Menu"
    "\n =================================="
    "\n"
    "\n i 1: I2C Read"
    "\n o 2: I2C Write"
    "\n   3: Set Mode"
    "\n   4: Set Device Address"
    "\n r 5: Set Register Address"
    "\n d 6: Set Data"
    "\n"
    "\n x: Exit"
    "\n"
};

static char utils_menu1[] = {
    "\n"
    "\n 0: 8A8D"
    "\n 1: 8A16D"
    "\n 2: 16A8D"
    "\n 3: 16A16D"
    "\n"
    "\n Enter Choice: "
    "\n"
};

static char* replace_char(char* str){
    int len = strlen(str);
    int pos = 0;
    while (pos<len) {
        char curr_char = str[pos];
        if(isalnum(curr_char)==0){
            str[pos] = '_';
        }
        pos++;
    }
    return str;
}
    
static int remove_directory(const char *path) {
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d) {
      struct dirent *p;

      r = 0;
      while (!r && (p=readdir(d))) {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
             continue;

          len = path_len + strlen(p->d_name) + 2; 
          buf = malloc(len);

          if (buf) {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);
             if (!stat(buf, &statbuf)) {
                if (S_ISDIR(statbuf.st_mode))
                   r2 = remove_directory(buf);
                else
                   r2 = unlink(buf);
             }
             free(buf);
          }
          r = r2;
      }
      closedir(d);
   }

   if (!r)
      r = rmdir(path);

   return r;
}

static int mkpath(const char *path, mode_t mode)
{
	char tmp[PATH_MAX];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp),"%s",path);
	len = strlen(tmp);
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++)
		if(*p == '/') {
			*p = 0;
			if (mkdir(tmp, mode) < 0 && errno != EEXIST)
				return -1;
			*p = '/';
		}
	if (mkdir(tmp, mode) < 0 && errno != EEXIST)
		return -1;
	return 0;
}

void set_usb_path(char device_letter, int8_t partition_number)
{
    if (partition_number > 0)
    {
        sprintf(usbPath, "/run/media/sd%c%u", device_letter, partition_number);
    }
    else
    {
        sprintf(usbPath, "/run/media/sd%c", device_letter);
    }
}

bool find_usb_stick()
{
    char device_letter;
    int8_t partition_number;
    char command[256];
    FILE *f = NULL;

    for (device_letter = 'a'; device_letter < 'g'; device_letter++)
    {
        // This iterates in reverse order because for example a USB stick may
        // mount as either /run/media/sda or /run/media/sda1. If mounted as
        // sda1 but we first look for sda we will think we found it on
        // sda rather than sda1
        for (partition_number = 7; partition_number >= 0; partition_number--)
        {
            set_usb_path(device_letter, partition_number);
            sprintf(command, "mount | grep %s", usbPath);
            f = popen(command, "r");
            if (NULL != f && EOF != fgetc(f))
            {
                pclose(f);
                return true;
            }
            if (NULL != f)
            {
                pclose(f);
            }
        }
    }
    return false;
}

//TODO: handle instance ids
int32_t appRemoteServiceHeadSensorHandler(char *service_name, uint32_t cmd,
    void *prm, uint32_t prm_size, uint32_t flags)
{
    int32_t status = 0;
    gAppObj.wait_err = 0;
    memset(gAppObj.wait_msg, 0, 256);

    if (NULL != prm)
    {
        typeHEAD_SENSOR_MESSAGE* pHSM = prm;
        switch(cmd)
        {
            case HS_CMD_UPDATE_SENSOR_STATE:            
                syslog(LOG_NOTICE, "[OW_MAIN::appRemoteServiceHeadSensorHandler] HS_CMD_UPDATE_SENSOR_STATE called STATE: %d Mean: [%d,%d,%d,%d]", pHSM->state, pHSM->imageMean[0], pHSM->imageMean[1], pHSM->imageMean[2], pHSM->imageMean[3]);                        
                if(pHSM->state == 0)
                {
                    imageMean[0] = pHSM->imageMean[0];
                    imageMean[1] = pHSM->imageMean[1];
                    imageMean[2] = pHSM->imageMean[2];
                    imageMean[3] = pHSM->imageMean[3];
                    gAppObj.wait_file = 0;
                }
                else
                {               
                    syslog(LOG_ERR, "[OW_MAIN::appRemoteServiceHeadSensorHandler] Update Sensor State Failure: %s", pHSM->msg);
                    gAppObj.wait_file = 0;
                    gAppObj.wait_err = 1;
                }

                break;
            case HS_CMD_HISTO_DONE_STATE:
                if(pHSM->state != 0)
                {
                    printf("HISTO Write Failure: %s\n", pHSM->msg);
                    strcpy(gAppObj.wait_msg, pHSM->msg);
                    gAppObj.wait_err = 1;
                    syslog(LOG_ERR, "[OW_MAIN::appRemoteServiceHeadSensorHandler] HISTO Write Failure: %s", pHSM->msg);
                }
                gAppObj.wait_histo = 0;
                break;
            case HS_CMD_FILE_DONE_STATE:
                if(pHSM->state != 0)
                {
                    printf("IMAGE Write Failure: %s\n", pHSM->msg);
                    strcpy(gAppObj.wait_msg, pHSM->msg);
                    gAppObj.wait_err = 1;
                    syslog(LOG_ERR, "[OW_MAIN::appRemoteServiceHeadSensorHandler] IMAGE Write Failure: %s", pHSM->msg);
                }
                gAppObj.wait_file = 0;

                break;
            case HS_CMD_STAT_MSG:
                break;
            case HS_CMD_CAMERA_STATUS:
                break;
            default:
                syslog(LOG_NOTICE, "[OW_MAIN::appRemoteServiceHeadSensorHandler] Unhandled Command ID Recieved: %d", cmd);
                break;
        }

    }
    else
    {
        printf("ContactHandler: Invalid parameters passed !!!\n");
    }

    return status;
}

// Handler for system signals (such as SIGKILL)
void sigkill_handler(int signum)
{
    sigkill_active = 1;
}

vx_int32 app_openwater_main(vx_int32 argc, vx_char* argv[])
{
    AppObj *obj = &gAppObj;
    vx_status status = VX_SUCCESS;
    struct stat st = {0};

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("openwater", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "[OW_MAIN] Application Start Openwater Build Date: %s Version: %s", GIT_BUILD_DATE, GIT_BUILD_VERSION);

    printf("\n\n[OW_MAIN] App Openwater\n");
    printf("[OW_MAIN] Build Date: %s\n", GIT_BUILD_DATE);
    printf("[OW_MAIN] GIT Version: %s\n\n", GIT_BUILD_VERSION);

    printf("[OW_MAIN] Initialize Python\n");

    // initialize python
    pymodule_lib_init();

    init_state(); // current state is startup

    /*Default parameter settings*/
    app_default_param_set(obj);

    /*Config parameter reading*/
    app_parse_cmd_line_args(obj, argc, argv);

#if ENABLE_VERBOSE
    printf("[OW_MAIN] Number of left cameras = [%d]\n",obj->left_sensor.num_cameras_enabled);
    printf("[OW_MAIN] Number of right cameras = [%d]\n",obj->right_sensor.num_cameras_enabled);
    printf("[OW_MAIN] Camera Config TEST = [0x%02x]\n", obj->camera_config_test);
    printf("[OW_MAIN] Camera Config LONG = [0x%02x]\n", obj->camera_config_long);
    printf("[OW_MAIN] Camera SET 1 = [0x%02x]\n", obj->camera_config_set[0]);
    printf("[OW_MAIN] Camera SET 2 = [0x%02x]\n", obj->camera_config_set[1]);
    printf("[OW_MAIN] Camera SET 3 = [0x%02x]\n", obj->camera_config_set[2]);
    printf("[OW_MAIN] Camera SET 4 = [0x%02x]\n", obj->camera_config_set[3]);
    printf("[OW_MAIN] Camera left channel mask set to 0x%02X\n", (uint8_t)obj->left_sensor.ch_mask);
    printf("[OW_MAIN] Camera right channel mask set to 0x%02X\n", (uint8_t)obj->right_sensor.ch_mask);
    
    printf("[OW_MAIN] frame_sync_period = [%d]\n", obj->frame_sync_period);
    printf("[OW_MAIN] frame_sync_duration = [%d]\n", obj->frame_sync_duration);
    printf("[OW_MAIN] strobe_delay_start = [%d]\n", obj->strobe_delay_start);
    printf("[OW_MAIN] strobe_duration = [%d]\n", obj->strobe_duration);
    printf("[OW_MAIN] timer_resolution = [%d]\n", obj->timer_resolution);

    printf("[OW_LSW_LASER_PDU] i2c_device = [%s]\n", obj->lsw_laser_pdu_config.i2c_device);
    printf("[OW_LSW_LASER_PDU] gpio_device = [%s]\n", obj->lsw_laser_pdu_config.gpio_device);
    printf("[OW_LSW_LASER_PDU] seed_warmup_time_seconds = [%d]\n", obj->lsw_laser_pdu_config.seed_warmup_time_seconds);
    printf("[OW_LSW_LASER_PDU] seed_preset = [%d]\n", obj->lsw_laser_pdu_config.seed_preset);
    printf("[OW_LSW_LASER_PDU] ta_preset = [%d]\n", obj->lsw_laser_pdu_config.ta_preset);
    printf("[OW_LSW_LASER_PDU] adc_ref_volt_mv = [%d]\n", obj->lsw_laser_pdu_config.adc_ref_volt_mv);
    printf("[OW_LSW_LASER_PDU] dac_ref_volt_mv = [%d]\n", obj->lsw_laser_pdu_config.dac_ref_volt_mv);
    printf("[OW_LSW_LASER_PDU] fiber_switch_channel_left = [%d]\n", obj->lsw_laser_pdu_config.fiber_switch_channel_left);
    printf("[OW_LSW_LASER_PDU] fiber_switch_channel_right = [%d]\n", obj->lsw_laser_pdu_config.fiber_switch_channel_right);

    printf("[OW_MAIN] dark_frame_count = [%d]\n", obj->dark_frame_count);
    printf("[OW_MAIN] light_frame_count = [%d]\n", obj->light_frame_count);
    printf("[OW_MAIN] test_scan_frame_count = [%d]\n", obj->test_scan_frame_count);
    printf("[OW_MAIN] full_scan_frame_count = [%d]\n", obj->full_scan_frame_count);
    printf("[OW_MAIN] num_histo_captures_long_scan = [%d]\n", obj->num_histo_captures_long_scan);

    printf("[OW_MAIN] contact_thr_low_light_no_gain = [%d]\n", obj->contact_thr_low_no_gain);
    printf("[OW_MAIN] contact_thr_low_light_16_gain = [%d]\n", obj->contact_thr_low_16_gain);
    printf("[OW_MAIN] contact_thr_room_light = [%d]\n", obj->contact_thr_room_light);

    printf("[OW_MAIN] device_id = [%s]\n", obj->g_deviceID);

    for (int i = 0; i < LASER_SAFETY_SETPOINTS; i++)
    {
        printf("[OW_LSW_LASER_PDU] laser_safety_setpoint_%d = [%d]\n", i, obj->lsw_laser_pdu_config.laser_safety_setpoints[i]);
    }

    printf("[OW_MAIN] Starting MLB and PDU! \n");
#endif

    // Setup up handler for SIGKILL signal to take the app down gracefully
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sigkill_handler;
    sigaction(SIGTERM, &action, NULL);

    if(!obj->is_interactive)
    {
        memset(&deviceName[0], 0, 512);
        snprintf(&deviceName[0], 512, "/dev/ttyS5");

        // open ui comm port
        fd_serial_port = openSerialPort(&deviceName[0]);
        if (0 > fd_serial_port)
        {
            printf("[OW_MAIN] Failed to open Serial port: %s \n", deviceName);
            syslog(LOG_ERR, "[OW_MAIN] Failed to open Serial port: %s", deviceName);
            fd_serial_port = 0;
            return VX_FAILURE;
        }
        else
        {
            syslog(LOG_NOTICE, "[OW_MAIN] Opened serial port: %s", deviceName);            
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_STARTUP, CMD_SYSTEM_STATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_STARTUP], NULL);
        }
    }


    // configure output directory with device id
    memset(obj->output_file_path, 0, APP_MAX_FILE_PATH);
    sprintf(obj->output_file_path, "%s/%s", obj->left_capture.output_file_path, obj->g_deviceID);

    printf("[OW_MAIN] Creating output directory %s\n", obj->output_file_path);
    if (stat(obj->output_file_path, &st) == -1)
    {
        if (mkpath(obj->output_file_path, 0755) == -1) {            
            syslog(LOG_ERR, "[OW_MAIN] Error trying to create output directory Error: %s\n", strerror(errno));
            printf("[OW_MAIN] Some error trying to create output directory Error: %s\n", strerror(errno));

            if(!obj->is_interactive && fd_serial_port)
            {
                //TODO: send message and exit
                sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_STARTUP_CREATE_FOLDER], NULL);
                return VX_FAILURE;
            }
        }
    }
    
    memset(obj->left_capture.output_file_path, 0, APP_MAX_FILE_PATH);
    strcpy(obj->left_capture.output_file_path, obj->output_file_path);
    memset(obj->right_capture.output_file_path, 0, APP_MAX_FILE_PATH);
    strcpy(obj->right_capture.output_file_path, obj->output_file_path);
    memset(obj->left_histo.output_file_path, 0, APP_MAX_FILE_PATH);
    strcpy(obj->left_histo.output_file_path, obj->output_file_path);
    memset(obj->right_histo.output_file_path, 0, APP_MAX_FILE_PATH);
    strcpy(obj->right_histo.output_file_path, obj->output_file_path);
    
#if ENABLE_VERBOSE
    printf("[OW_MAIN] Created directory\n");
    printf("[OW_MAIN] Device ID: %s\n", obj->g_deviceID);
    printf("[OW_MAIN] Patient DIR: %s\n", obj->g_patient_path);
    printf("[OW_MAIN] output path: %s\n", obj->output_file_path);
    printf("[OW_MAIN] left capture obj path: %s\n", obj->left_capture.output_file_path);
    printf("[OW_MAIN] right capture obj path: %s\n", obj->right_capture.output_file_path);
    printf("[OW_MAIN] left histo obj path: %s\n", obj->left_histo.output_file_path);
    printf("[OW_MAIN] right histo obj path: %s\n", obj->right_histo.output_file_path);
#endif

    
    if(obj->en_ignore_error_state)
    {
        syslog(LOG_NOTICE, "[OW_MAIN] Application set for IGNORING all Laser Errors");
    }

    if(!obj->is_interactive && fd_serial_port)
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_STARTUP, CMD_SYSTEM_STATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_STARTUP_SENSORS], NULL);
    }

    if(!lsw_laser_pdu_startup(&obj->lsw_laser_pdu_config))
    {
        // this is really a failure to start the thread that starts the laser
        printf("[OW_MAIN] Failed lsw_laser_pdu_config MSG: %s\n", error_msg());
        syslog(LOG_ERR, "[OW_MAIN] Failed lsw_laser_pdu_config MSG: %s", error_msg());

        if(!obj->is_interactive && fd_serial_port)
        {
            //TODO: send message and exit
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_STARTUP_SEED], NULL);
            return VX_FAILURE;
        }
    }
    else
    {
        printf("[OW_MAIN] Waiting for laser to warmup\n");
        syslog(LOG_NOTICE, "[OW_MAIN] Waiting for laser to warmup");
        while (current_state() != OW_STATE_IDLE && current_state() != OW_STATE_ERROR) // wait for warmup to finish or error to occur 
        {
            if (lsw_remaining_startup_seconds() % 5 == 0)
            {
                char s_warmup[64] = {0};
                sprintf(s_warmup, "%s, %d seconds remaining", OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_STARTUP_WARMUP], lsw_remaining_startup_seconds());
                printf("[OW_MAIN] %s\n", s_warmup);
                if(!obj->is_interactive && fd_serial_port)
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_STARTUP, CMD_SYSTEM_STATE, NULL, s_warmup, NULL);
                }
                sleep(1);
            }
        }
    }

    if(!obj->is_interactive && fd_serial_port)
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_STARTUP, CMD_SYSTEM_STATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_STARTUP_INIT], NULL);
    }

    if(current_state() == OW_STATE_ESTOP)
    {
        syslog(LOG_ERR, "[OW_MAIN::Startup] E-Stop detected");
        return VX_FAILURE;
    }
    
    if(obj->en_ignore_error_state)
    {
        // ensuring we clear any laser errors before continuing
        if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
        {
            // check if it is an estop issue
        }
    }

    /* Querry sensor parameters */
    // left sensor config
    obj->left_sensor.sensor_index = 7;
    obj->left_sensor.enable_ldc = 0;
    obj->left_sensor.num_cameras_enabled = 1;
    obj->left_sensor.usecase_option = APP_SENSOR_FEATURE_CFG_UC0;
    status = app_querry_sensor(&obj->left_sensor);
    if(status != VX_SUCCESS){
        syslog(LOG_ERR, "[OW_MAIN] left sensor app_query_sensor Failed");
        if(!obj->is_interactive && fd_serial_port)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_STARTUP_QUERY], NULL);
            return VX_FAILURE;
        }        
    }

    // right sensor config
    obj->right_sensor.sensor_index = 7;
    obj->right_sensor.enable_ldc = 0;
    obj->right_sensor.num_cameras_enabled = 1;
    obj->right_sensor.usecase_option = APP_SENSOR_FEATURE_CFG_UC0;
    status = app_querry_sensor(&obj->right_sensor);
    if(status != VX_SUCCESS){
        syslog(LOG_ERR, "[OW_MAIN] right sensor app_query_sensor Failed");
        if(!obj->is_interactive && fd_serial_port)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_STARTUP_QUERY], NULL);
            return VX_FAILURE;
        }        
    }
    
    if (obj->input_streaming_type == INPUT_STREAM_FILE)
    {
        syslog(LOG_ERR, "[OW_MAIN] stream from file not supported");
    }

    /* Update of parameters */
    app_update_param_set(obj);

    /* Register App Remote Service */

#ifdef ENABLE_VERBOSE
    printf("[OW_MAIN] Registering Service \n");
#endif
    status = appRemoteServiceRegister((char*)HEAD_SENSOR_SERVICE_NAME, appRemoteServiceHeadSensorHandler);
    if(status!=0)
    {
        printf("[OW_MAIN] appRemoteServiceRegister: ERROR: Unable to register remote service Head Sensor handler\n");
        syslog(LOG_ERR, "[OW_MAIN] appRemoteServiceRegister: ERROR: Unable to register remote service Head Sensor handler");
        if(!obj->is_interactive && fd_serial_port)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_STARTUP_RPMSG], NULL);
            return VX_FAILURE;
        }
    }

    if(current_state() != OW_STATE_IDLE){
        printf("[OW_MAIN] Error state should be IDLE before going into interactive or headless mode SHUTTING DOWN\n");
        syslog(LOG_ERR, "Error state should be IDLE before going into interactive or headless mode SHUTTING DOWN");
        if(!obj->is_interactive && fd_serial_port)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE], NULL);
            return VX_FAILURE;
        }
    }
    else
    {
        if(obj->is_interactive)
        {
            syslog(LOG_NOTICE, "[OW_MAIN] Starting in interactive mode!");
            status = app_run_graph_interactive(obj);
        }
        else
        {
            syslog(LOG_NOTICE, "[OW_MAIN] Starting in headless mode!");
            status = app_run_graph_uart_mode(obj);
        }
    }

    // should be in idle state here

    if(!lsw_laser_pdu_shutdown())  // TODO: this will never fail always returns true
    {
        if(!obj->en_ignore_error_state)
        {
            printf("[OW_MAIN] Failed lsw_laser_pdu_shutdown MSG: %s", error_msg());
            syslog(LOG_ERR, "[OW_MAIN] Failed lsw_laser_pdu_shutdown MSG: %s", error_msg());

            if(!obj->is_interactive && fd_serial_port)
            {
                //TODO: send error and exit
            }
        }
    }

    /* UnRegister APP Remote Service */
#ifdef ENABLE_VERBOSE
    printf("[OW_MAIN] UNRegistering Service \n");
#endif

    if(appRemoteServiceUnRegister((char*)HEAD_SENSOR_SERVICE_NAME)!=0)
    {
        printf("[OW_MAIN] appRemoteServiceUnRegister: ERROR: Unable to un-register remote service Head Sensor handler\n");
        syslog(LOG_ERR, "[OW_MAIN] appRemoteServiceUnRegister: ERROR: Unable to un-register remote service Head Sensor handler");
    }

    if(!obj->is_interactive && fd_serial_port)
    {
        printf("[OW_MAIN] Closing Serial Port\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_STATE, OW_STATE_SHUTDOWN, CMD_SYSTEM_STATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SHUTDOWN_COMPLETE], NULL);            
        sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, OW_STATE_SHUTDOWN, CMD_SYSTEM_SHUTDOWN, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SHUTDOWN_COMPLETE], NULL);                
        closeSerialPort(fd_serial_port);
        syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_uart_mode] Closed serial port: %s", deviceName);
    }

    // Clean up the Python interpreter
    printf("[OW_MAIN] clean up python\n");
    pymodule_lib_cleanup();
    
    // exiting application no need to check if it passes or not
    change_state(OW_STATE_SHUTDOWN, OW_UNDEFINED);

    printf ("[OW_MAIN] App Openwater application EXIT\n");
    syslog(LOG_NOTICE, "[OW_MAIN] Application Exit");
    closelog ();
    return status;
}

static void add_graph_parameter_by_node_index(vx_graph graph, vx_node node, vx_uint32 node_parameter_index)
{
    vx_parameter parameter = vxGetParameterByIndex(node, node_parameter_index);

    vxAddParameterToGraph(graph, parameter);
    vxReleaseParameter(&parameter);
}

static void app_pipeline_params_defaults(AppObj *obj)
{
    obj->pipeline       = -APP_BUFFER_Q_DEPTH + 1;
    obj->enqueueCnt     = 0;
    obj->dequeueCnt     = 0;
}

static void set_sensor_defaults(SensorObj *sensorObj)
{
    strcpy(sensorObj->sensor_name, SENSOR_HIMAX5530_UB953_D3);

    sensorObj->num_sensors_found = 0;
    sensorObj->sensor_features_enabled = 0;
    sensorObj->sensor_features_supported = 0;
    sensorObj->sensor_dcc_enabled = 0;
    sensorObj->sensor_wdr_enabled = 0;
    sensorObj->sensor_exp_control_enabled = 0;
    sensorObj->sensor_gain_control_enabled = 0;
}

static vx_status app_run_graph(AppObj *obj, uint64_t phys_ptr, void *virt_ptr)
{
    vx_status status = VX_SUCCESS;

    SensorObj *left_sensor = &obj->left_sensor;
    SensorObj *right_sensor = &obj->right_sensor;
    vx_int32 frame_id;

    uint32_t pair_mask = 0;

    app_pipeline_params_defaults(obj);

    if(NULL == left_sensor->sensor_name)
    {
        printf("[OW_MAIN] left sensor name is NULL \n");
        return VX_FAILURE;
    }

    if(NULL == right_sensor->sensor_name)
    {
        printf("[OW_MAIN] right sensor name is NULL \n");
        return VX_FAILURE;
    }

    /* set to pair 1 at startup */
    status |= app_switch_camera_pair(obj, 1, 0);

    for(frame_id = 0; ; frame_id++)
    {        
        // every five frames, update the cam_temp_vals
        if (frame_id % 10 == 0) {
            if (obj->currentActivePair > 0 && obj->currentActivePair < 5) {
                pair_mask = app_get_pair_mask(obj, obj->currentActivePair, obj->currentActiveScanType);
                status=VX_SUCCESS;
                status = appGetTempCameraPair(pair_mask, phys_ptr, virt_ptr);
                if(status==VX_SUCCESS) 
                {
                    cam_temp_lower_output = ((CameraTempReadings *)virt_ptr)->cam_temp_lower;
                    cam_temp_upper_output = ((CameraTempReadings *)virt_ptr)->cam_temp_upper;
                    // printf("TASK MASK: 0x%02X MASK: 0x%02X TEMPs: %04X  %04X\n", pair_mask, (uint8_t)((CameraTempReadings *)virt_ptr)->mask, ((CameraTempReadings *)virt_ptr)->cam_temp_upper, ((CameraTempReadings *)virt_ptr)->cam_temp_lower);                     
                }
                else
                {
                    printf("Could not read cam temp values\n");
                }
                status=VX_SUCCESS; //don't want the rest of the pipeline fail because it can't read temps
            }
        }
    
        if(obj->write_file == 1)
        {
            printf("[OW_MAIN] write file start \n");
            uint32_t enableChanBitFlag = 0x3F; // for 3 channels
            struct stat st = {0};
            /* D3: Create output directory if it doesn't already exists should remove this don't need to do it every time */
            printf("[OW_MAIN] checking for directory for left capture: %s!\n", obj->left_capture.output_file_path);
            if (stat(obj->left_capture.output_file_path, &st) == -1)
            {
                printf("[OW_MAIN] Creating directory for right %s!\n", obj->left_capture.output_file_path);
                mkdir(obj->left_capture.output_file_path, 0777);
            }
            printf("[OW_MAIN] checking direcotry for right capture: %s!\n", obj->right_capture.output_file_path);
            if (stat(obj->right_capture.output_file_path, &st) == -1)
            {
                printf("[OW_MAIN] Creating directory for right %s!\n", obj->right_capture.output_file_path);
                mkdir(obj->right_capture.output_file_path, 0777);
            }
            
            if ((obj->left_capture.en_out_capture_write == 1) && (status == VX_SUCCESS))
            {
                printf("[OW_MAIN] app_send_cmd_cap_write_node PatientPath: %s SubPath: %s \n", obj->g_patient_path, obj->g_sub_path);
                status = app_send_cmd_capture_write_node(&obj->left_capture, enableChanBitFlag, frame_id + 1, obj->num_frames_to_write, obj->num_frames_to_skip,  obj->scan_type, obj->g_patient_path, obj->g_sub_path);
            }

            if ((obj->right_capture.en_out_capture_write == 1) && (status == VX_SUCCESS))
            {
                printf("[OW_MAIN] app_send_cmd_cap_write_node PatientPath: %s SubPath: %s \n", obj->g_patient_path, obj->g_sub_path);
                status = app_send_cmd_capture_write_node(&obj->right_capture, enableChanBitFlag, frame_id + 1, obj->num_frames_to_write, obj->num_frames_to_skip,  obj->scan_type, obj->g_patient_path, obj->g_sub_path);
            }
            
            if(status != VX_SUCCESS)
            {
                printf("[OW_MAIN] Error sending capture write node command ERROR: %d \n", status);
            }

            obj->write_file = 0;
            printf("[OW_MAIN] write file finished \n");
        }


        if(obj->write_histo == 1)
        {
            printf("[OW_MAIN] write histo start \n");
            uint32_t enableChanBitFlag = 0x3F; // for 3 channels
            status = app_send_cmd_histo_write_node(&obj->left_histo, enableChanBitFlag, frame_id+1, obj->num_histo_frames_to_write, obj->num_histo_frames_to_skip, obj->currentActivePair, obj->scan_type, obj->g_patient_path, obj->g_sub_path, virt_ptr); 
            status |= app_send_cmd_histo_write_node(&obj->right_histo, enableChanBitFlag, frame_id+1, obj->num_histo_frames_to_write, obj->num_histo_frames_to_skip, obj->currentActivePair, obj->scan_type, obj->g_patient_path, obj->g_sub_path, virt_ptr);

            obj->write_histo = 0;
            printf("[OW_MAIN] write histo finished \n");
        }

        if (status == VX_SUCCESS)
        {
            status = app_run_graph_for_one_frame_pipeline(obj, frame_id);
        }

        /* user asked to stop processing */
        if(obj->stop_task)
          break;
    }

    if (status == VX_SUCCESS)
    {
        status = vxWaitGraph(obj->graph);
    }
    obj->stop_task = 1;

    if (status == VX_SUCCESS)
    {
        // pair 0 to shut off
        status |= app_switch_camera_pair(obj, 0, 0);
    }

    return status;
}

static vx_status app_run_graph_for_one_frame_pipeline(AppObj *obj, vx_int32 frame_id)
{
    vx_status status = VX_SUCCESS;
    appPerfPointBegin(&obj->total_perf);

    /* This is the number of frames required for the pipeline AWB and AE algorithms to stabilize
        (note that 15 is only required for the 6-8 camera use cases - others converge quicker) */

    if(obj->pipeline < 0)
    {

        /* Enqueue inputs during pipeup dont execute */
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_capture.graph_parameter_index, (vx_reference *)&obj->left_capture.raw_image_arr[obj->enqueueCnt], 1);            
        }
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_distribution_index, (vx_reference*)&obj->left_histo.outHistoCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_mean_index, (vx_reference*)&obj->left_histo.outMeanCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_sd_index, (vx_reference*)&obj->left_histo.outSdCh0[obj->enqueueCnt], 1);
        }

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_capture.graph_parameter_index, (vx_reference *)&obj->right_capture.raw_image_arr[obj->enqueueCnt], 1);            
        }
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_distribution_index, (vx_reference*)&obj->right_histo.outHistoCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_mean_index, (vx_reference*)&obj->right_histo.outMeanCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_sd_index, (vx_reference*)&obj->right_histo.outSdCh0[obj->enqueueCnt], 1);
        }

        obj->enqueueCnt++;
        obj->enqueueCnt   = (obj->enqueueCnt  >= APP_BUFFER_Q_DEPTH)? 0 : obj->enqueueCnt;
        obj->pipeline++;
    }

    if((obj->pipeline == 0) && (status == VX_SUCCESS))
    {
        /* Execute 1st frame */
        if(status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_capture.graph_parameter_index, (vx_reference *)&obj->left_capture.raw_image_arr[obj->enqueueCnt], 1);
        }
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_distribution_index, (vx_reference*)&obj->left_histo.outHistoCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_mean_index, (vx_reference*)&obj->left_histo.outMeanCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_sd_index, (vx_reference*)&obj->left_histo.outSdCh0[obj->enqueueCnt], 1);
        }

        if(status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_capture.graph_parameter_index, (vx_reference *)&obj->right_capture.raw_image_arr[obj->enqueueCnt], 1);
        }
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_distribution_index, (vx_reference*)&obj->right_histo.outHistoCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_mean_index, (vx_reference*)&obj->right_histo.outMeanCh0[obj->enqueueCnt], 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_sd_index, (vx_reference*)&obj->right_histo.outSdCh0[obj->enqueueCnt], 1);
        }
        
        obj->enqueueCnt++;
        obj->enqueueCnt   = (obj->enqueueCnt  >= APP_BUFFER_Q_DEPTH)? 0 : obj->enqueueCnt;
        obj->pipeline++;
    }

    if((obj->pipeline > 0) && (status == VX_SUCCESS))
    {
        vx_object_array captured_left_frames;
        vx_object_array captured_right_frames;
        vx_distribution output_left_histo;
        vx_distribution output_right_histo;
        vx_scalar output_left_mean;
        vx_scalar output_right_mean;
        vx_scalar output_left_sd;
        vx_scalar output_right_sd;

        uint32_t num_refs;

        /* Dequeue input */

        status = vxGraphParameterDequeueDoneRef(obj->graph, obj->left_capture.graph_parameter_index, (vx_reference *)&captured_left_frames, 1, &num_refs);

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterDequeueDoneRef(obj->graph, obj->left_histo.graph_parameter_distribution_index, (vx_reference *)&output_left_histo, 1, &num_refs);
            status |= vxGraphParameterDequeueDoneRef(obj->graph, obj->left_histo.graph_parameter_mean_index, (vx_reference *)&output_left_mean, 1, &num_refs);
            status |= vxGraphParameterDequeueDoneRef(obj->graph, obj->left_histo.graph_parameter_sd_index, (vx_reference *)&output_left_sd, 1, &num_refs);
        }

        if (status != VX_SUCCESS)
        {
            printf("left vxGraphParameterDequeueDoneRef failed\n");
        }

        status = vxGraphParameterDequeueDoneRef(obj->graph, obj->right_capture.graph_parameter_index, (vx_reference *)&captured_right_frames, 1, &num_refs);

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterDequeueDoneRef(obj->graph, obj->right_histo.graph_parameter_distribution_index, (vx_reference *)&output_right_histo, 1, &num_refs);
            status |= vxGraphParameterDequeueDoneRef(obj->graph, obj->right_histo.graph_parameter_mean_index, (vx_reference *)&output_right_mean, 1, &num_refs);
            status |= vxGraphParameterDequeueDoneRef(obj->graph, obj->right_histo.graph_parameter_sd_index, (vx_reference *)&output_right_sd, 1, &num_refs);
        }

        if (status != VX_SUCCESS)
        {
            printf("right vxGraphParameterDequeueDoneRef failed\n");
        }

        /* Enqueue input - start execution */
        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_capture.graph_parameter_index, (vx_reference *)&captured_left_frames, 1);
        }

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_distribution_index, (vx_reference*)&output_left_histo, 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_mean_index, (vx_reference*)&output_left_mean, 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->left_histo.graph_parameter_sd_index, (vx_reference*)&output_left_sd, 1);
        }
        
        if (status != VX_SUCCESS)
        {
            printf("left vxGraphParameterEnqueueReadyRef failed\n");
            syslog(LOG_ERR, "[OW_MAIN::app_run_graph_for_one_frame_pipeline] left vxGraphParameterEnqueueReadyRef failed");
        }

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_capture.graph_parameter_index, (vx_reference *)&captured_right_frames, 1);
        }

        if (status == VX_SUCCESS)
        {
            status = vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_distribution_index, (vx_reference*)&output_right_histo, 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_mean_index, (vx_reference*)&output_right_mean, 1);
            status |= vxGraphParameterEnqueueReadyRef(obj->graph, obj->right_histo.graph_parameter_sd_index, (vx_reference*)&output_right_sd, 1);
        }
        
        if (status != VX_SUCCESS)
        {
            printf("right vxGraphParameterEnqueueReadyRef failed\n");
            syslog(LOG_ERR, "[OW_MAIN::app_run_graph_for_one_frame_pipeline] left vxGraphParameterEnqueueReadyRef failed");
        }

        obj->enqueueCnt++;
        obj->dequeueCnt++;

        obj->enqueueCnt = (obj->enqueueCnt >= APP_BUFFER_Q_DEPTH)? 0 : obj->enqueueCnt;
        obj->dequeueCnt = (obj->dequeueCnt >= APP_BUFFER_Q_DEPTH)? 0 : obj->dequeueCnt;
    }

    appPerfPointEnd(&obj->total_perf);
    return status;
}

static void app_run_task(void *app_var)
{
    AppObj *obj = (AppObj *)app_var;
    volatile vx_status status = VX_SUCCESS;
    CameraTempReadings *virt_ptr;
    uint64_t phys_ptr;
    uint32_t mem_size = sizeof(CameraTempReadings);

    // virt_ptr = appMemAlloc(APP_MEM_HEAP_DDR, mem_size, 32);
    virt_ptr = (CameraTempReadings *)appMemAlloc(APP_MEM_HEAP_DDR, mem_size, 32);

    if(NULL == virt_ptr)
    {
        printf("appGetTempCameraPair: failed invalid memory pointer!!!\n");
        obj->stop_task_done = 1;
        return;
    }

    // Assign values to the structure members
    virt_ptr->mask = 0x00; 
    virt_ptr->cam_temp_lower = 0; 
    virt_ptr->cam_temp_upper = 0;

    phys_ptr = appMemGetVirt2PhyBufPtr((uint64_t)(uintptr_t)virt_ptr, APP_MEM_HEAP_DDR);
    
    // printf("MAIN Temp PHYS_PTR: %p\n", (void*)phys_ptr);
    while((obj->stop_task == 0) && (status == VX_SUCCESS))
    {
        status = app_run_graph(obj, phys_ptr, virt_ptr);
    }

    appMemFree(APP_MEM_HEAP_DDR, virt_ptr, mem_size);

    obj->stop_task_done = 1;
}

static int32_t app_run_task_create(AppObj *obj)
{
    tivx_task_create_params_t params;
    vx_status status;

    // create and run task
    tivxTaskSetDefaultCreateParams(&params);
    params.task_main = app_run_task;
    params.app_var = obj;

    obj->stop_task_done = 0;
    obj->stop_task = 0;

    printf("+++++++++++++> CREATE TASK\n");
    status = tivxTaskCreate(&obj->task, &params);

    return status;
}

static void app_run_task_delete(AppObj *obj)
{
    obj->stop_task = 1;
    printf("-------------> DELETE TASK\n");
    // delete task
    while(obj->stop_task_done==0)
    {
        tivxTaskWaitMsecs(100);
    }

    tivxTaskDelete(&obj->task);
}

static inline const char* modeToStr(D3_UTILS_I2C_TRANSACTION_TYPE type) {
    switch(type) {
        case TRANSACTION_TYPE_8A8D:
            return "8A8D  ";
        case TRANSACTION_TYPE_8A16D:
            return "8A16D ";
        case TRANSACTION_TYPE_16A8D:
            return "16A8D ";
        case TRANSACTION_TYPE_16A16D:
            return "16A16D";
        default:
            return "ERR   ";
    }
}

static uint32_t __readLine(uint32_t maxlen, char *buf) {
    int i = 0;
    char ch = 0;

    if(buf == NULL || maxlen <= 0) {
        return 0;
    }

    buf[0] = '\0'; // start with a null string

    for(i = 0; i < maxlen; i++) {
        ch = getchar();
        buf[i] = ch;
        if(buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
            break;
        }
    }

    return strlen(buf);
}

static void run_d3_utils_menu()
{
    D3UtilsI2C_Command *cmd = malloc(sizeof(D3UtilsI2C_Command));
    cmd->type = D3_I2C_READ;
    cmd->regAddr = 0;
    cmd->regData = 0;
    cmd->devAddr = 0;
    uint8_t done = 0;
    char ch;
    char inputBuffer[8];

    while(done == 0) {
        printf(utils_menu0);
        printf(
            " MODE:%s Device: ADDR:0x%02X, REG:0x%04X, Data:0x%04X\n",
            modeToStr(cmd->type),
            cmd->devAddr,
            cmd->regAddr,
            cmd->regData
        );
        printf(" Enter Choice: \n");
        do {
            ch = getchar();
        } while(ch == '\n' || ch == '\r');
        getchar(); // Eat the newline.
        switch(ch) {
            case 'i':
            case '1':
                printf("READ.\n");
                cmd->direction = D3_I2C_READ;
                appD3I2CMenu(cmd);
                break;
            case 'o':
            case '2':
                printf("WRITE.\n");
                cmd->direction = D3_I2C_WRITE;
                appD3I2CMenu(cmd);
                break;
            case '3':
                printf(utils_menu1);
                ch = getchar();
                switch(ch) {
                    case '0':
                        cmd->type = TRANSACTION_TYPE_8A8D;
                        break;
                    case '1':
                        cmd->type = TRANSACTION_TYPE_8A16D;
                        break;
                    case '2':
                        cmd->type = TRANSACTION_TYPE_16A8D;
                        break;
                    case '3':
                        cmd->type = TRANSACTION_TYPE_16A16D;
                        break;
                    default:
                        printf("Invalid type selected.\n");
                        break;
                }
                break;
            case '4':
                printf("Enter Device Address (7b Hex) and press Enter: ");
                __readLine(8, inputBuffer);
                cmd->devAddr = ((uint8_t) strtol(inputBuffer, NULL, 16)) & 0x7FU;
                break;
            case '5':
            case 'r':
                printf("Enter Register Address (Hex) and press Enter: ");
                __readLine(8, inputBuffer);
                cmd->regAddr = ((uint16_t) strtol(inputBuffer, NULL, 16));
                switch(cmd->type) {
                    case TRANSACTION_TYPE_8A16D:
                    case TRANSACTION_TYPE_8A8D:
                        cmd->regAddr &= 0xFFU;
                        break;
                    default:
                        break;
                }
                break;
            case '6':
            case 'd':
                printf("Enter Data (Hex) and press Enter: ");
                __readLine(8, inputBuffer);
                cmd->regData = ((uint16_t) strtol(inputBuffer, NULL, 16));
                switch(cmd->type) {
                    case TRANSACTION_TYPE_8A16D:
                    case TRANSACTION_TYPE_8A8D:
                        cmd->regData &= 0xFFU;
                        break;
                    default:
                        break;
                }
                break;
            case 'x':
                done = 1;
                break;
            default:
                printf("Unrecognized command.\n");
                break;
        }
    }
}

static bool int_token(vx_char *token, const char *tok_name, int *val)
{
    vx_char s[]=" \n";
    if(strcmp(token, tok_name) == 0)
    {
        token = strtok(NULL, s);
        if(token != NULL)
        {
            *val = atoi(token);
            return true;
        }
    }
    return false;
}

#define GET_NUM_TOKEN(tok_name, val) \
if (int_token(token, tok_name, &tok_val))\
{\
    val = tok_val;\
    continue;\
}

#define GET_BIT_TOKEN(tok_name, val) \
if (int_token(token, tok_name, &tok_val))\
{\
    if (tok_val > 1) tok_val = 1;\
    val = tok_val;\
    continue;\
}

#define GET_STR_TOKEN(tok_name, val) \
if(strcmp(token, tok_name) == 0)\
{\
    token = strtok(NULL, s);\
    if(token != NULL)\
    {\
        token[strlen(token)-1] = 0;\
        strcpy(val, token);\
    }\
    continue;\
}\

static int count_bits_enabled(uint8_t register_val) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if ((register_val & (1 << i)) != 0) {
            count++;
        }
    }
    return count;
}

static bool test_half_bytes(uint8_t value) {
    int lower_count = 0, upper_count = 0;
    // Count set bits in lower half
    for (int i = 0; i < 4; i++) {
        if ((value & (1 << i)) != 0) {
            lower_count++;
        }
    }
    // Count set bits in upper half
    for (int i = 4; i < 8; i++) {
        if ((value & (1 << i)) != 0) {
            upper_count++;
        }
    }
    // Return true if both counts are less than or equal to 2
    return (lower_count <= 3) && (upper_count <= 3);
}

static void app_parse_cfg_file(AppObj *obj, vx_char *cfg_file_name)
{
    printf("[OW_MAIN] Parse config file [%s]\n", cfg_file_name);
    FILE *fp = fopen(cfg_file_name, "r");
    vx_char line_str[1024];
    vx_char *token;

    obj->left_sensor.ch_mask = 0;

    if(fp==NULL)
    {
        printf("[OW_MAIN] # ERROR: Unable to open config file [%s]\n", cfg_file_name);
        syslog(LOG_ERR, "[OW_MAIN::app_parse_cfg_file] Unable to open config file %s", cfg_file_name);
        exit(0);
    }

    while(fgets(line_str, sizeof(line_str), fp)!=NULL)
    {
        int tok_val;
        vx_char s[]=" \t";

        if (strchr(line_str, '#'))
        {
            continue;
        }

        /* get the first token */
        token = strtok(line_str, s);
        if(token != NULL)
        {
            // printf("[OW_MAIN] Process Token [%s]\n", token);
            GET_NUM_TOKEN("sensor_index", obj->left_sensor.sensor_index)
            GET_NUM_TOKEN("num_frames_to_run", obj->num_frames_to_run)
            GET_BIT_TOKEN("enable_error_detection", obj->left_capture.enable_error_detection)
            GET_BIT_TOKEN("enable_ldc", obj->left_sensor.enable_ldc)
            GET_BIT_TOKEN("en_ignore_graph_errors", obj->en_ignore_graph_errors)
            GET_BIT_TOKEN("en_out_capture_write", obj->left_capture.en_out_capture_write)
            GET_BIT_TOKEN("is_interactive", obj->is_interactive)
            GET_BIT_TOKEN("ignore_error_state", obj->en_ignore_error_state)
            GET_NUM_TOKEN("usecase_option", obj->left_sensor.usecase_option)
            GET_NUM_TOKEN("num_frames_to_write", obj->num_frames_to_write)
            GET_NUM_TOKEN("num_frames_to_skip", obj->num_frames_to_skip)
            GET_NUM_TOKEN("frame_sync_period", obj->frame_sync_period)
            GET_NUM_TOKEN("frame_sync_duration", obj->frame_sync_duration)
            GET_NUM_TOKEN("strobe_delay_start", obj->strobe_delay_start)
            GET_NUM_TOKEN("strobe_duration", obj->strobe_duration)
            GET_NUM_TOKEN("timer_resolution", obj->timer_resolution)
            GET_NUM_TOKEN("num_histo_bins", obj->num_histo_bins)
            GET_STR_TOKEN("i2c_device", obj->lsw_laser_pdu_config.i2c_device)
            GET_STR_TOKEN("gpio_device", obj->lsw_laser_pdu_config.gpio_device)
            GET_NUM_TOKEN("seed_warmup_time_seconds", obj->lsw_laser_pdu_config.seed_warmup_time_seconds)
            GET_NUM_TOKEN("seed_preset", obj->lsw_laser_pdu_config.seed_preset)
            GET_NUM_TOKEN("ta_preset", obj->lsw_laser_pdu_config.ta_preset)
            GET_NUM_TOKEN("adc_ref_volt_mv", obj->lsw_laser_pdu_config.adc_ref_volt_mv)
            GET_NUM_TOKEN("dac_ref_volt_mv", obj->lsw_laser_pdu_config.dac_ref_volt_mv)
            GET_NUM_TOKEN("fiber_switch_channel_left", obj->lsw_laser_pdu_config.fiber_switch_channel_left)
            GET_NUM_TOKEN("fiber_switch_channel_right", obj->lsw_laser_pdu_config.fiber_switch_channel_right)
            GET_NUM_TOKEN("dark_frame_count", obj->dark_frame_count)
            GET_NUM_TOKEN("light_frame_count", obj->light_frame_count)
            GET_NUM_TOKEN("test_scan_frame_count", obj->test_scan_frame_count)
            GET_NUM_TOKEN("full_scan_frame_count", obj->full_scan_frame_count)
            GET_NUM_TOKEN("num_histo_frames_to_write", obj->num_histo_frames_to_write)
            GET_NUM_TOKEN("num_histo_frames_to_skip", obj->num_histo_frames_to_skip)
            GET_NUM_TOKEN("contact_thr_low_light_no_gain", obj->contact_thr_low_no_gain)
            GET_NUM_TOKEN("contact_thr_low_light_16_gain", obj->contact_thr_low_16_gain)
            GET_NUM_TOKEN("contact_thr_room_light", obj->contact_thr_room_light)
            GET_STR_TOKEN("device_id", obj->g_deviceID)
            GET_STR_TOKEN("archive_file_path", obj->archive_file_path)
            GET_NUM_TOKEN("num_histo_captures_long_scan", obj->num_histo_captures_long_scan)

            if(strcmp(token, "output_file_path") == 0)
            {
                token = strtok(NULL, s);
                if(token != NULL)
                {
                    token[strlen(token)-1]=0;
                    strcpy(obj->left_capture.output_file_path, token);
                    strcpy(obj->left_histo.output_file_path, token);
                    strcpy(obj->output_file_path, token);
                }
                continue;
            }

            if(strcmp(token, "camera_config_test") == 0)
            {
                GET_NUM_TOKEN("camera_config_test", obj->camera_config_test)     
            }

            if(strcmp(token, "camera_config_long") == 0)
            {
                GET_NUM_TOKEN("camera_config_long", obj->camera_config_long)     
            }
            
            if(strcmp(token, "camera_set_1") == 0)
            {
                GET_NUM_TOKEN("camera_set_1", obj->camera_config_set[0])     
            }
            
            if(strcmp(token, "camera_set_2") == 0)
            {
                GET_NUM_TOKEN("camera_set_2", obj->camera_config_set[1])
            }
                        
            if(strcmp(token, "camera_set_3") == 0)
            {
                GET_NUM_TOKEN("camera_set_3", obj->camera_config_set[2])
            }
                        
            if(strcmp(token, "camera_set_4") == 0)
            {
                GET_NUM_TOKEN("camera_set_4", obj->camera_config_set[3])
            }

            if(strcmp(token, "test_mode") == 0)
            {
                token = strtok(NULL, s);
                if(token != NULL)
                {
                    obj->test_mode = atoi(token);
                    obj->left_capture.test_mode = atoi(token);
                }
                continue;
            }

            if(strcmp(token, "input_streaming_type") == 0)
            {
                token = strtok(NULL, s);
                if(token != NULL)
                {
                    token[strlen(token)-1]=0;
                    obj->input_streaming_type= atoi(token);
                    if ((obj->input_streaming_type != INPUT_STREAM_SERDES))
                    {
                        printf("[OW_MAIN] File config parser error: Invalid vision_input_streaming_type= %d\n", obj->input_streaming_type);
                        syslog(LOG_ERR, "[OW_MAIN::app_parse_cfg_file] File config parser error: Invalid vision_input_streaming_type= %d",  obj->input_streaming_type);
                    }
                }
                continue;
            }

            if(strncmp(token, "laser_safety_setpoint_", strlen(token) - 1) == 0)
            {
                int i = token[strlen(token) - 1] - '0';
                token = strtok(NULL, s);
                if(token != NULL && i >= 0 && i < LASER_SAFETY_SETPOINTS)
                {
                    obj->lsw_laser_pdu_config.laser_safety_setpoints[i] = atoi(token);
                }
                continue;
            }

        }
    }

    obj->left_sensor.ch_mask = 0; // Initialize channel mask
    obj->left_sensor.num_cameras_enabled = 0; // Initialize count of enabled cameras
    obj->right_sensor.ch_mask = 0; // Initialize channel mask
    obj->right_sensor.num_cameras_enabled = 0; // Initialize count of enabled cameras

    // calculate channel mask and number of cameras enabled
    for (int cam_check = 0; cam_check < 4; cam_check++){
        if(!test_half_bytes(obj->camera_config_set[cam_check])){
            printf("Camera SET %d has an invalid configuration 0x%02x\n", cam_check + 1, obj->camera_config_set[cam_check]);
            obj->camera_config_set[cam_check] = 0x00; // set it to disabled
        }
        obj->left_sensor.ch_mask |= (uint8_t)(obj->camera_config_set[cam_check] & 0x0F);
        obj->right_sensor.ch_mask |= (uint8_t)(obj->camera_config_set[cam_check] & 0xF0);
    }

    obj->left_sensor.num_cameras_enabled = count_bits_enabled((uint8_t)obj->left_sensor.ch_mask);
    obj->right_sensor.num_cameras_enabled = count_bits_enabled((uint8_t)(obj->right_sensor.ch_mask >> 4));

    fclose(fp);

}

static void app_set_cfg_default(AppObj *obj)
{
    memset(obj->g_deviceID, 0, 24);
    strcpy(obj->g_deviceID, "device000");

    snprintf(obj->left_capture.output_file_path,APP_MAX_FILE_PATH, ".");

    obj->left_capture.en_out_capture_write = 1;
    obj->right_capture.en_out_capture_write = 1;
    obj->input_streaming_type= INPUT_STREAM_SERDES;

    obj->num_frames_to_write = 0;
    obj->num_frames_to_skip = 0;

    obj->num_histo_frames_to_write = 0;
    obj->num_histo_frames_to_skip = 0;

    obj->camera_config_set[0] = 0x11;
    obj->camera_config_set[1] = 0x22;
    obj->camera_config_set[2] = 0x44;
    obj->camera_config_set[3] = 0x11;
}

static void app_parse_cmd_line_args(AppObj *obj, vx_int32 argc, vx_char *argv[])
{
    vx_int32 i;
    app_set_cfg_default(obj);

    if(argc==1)
    {
        app_show_usage(argc, argv);
        exit(0);
    }

    for(i=0; i<argc; i++)
    {
        if(strcmp(argv[i], "--cfg") == 0)
        {
            i++;
            if(i>=argc)
            {
                app_show_usage(argc, argv);
            }
            app_parse_cfg_file(obj, argv[i]);
        }
        else
        if(strcmp(argv[i], "--help") == 0)
        {
            app_show_usage(argc, argv);
            exit(0);
        }
    }


    return;
}

static vx_status app_init(AppObj *obj)
{
    vx_status status = VX_SUCCESS;

    AppD3boardInitCmdParams cmdPrms;

    cmdPrms.moduleId = APP_REMOTE_SERVICE_D3BOARD_FRAMESYNC;
    cmdPrms.fSyncPeriod = obj->frame_sync_period;
    cmdPrms.fSyncDuration = obj->frame_sync_duration;
    cmdPrms.strobeLightDelayStart = obj->strobe_delay_start;
    cmdPrms.strobeLightDuration = obj->strobe_duration;
    cmdPrms.timerResolution = obj->timer_resolution;

    /* Create OpenVx Context */
    obj->context = vxCreateContext();
    status = vxGetStatus((vx_reference)obj->context);
    APP_PRINTF("Creating context done!\n");
    if (status == VX_SUCCESS)
    {
        tivxHwaLoadKernels(obj->context);
        tivxImagingLoadKernels(obj->context);
        tivxFileIOLoadKernels(obj->context);
        APP_PRINTF("Kernel loading done!\n");
    }
    else
    {
        syslog(LOG_ERR, "[OW_MAIN::app_init] Error creating context");
        APP_PRINTF("Error creating context!\n");
    }

    /* Initialize modules */

    if (status == VX_SUCCESS)
    {
        tivxOwAlgosLoadKernels(obj->context);
        if(status == VX_SUCCESS)
        {
            APP_PRINTF("tivxOwAlgosLoadKernels done!\n");
        }
        else
        {
            syslog(LOG_ERR, "[OW_MAIN::app_init] Error loading c66 Kernels");
            APP_PRINTF("Error loading c66 Kernels!\n");
        }
    }
    
    // TODO: Determine which personality board here
    int32_t d3board_init_status = appInitD3board(&cmdPrms);
    if (0 != d3board_init_status)
    {
        /* Not returning failure because application may be waiting for
        error/test frame */
        syslog(LOG_ERR, "[OW_MAIN::app_init] Error initializing D3 board");
        printf("[OW_MAIN] Error initializing D3 board\n");
        status = VX_FAILURE;
    }

    return status;
}

static void app_deinit(AppObj *obj)
{
    appDeInitD3board();

    printf("deinit left\n");
    app_deinit_histo(&obj->left_histo, APP_BUFFER_Q_DEPTH);
    app_deinit_custom_capture(&obj->left_capture, APP_BUFFER_Q_DEPTH);
    app_deinit_sensor(&obj->left_sensor);
    
    printf("deinit right\n");
    app_deinit_histo(&obj->right_histo, APP_BUFFER_Q_DEPTH);
    app_deinit_custom_capture(&obj->right_capture,APP_BUFFER_Q_DEPTH);
    app_deinit_sensor(&obj->right_sensor);

    tivxHwaUnLoadKernels(obj->context);
    tivxImagingUnLoadKernels(obj->context);
    tivxFileIOUnLoadKernels(obj->context);
    tivxOwAlgosUnLoadKernels(obj->context);
    printf("Kernels unload done!\n");

    vxReleaseContext(&obj->context);
    printf("Release context done!\n");
}

static vx_status app_create_graph(AppObj *obj)
{
    vx_status status = VX_SUCCESS;
    vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[9];
    vx_int32 graph_parameter_index = 0;

    appPerfPointSetName(&obj->total_perf , "TOTAL");
    appPerfPointSetName(&obj->fileio_perf, "FILEIO");

    obj->graph = vxCreateGraph(obj->context);
    status = vxGetStatus((vx_reference)obj->graph);
    if (status == VX_SUCCESS)
    {
        status = vxSetReferenceName((vx_reference)obj->graph, "app_openwater_graph");
        APP_PRINTF("Graph create done!\n");
    }

    // left side of the graph
    if (status == VX_SUCCESS)
    {
        status = app_init_sensor(&obj->left_sensor, "left_sensor");
        APP_PRINTF("left sensor init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_init_custom_capture(obj->context, &obj->left_capture, &obj->left_sensor, "left_capture", APP_BUFFER_Q_DEPTH, CAPT1_INST_ID);
        APP_PRINTF("left capture init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_create_custom_graph_capture(obj->graph, &obj->left_capture, CAPT1_INST_ID);
        APP_PRINTF("left graph capture create!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_init_histo(obj->context, &obj->left_histo, &obj->left_sensor, "left_histo", APP_BUFFER_Q_DEPTH, 1024, NUM_CHANNELS, CAPT1_INST_ID);
        APP_PRINTF("left histo init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_create_graph_histo(obj->graph, &obj->left_histo, obj->left_capture.raw_image_arr[0], CAPT1_INST_ID);
        APP_PRINTF("left graph histo create done!\n");
    }

    if (status == VX_SUCCESS)
    {
        add_graph_parameter_by_node_index(obj->graph, obj->left_capture.node, 1);
        obj->left_capture.graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference *)&obj->left_capture.raw_image_arr[0];
        graph_parameter_index++;
    }
    
    // right side of the graph
    if (status == VX_SUCCESS)
    {
        status = app_init_sensor(&obj->right_sensor, "right_sensor");
        APP_PRINTF("right sensor init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_init_custom_capture(obj->context, &obj->right_capture, &obj->right_sensor, "right_capture", APP_BUFFER_Q_DEPTH, CAPT2_INST_ID);
        APP_PRINTF("right capture init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_create_custom_graph_capture(obj->graph, &obj->right_capture, CAPT2_INST_ID);
        APP_PRINTF("right graph capture create!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_init_histo(obj->context, &obj->right_histo, &obj->right_sensor, "left_histo", APP_BUFFER_Q_DEPTH, 1024, NUM_CHANNELS, CAPT2_INST_ID);
        APP_PRINTF("right histo init done!\n");
    }

    if (status == VX_SUCCESS)
    {
        status = app_create_graph_histo(obj->graph, &obj->right_histo, obj->right_capture.raw_image_arr[0], CAPT2_INST_ID);
        APP_PRINTF("right graph histo create done!\n");
    }

    if (status == VX_SUCCESS)
    {
        add_graph_parameter_by_node_index(obj->graph, obj->right_capture.node, 1);
        obj->right_capture.graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference *)&obj->right_capture.raw_image_arr[0];
        graph_parameter_index++;
    }

    // add left histo params
    if (status == VX_SUCCESS)
    {
        APP_PRINTF("left histo params\n");
        add_graph_parameter_by_node_index(obj->graph, obj->left_histo.node, TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX);
        obj->left_histo.graph_parameter_distribution_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&obj->left_histo.outHistoCh0[0];
        graph_parameter_index++;

        add_graph_parameter_by_node_index(obj->graph, obj->left_histo.node, TIVX_KERNEL_OW_CALC_HISTO_MEAN_IDX);
        obj->left_histo.graph_parameter_mean_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&obj->left_histo.outMeanCh0[0];
        graph_parameter_index++;

        add_graph_parameter_by_node_index(obj->graph, obj->left_histo.node, TIVX_KERNEL_OW_CALC_HISTO_SD_IDX);
        obj->left_histo.graph_parameter_sd_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference *)&obj->left_histo.outSdCh0[0];
        graph_parameter_index++;        
    }

    // add right histo params
    if(status == VX_SUCCESS)
    {
        APP_PRINTF("right histo params\n");
        add_graph_parameter_by_node_index(obj->graph, obj->right_histo.node, TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX);
        obj->right_histo.graph_parameter_distribution_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&obj->right_histo.outHistoCh0[0];
        graph_parameter_index++;

        add_graph_parameter_by_node_index(obj->graph, obj->right_histo.node, TIVX_KERNEL_OW_CALC_HISTO_MEAN_IDX);
        obj->right_histo.graph_parameter_mean_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&obj->right_histo.outMeanCh0[0];
        graph_parameter_index++;

        add_graph_parameter_by_node_index(obj->graph, obj->right_histo.node, TIVX_KERNEL_OW_CALC_HISTO_SD_IDX);
        obj->right_histo.graph_parameter_sd_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = APP_BUFFER_Q_DEPTH;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference *)&obj->right_histo.outSdCh0[0];
        graph_parameter_index++;
    }

    if(status == VX_SUCCESS)
    {

        status = vxSetGraphScheduleConfig(obj->graph,
                VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO,
                graph_parameter_index,
                graph_parameters_queue_params_list);

    }
    return status;
}

static vx_status app_verify_graph(AppObj *obj)
{
    vx_status status = VX_SUCCESS;

    status = vxVerifyGraph(obj->graph);

    if(status == VX_SUCCESS)
    {
        APP_PRINTF("Graph verify done!\n");
    }

#if 1
    if(VX_SUCCESS == status)
    {
      status = tivxExportGraphToDot(obj->graph,".", "app_openwater");
    }
#endif

    if ((obj->left_capture.enable_error_detection) && (status == VX_SUCCESS))
    {
        status = app_send_error_frame(&obj->left_capture);
    }

    if ((obj->right_capture.enable_error_detection) && (status == VX_SUCCESS))
    {
        status = app_send_error_frame(&obj->right_capture);
    }

    /* wait a while for prints to flush */
    tivxTaskWaitMsecs(250);

    return status;
}

static bool set_patient_id(AppObj *obj, const char *patientID)
{
    if (current_state() != OW_STATE_IDLE)
    {
        printf("[OW_MAIN::set_patient_id] set_patient_id invalid state STATE: %s\n", OW_SYSTEM_STATE_STRING[current_state()]);
        syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Command not allowed in current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);           
        return false;
    }

    if(!change_state(OW_STATE_BUSY, OW_STATE_IDLE))
    {
        printf("[OW_MAIN::set_patient_id] Failed to change state to STATE: %s FROM STATE: %s\n", OW_SYSTEM_STATE_STRING[OW_STATE_BUSY], OW_SYSTEM_STATE_STRING[OW_STATE_IDLE]);
        syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Failed to change state current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE_RECOVERABLE], NULL);             
        return false;
    }
    else
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_CREATE_PATIENT_STORE], NULL);  
    }

    char pid[25] = {0};
    struct stat st = {0};
    char patientPath[756] = {0};
    int i=0;
    
    while(patientID[i]){
        if(i == 24)
            break;

        if (isalnum(patientID[i])==0) 
            pid[i]='_';
        else
            pid[i]=patientID[i];
        i++;
    }

    time_t current_time;
	/* Obtain current time. */
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        // error
        printf("[OW_MAIN::set_patient_id] Failure to obtain the current time.\n");
        syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Failure to obtain the current time");
        change_state(OW_STATE_IDLE, OW_UNDEFINED);
        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SYSTEM_TIME], NULL);  
        return false;
    }

    memset(obj->g_patient_path,0,64);
    struct tm tm = *localtime(&current_time);    
    sprintf(obj->g_patient_path, "%04d_%02d_%02d_%02d%02d%02d_%s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, pid);    
    sprintf((char*)patientPath, "%s/%s", obj->left_capture.output_file_path, obj->g_patient_path);
    printf("[OW_MAIN::set_patient_id] Directory Path for data: %s\n", patientPath);

    if (stat((char*)patientPath, &st) == -1)
    {
        if (mkdir((char*)patientPath, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
            printf("Error: %s\n", strerror(errno));
            syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Error trying to create patient directory Error: %s\n", strerror(errno));
            printf("[OW_MAIN::set_patient_id] Some error trying to create patient directory Error: %s\n", strerror(errno));
            
            change_state(OW_STATE_IDLE, OW_UNDEFINED);
            
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, strerror(errno), OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PATIENT_CREATE], NULL);  
            return false;
        }
    }

    // add version info to patient data collection directory
    char sVersionFilename[900] = {0};
    sprintf((char *)sVersionFilename, "%s/%s", patientPath, "version.txt");
    FILE *fp = fopen(sVersionFilename, "w+");
    if (fp == NULL)
    {
        syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Error writing version file %s", sVersionFilename);
        change_state(OW_STATE_IDLE, OW_UNDEFINED);
        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, strerror(errno), OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PATIENT_CREATE], NULL);  
        return false;
    }

    fprintf(fp, "Openwater Version INFO:\n");
    fprintf(fp, "Build Date: %s\n", GIT_BUILD_DATE);
    fprintf(fp, "Git HASH: %s\n", GIT_BUILD_HASH);
    fprintf(fp, "Version: %s\n", GIT_BUILD_VERSION);
    fprintf(fp, "DEVICEID: %s\n", obj->g_deviceID);
    fclose(fp);

    if (change_state(OW_STATE_READY, OW_UNDEFINED)) 
    {        
        syslog(LOG_INFO, "[OW_MAIN::set_patient_id] Created Patient folder: %s Current State: %s", (char*)patientPath, OW_SYSTEM_STATE_STRING[current_state()]);
        printf("[OW_MAIN] Created Patient folder: %s Current State: %s", (char*)patientPath, OW_SYSTEM_STATE_STRING[current_state()]);
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_CREATE_PATIENT_SUCCESS], NULL);  
        return true;
    }

    syslog(LOG_ERR, "[OW_MAIN::set_patient_id] Failed to change sate\n");
    printf("[OW_MAIN::set_patient_id] failed to change state patientPath: %s Current State: %s", (char*)patientPath, OW_SYSTEM_STATE_STRING[current_state()]);
    change_state(OW_STATE_IDLE, OW_UNDEFINED);
    sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_UNEXPECTED], NULL);  
    return false;
}

static bool set_cam_pair(AppObj *obj, int pair)
{
    if(current_state() != OW_STATE_READY){
        syslog(LOG_ERR, "[OW_MAIN::set_cam_pair] Must be in ready state to switch cameras, current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
        printf("[OW_MAIN::set_cam_pair] Must be in ready state to switch cameras, current state: %s\n", OW_SYSTEM_STATE_STRING[current_state()]);        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_SET_CAMERA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);  
        return false;
    }

    if(pair < 1 || pair > 4)
    {
        // error
        printf("[OW_MAIN] Selected camera pair must be in the range of 1-4");        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_SET_CAMERA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_INVALID_CAMERA], NULL);  
        return false;
    }
    else
    {
        if(app_switch_camera_pair(obj, pair, 0)==0)
        {
            // success
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_SET_CAMERA, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SWITCH_CAMERA_SUCCESS], NULL);  
        }
        else
        {
            printf("[OW_MAIN::set_cam_pair] %s\n", OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_CAMERA]);
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_SET_CAMERA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_CAMERA], NULL);  
            return false;
        }
    }

    return true;
}

static bool owWriteImageToFile(AppObj *obj, uint32_t cap_frames, uint32_t skip_frames, bool enableGatekeeper, uint32_t timeout)
{
    uint32_t timeout_count = 0;
    bool bSuccess = true;

    obj->num_frames_to_write = cap_frames;
    obj->num_frames_to_skip = skip_frames;
    if(enableGatekeeper) obj->wait_file = 1;
    obj->write_file = 1;

    while(obj->write_file != 0)
    {
        usleep(1000);
        if( timeout_count >= (timeout*1000))
        {
            // error
            printf("[OW_MAIN::owWriteImageToFile] Timed out during frame capture to disk\n");
            syslog(LOG_ERR, "[OW_MAIN::owWriteImageToFile] Timed out during frame capture to disk");
            bSuccess = false;
            break;
        }

        timeout_count++;
    }

    if(enableGatekeeper){

        timeout_count = 0;
        while(obj->wait_file != 0)
        {
            usleep(1000);
            if( timeout_count >= (timeout*1000))
            {
                // error
                printf("[GateKeeper] Timed out during wait for file\n");
                syslog(LOG_ERR, "[GateKeeper] Timed out during wait for file");
                bSuccess = false;
                break;
            }

            timeout_count++;
        }

    }

    if(obj->wait_err == 1)
    {
        // error was thrown during write to file
        printf("[GateKeeper] Error writing to file: %s\n", obj->wait_msg);
        syslog(LOG_ERR, "[GateKeeper] Error writing to file: %s", obj->wait_msg);
    }


    return bSuccess;
}

static bool owWriteHistoToFile(AppObj *obj, uint32_t hist_frames, uint32_t skip_frames, bool enableGatekeeper, uint32_t timeout)
{
    uint32_t timeout_count = 0;
    bool bSuccess = true;

    obj->num_histo_frames_to_write = hist_frames;  // assuming 25fps and 1sec capture  (capture seconds * framerate)
    obj->num_frames_to_skip = skip_frames;

    if(enableGatekeeper) obj->wait_histo = 1;
    obj->write_histo = 1;

    while(obj->write_histo != 0)
    {
        usleep(1000);
        if( timeout_count >= (timeout*1000))
        {
            // error
            printf("Timed out during frame histogram capture to disk\n");
            syslog(LOG_ERR, "Timed out during frame histogram capture to disk");
            bSuccess = false;
            break;
        }

        timeout_count++;
    }

    if(enableGatekeeper){
        timeout_count = 0;
        while(obj->wait_histo != 0)
        {
            usleep(1000);
            if( timeout_count >= (timeout*1000))
            {
                // error
                printf("[GateKeeper] Timed out during wait for histo\n");
                syslog(LOG_ERR, "[GateKeeper] Timed out during wait for histo");
                bSuccess = false;
                break;
            }

            timeout_count++;

        }
    }

    if(obj->wait_err == 1)
    {
        // error was thrown during write of histogram data
        printf("[GateKeeper] Error writing histogram: %s\n", obj->wait_msg);
        syslog(LOG_ERR, "[GateKeeper] Error writing histogram: %s", obj->wait_msg);
    }

    return bSuccess;

}

static bool run_scan_sequence(AppObj *obj, uint32_t dark_frames, uint32_t light_frames, uint32_t histo_frames, int32_t scan_type, uint32_t camera_pair)
{
    bool bFailed = false;
    printf("enter run sequence\n");

    // set scan type
    obj->scan_type = scan_type;

    // dark capture
    // ensure laser is off (camera pair selected before running test scan)
    printf("turn laser off\n");
    appDisableLaser();
    
    if(scan_type==0 || scan_type == 2) // full scan 
    {
        syslog(LOG_NOTICE, "full scan write %d dark frames", dark_frames);
        if(!owWriteImageToFile(obj, dark_frames, 0, true, 60)){
            // error
            printf("[OW_MAIN] Failure writing dark frames.\n");
            syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing dark frames");
            bFailed = true;
            return bFailed;
        }
    }
    else
    {
        //Test scan
        syslog(LOG_NOTICE, "test scan write 2 dark frames");
        if(!owWriteImageToFile(obj, 2, 0, true, 60)){
            // error
            printf("[OW_MAIN] Failure writing dark frames.\n");
            syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing dark frames");
            bFailed = true;
            return bFailed;
        }

        //Test the sensor for ambient light
        if( imageMean[0]!=UINT32_MAX && imageMean[1]!=UINT32_MAX  && imageMean[2]!=UINT32_MAX  && imageMean[3]!=UINT32_MAX )
        {
            printf("[OW_MAIN] Checking ambient light for pair %u, mean ch0 is %u, mean ch1 is %u, mean ch2 is %u, mean ch3 is %u  and the threshold is %u\n",
                    camera_pair, imageMean[0], imageMean[1], imageMean[2], imageMean[3], obj->contact_thr_room_light);
            if( imageMean[0]>=obj->contact_thr_room_light )
                goodSensorContact[0] |= 1;
            if( imageMean[1]>=obj->contact_thr_room_light )
                goodSensorContact[1] |= 1;
            if( imageMean[2]>=obj->contact_thr_room_light )
                goodSensorContact[2] |= 1;
            if( imageMean[3]>=obj->contact_thr_room_light )
                goodSensorContact[3] |= 1;
        }
        imageMean[0] = UINT32_MAX; imageMean[1] = UINT32_MAX; imageMean[2] = UINT32_MAX; imageMean[3] = UINT32_MAX;
    }

    // light capture
    // ensure laser is on (camera pair selected before running test scan)
    printf("turn laser on\n");
    appEnableLaser();
    usleep(100000);
    
    if(scan_type==0 || scan_type == 2) // full scan 
    {
        syslog(LOG_NOTICE, "full scan write %d light frames", light_frames);
        if(!bFailed && !owWriteImageToFile(obj, light_frames, 0, true, 60)){
            // error
            printf("[OW_MAIN] Failure writing light frames.\n");
            syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing light frames");
            bFailed = true;
            return bFailed;
        }
    }
    else
    {
        syslog(LOG_NOTICE, "test scan write 2 light frames");
        if(!bFailed && !owWriteImageToFile(obj, 2, 0, true, 60)){
            // error
            printf("[OW_MAIN] Failure writing light frames.\n");
            syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing light frames");
            bFailed = true;
            return bFailed;
        }

    }

    syslog(LOG_NOTICE, "write histo frames");
    if(!bFailed && !owWriteHistoToFile(obj, histo_frames, 0, true, 60))
    {
        // error
        printf("[OW_MAIN] Failure writing histogram frames.\n");
        syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing histogram frames");
        bFailed = true;
        return bFailed;
    }
    
    if(scan_type==1) //Test scan - check for laser light
    {
        uint32_t laserLightThresholdHorizontal = obj->contact_thr_low_16_gain;
        uint32_t laserLightThresholdNear = obj->contact_thr_low_no_gain;
        if( imageMean[0]!=UINT32_MAX && imageMean[1]!=UINT32_MAX  && imageMean[2]!=UINT32_MAX  && imageMean[3]!=UINT32_MAX )
        {
            printf("[OW_MAIN] Checking ambient light for pair %u, mean ch0 is %u, mean ch1 is %u, mean ch2 is %u, mean ch3 is %u  and the threshold is %u (ch0,ch2) %u (ch1,ch3)\n",
                    camera_pair, imageMean[0], imageMean[1], imageMean[2], imageMean[3], laserLightThresholdHorizontal, laserLightThresholdNear );
            if( imageMean[0]<laserLightThresholdHorizontal )
                goodSensorContact[0] |= 2;
            if( imageMean[1]<laserLightThresholdHorizontal )
                goodSensorContact[1] |= 2;
            if( imageMean[2]<laserLightThresholdHorizontal )
                goodSensorContact[2] |= 2;
            if( imageMean[3]<laserLightThresholdHorizontal )
                goodSensorContact[3] |= 2;
        }
        imageMean[0] = UINT32_MAX; imageMean[1] = UINT32_MAX; imageMean[2] = UINT32_MAX; imageMean[3] = UINT32_MAX;
    }

    // ensure laser is off
    printf("turn laser off\n");
    appDisableLaser();
    usleep(100000);

    syslog(LOG_NOTICE, "write 2 dark histo frames");
    obj->scan_type = 3;  // set to dark histo scan type
    if(!bFailed && !owWriteHistoToFile(obj, 2, 0, true, 10))
    {
        // error
        printf("[OW_MAIN] Failure writing histogram frames.\n");
        syslog(LOG_ERR, "[OW_MAIN::run_scan_sequence] Failure writing dark histogram frames");
        bFailed = true;
        return bFailed;
    }

    // set scan type back just in case
    obj->scan_type = scan_type;
    
    printf("exit run sequence\n");
    return bFailed;
}

static bool perform_scan(AppObj *obj, uint32_t num_frames, int32_t scan_type)
{
    struct stat st = {0};
    char current_scan_path[1024] = {0};
    memset(current_scan_path, 0, 1024);

    /* Obtain current time. */
    time_t now = time(NULL);
    if (now == -1)
    {
        // error
        printf("[OW_MAIN] Failure to obtain the current time.\n");
        syslog(LOG_ERR, "[OW_MAIN::perform_scan] Failure to obtain the current time");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SYSTEM_TIME], NULL);
        return false;
    }

    syslog(LOG_NOTICE, "[OW_MAIN::perform_scan] CURRENT STATE: %s", OW_SYSTEM_STATE_STRING[current_state()]);

    if (current_state() != OW_STATE_READY)
    {        
        syslog(LOG_ERR, "[OW_MAIN::perform_scan] Failed, scan not allowed in current state");
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        change_state(OW_STATE_READY, OW_UNDEFINED);
        return false;
    }

    if(!change_state(OW_STATE_BUSY, OW_STATE_READY))
    {
        printf("[OW_MAIN::perform_scan] Failed to change state to STATE: %s FROM STATE: %s\n", OW_SYSTEM_STATE_STRING[OW_STATE_BUSY], OW_SYSTEM_STATE_STRING[OW_STATE_IDLE]);
        syslog(LOG_ERR, "[OW_MAIN::perform_scan] Failed to change state current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE_RECOVERABLE], NULL);             
        change_state(OW_STATE_READY, OW_UNDEFINED);
        return false;
    }

    syslog(LOG_NOTICE, "[OW_MAIN::perform_scan] CURRENT STATE: %s", OW_SYSTEM_STATE_STRING[current_state()]);

    // create scan directory
    memset(obj->g_sub_path,0,32);
    struct tm *ptm = localtime(&now);
    char scan_type_string[12] = {0};
    switch(scan_type){
        case 1: // test scan
            strcpy(scan_type_string, "TESTSCAN_6C");
            break;
        case 2: // long scan
            strcpy(scan_type_string, "LONGSCAN_6C");
            break;
        case 0: // full scan
        default:
            strcpy(scan_type_string, "FULLSCAN_6C");
            break;
    }
    printf("[OW_MAIN] Current TIME: %02d/%02d/%04d %02d:%02d:%02d\n", ptm->tm_mon + 1,  ptm->tm_mday, ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    snprintf((char*)obj->g_sub_path, 31, "%.11s_%04d_%02d_%02d_%02d%02d%02d", scan_type_string, ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    sprintf(current_scan_path, "%s/%s/%s", obj->left_capture.output_file_path, obj->g_patient_path, obj->g_sub_path);
    if (stat(current_scan_path, &st) == -1)
    {
        if (mkdir(current_scan_path, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
            printf("Error: %s\n", strerror(errno));
            syslog(LOG_ERR, "[OW_MAIN::perform_scan] Error trying to create patient directory Error: %s\n", strerror(errno));
            printf("[OW_MAIN] Some error trying to create patient directory Error: %s\n", strerror(errno));
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PATIENT_CREATE], NULL);
            change_state(OW_STATE_READY, OW_UNDEFINED);
            return false;
        }
    }
    
    syslog(LOG_NOTICE, "[OW_MAIN::perform_scan] CURRENT STATE: %s CURRENT SCAN PATH: %s", OW_SYSTEM_STATE_STRING[current_state()], current_scan_path);

    if(scan_type==1) //Test scan
    {
        for(uint32_t i=0; i<8; ++i)
            goodSensorContact[i] = 0;
        imageMean[0] = 0; imageMean[1] = 0; imageMean[2] = 0; imageMean[3] = 0;
    }

    for(int pair_count = 0; pair_count < 4; pair_count++)
    {
        if((scan_type==1 || scan_type==2) && pair_count > 0)
            break; // finished only doing pair 0 for long and test scan skip the remaining pairs

        // check E-Stop
        if(current_state() == OW_STATE_ESTOP)
        {
            // abort current process
            syslog(LOG_ERR, "[OW_MAIN::perform_scan] E-Stop detected aborting scan process");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_ESTOP], NULL);
            return false;
        }

        if(app_get_pair_mask(obj, pair_count + 1, scan_type) == 0x00) // camera setting is disabled in config with 0
        {
            printf("Camera config %d disabled in configuration file\n", pair_count + 1);
            syslog(LOG_WARNING, "[OW_MAIN::perform_scan] Camera config %d disabled in configuration file", pair_count + 1);
            continue;
        }

        // set camera pair
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SWITCH_CAMERA_PAIR1 + pair_count], NULL);
        if(app_switch_camera_pair(obj, pair_count + 1, scan_type) < 0) 
        {
            // error
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_PAIR1 + pair_count], NULL);
            change_state(OW_STATE_READY, OW_UNDEFINED);
            return false;
        }

        usleep(100000);
        
        if(scan_type==2){ // iff a long scan
            
            printf("[OW_MAIN] Scanning in LONG mode, TOTAL CAPTURES: %d\n", obj->num_histo_captures_long_scan);

            for(uint32_t scan_round =0;scan_round < obj->num_histo_captures_long_scan;scan_round++){
                printf("[OW_MAIN] Scanning in LONG mode, round %d\n", scan_round);
                
                memset(s_generic_message, 0, sizeof(s_generic_message));
                sprintf(s_generic_message, "Scanning... (%d/%d)", scan_round+1,obj->num_histo_captures_long_scan);

                sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SCAN_PATIENT, NULL, s_generic_message, NULL);        
                if(run_scan_sequence(obj, obj->dark_frame_count, obj->light_frame_count, num_frames, scan_type, scan_round))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SCAN], NULL);
                    change_state(OW_STATE_READY, OW_UNDEFINED);
                    return false;
                }
                
                if(current_state() == OW_STATE_ESTOP)
                {
                    // abort current process
                    syslog(LOG_ERR, "[OW_MAIN::perform_scan] E-Stop detected aborting scan process");
                    sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_ESTOP], NULL);
                    return false;
                }
            }
        }
        else {

            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SCAN_PAIR1 + pair_count], NULL);
            if(run_scan_sequence(obj, obj->dark_frame_count, obj->light_frame_count, num_frames, scan_type, pair_count))
            {
                sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SCAN], NULL);
                change_state(OW_STATE_READY, OW_UNDEFINED);
                return false;
            }
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SUCCESS_PAIR1 + pair_count], NULL);
        }

    }

    if(scan_type==1) //Test scan
    {
        char s_contact_data[64] = {0};
        sprintf(s_contact_data, "[%d,%d,%d,%d]", goodSensorContact[1], goodSensorContact[3], goodSensorContact[0],  goodSensorContact[2]);
        printf("!!!!!!!!Contact data: %s",s_contact_data);
        
        sendResponseMessage(fd_serial_port, OW_CONTENT_DATA, current_state(), CMD_SCAN_PATIENT, scan_type?"test":"full", OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SCAN_COMPLETE], s_contact_data);            
    }

    if(!change_state(OW_STATE_READY, OW_UNDEFINED))
    {
        syslog(LOG_ERR, "[OW_MAIN::perform_scan] Failed, scan was not able to set back to ready state after scan");
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SCAN], NULL);
        return false;
    }
    
    return true;
}

static bool set_patient_note(AppObj *obj, const char* patientNote)
{
    if (current_state() == OW_STATE_READY)
    {
        char patientNoteFilePath[1024];
        memset(patientNoteFilePath, 0, 1024);
        sprintf(patientNoteFilePath, "%s/%s/%s/note.txt", obj->left_capture.output_file_path, obj->g_patient_path, obj->g_sub_path);
        printf("[OW_MAIN] Note File: %s\n", patientNoteFilePath);

        FILE *fp = fopen(patientNoteFilePath, "w+");
        if(fp)
        {
            fprintf(fp, "%s\n", patientNote);
            fclose(fp);
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SAVE_PATIENT_NOTE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SAVENOTE_COMPLETE], NULL);
            return true;
        }
        else
        {
            // error            
            printf("Failed to open file for writing: %s\n", patientNoteFilePath);
            syslog(LOG_ERR, "[OW_MAIN::set_patient_note] Failed to open file for writing: %s", patientNoteFilePath);
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SAVE_PATIENT_NOTE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PATIENT_NOTES], NULL);
        }
    }
    else
    {
        // error
        printf("[OW_MAIN::set_patient_note] Save note not allowed in current state: %s\n", OW_SYSTEM_STATE_STRING[current_state()]);
        syslog(LOG_ERR, "[OW_MAIN::set_patient_note] Save note not allowed in current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SAVE_PATIENT_NOTE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
    }

    return false;
}

static bool backup(int fd, AppObj *obj)
{
    struct stat st = {0};
    struct dirent *entry;
    DIR *dp;
    int iRet = 0;
    int iErrorFlag = 0;
    TAR *pTar;
    char full_path[576] = {0};
    char src_path[256] = {0};
    char arch_path[256] = {0};
    char dest_path[256] = {0};
    char new_archive_path[576] = {0};
    char fixed_name[288] = {0};
    char command[1512] = {0};
    if (!(current_state() == OW_STATE_IDLE || current_state() == OW_STATE_ESTOP))
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        return false;
    }

    if (!find_usb_stick())
    {
        syslog(LOG_ERR, "[OW_MAIN::backup] No USB stick found\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_USB_NOTFOUND], NULL);
        return false;
    }

    // Get output path as source path and get archive path for destination of compressed scans and finally get the usb path    
    sprintf(src_path, "%s", obj->output_file_path);
    sprintf(arch_path, "%s", obj->archive_file_path);
    sprintf(dest_path, "%s/data_scans", usbPath);

    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_BACKUP_BEGIN], NULL);
    
    // check if source data path exists if not error
    printf("Checking Source PATH: %s\r\n", src_path);
    if (stat(src_path, &st) == -1) {
        printf("Source PATH NOT FOUND: %s\r\n", src_path);
        syslog(LOG_ERR, "[OW_MAIN::backup] Failed to find source path\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_DATA_NOTFOUND], NULL);
        return false;
    }

    printf("Checking Destination PATH: %s\r\n", dest_path);
    // check if data_scans directory exists on usb and create it if it don't
    if (stat(dest_path, &st) == -1) {        
        printf("Creating Destination PATH: %s\r\n", dest_path);
        if(mkdir(dest_path, 0777)!=0){
            syslog(LOG_ERR, "[OW_MAIN::backup] Failed to create destination path\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_MKDIR_ERROR], NULL);
            return false;
        }
    }

    // check if archive path exists if not create it
    printf("Checking Archive PATH: %s\r\n", arch_path);
    if (stat(arch_path, &st) == -1) {
        printf("Creating Archive PATH: %s\r\n", arch_path);
        if(mkdir(arch_path, 0777)!=0){
            syslog(LOG_ERR, "[OW_MAIN::backup] Failed to create archive path\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_MKDIR_ERROR], NULL);
            return false;
        }
    }

    // open source directory
    printf("Opening directory PATH: %s\r\n", src_path);
    dp = opendir(src_path);
    if (dp == NULL)
    {
        syslog(LOG_ERR, "[OW_MAIN::backup] Failed to open data path\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_DATA_NOTFOUND], NULL);
        return false;
    }

    // start reading the patient scans collected in the source directory
    printf("Reading directory \r\n");
    syslog(LOG_WARNING, "[backup::scan-list]\n");
    while((entry = readdir(dp)) != NULL)
    {
        char archive_path[576] = {0};

        if(strcmp (entry->d_name, "..") == 0 || strcmp (entry->d_name, ".") == 0 ) continue;
        
        syslog(LOG_WARNING, "[backup::scan-item] %s\n", entry->d_name);
        memset(full_path, 0, 576);
        sprintf(full_path, "%s/%s", src_path, entry->d_name);

        memset(fixed_name, 0, 288);        
        sprintf(fixed_name, "%s_%s", obj->g_deviceID, replace_char(entry->d_name));

        // create tar file name
        sprintf(archive_path, "%s/%s.tar", arch_path, fixed_name);

        // open tar file for archiving scan
        iRet = tar_open(&pTar, archive_path, NULL,  O_WRONLY | O_CREAT,  0644,  TAR_GNU);
        if(iRet == 0)
        {
            printf("[backup] Starting transfer to USB stick\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP, entry->d_name, "Copying patient data", NULL); 
            iRet = tar_append_tree(pTar, full_path, entry->d_name);
            tar_close(pTar);
            if(iRet == 0)
            {
                memset(command, 0, 1512);
                sprintf(command, "gzip %s", archive_path);
                printf("[backup] run command: %s \n", command);
                if(system(command)<0){
                    printf("failed to compress file.\n");
                    syslog(LOG_ERR, "[OW_MAIN::backup] failed to compress file\n");
                    iErrorFlag = 1;
                }
                else
                {
                    // create tgz file
                    memset(new_archive_path, 0, 576);
                    sprintf(new_archive_path, "%s/%s.tgz", arch_path, fixed_name);

                    memset(command, 0, 1512);
                    sprintf(command, "mv %s.gz %s", archive_path, new_archive_path);
                    printf("[backup] run command: %s \n", command);
                    if(system(command)<0){
                        printf("failed to move file.\n");
                        syslog(LOG_ERR, "[OW_MAIN::backup] failed to move file\n");
                        iErrorFlag = 1;
                    }
                    else
                    {
                        memset(command, 0, 1512);
                        sprintf(command, "cp -f %s %s/", new_archive_path, dest_path); 
                        printf("[backup] run command: %s \n", command);
                        if(system(command)<0){
                            //error
                            printf("failed to move file.\n");
                            syslog(LOG_ERR, "[OW_MAIN::backup] failed move %s to usb drive %s\n", new_archive_path, dest_path);
                            iErrorFlag = 1;                            
                        }
                        else
                        {
                            printf("[backup] remove patient data \n");
                            if(remove_directory(full_path) < 0){
                                //error
                                printf("failed to remove source path\n");
                                syslog(LOG_ERR, "[OW_MAIN::backup] failed to remove source %s\n", full_path);
                                iErrorFlag = 1;   
                            }
                        }       
                    }
                }

            }
            else
            {
                syslog(LOG_ERR, "[OW_MAIN::backup] Failed to add files to archive\n");
                iErrorFlag = 1;
            }
        }
        else
        {
            syslog(LOG_ERR, "[OW_MAIN::backup] Failed to create archive\n");
            iErrorFlag = 1;
        }

    }

    closedir(dp);

    printf("performing file sync.\n");
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_BACKUP_SYNC], NULL);
    printf("[OW_MAIN] Finishing transfer to USB stick\n");
    system("sync");

    printf("[OW_MAIN] Unmounting USB stick \n");
    memset(command, 0, 1024);
    sprintf(command, "umount %s", usbPath);
    system(command);
    
    if(iErrorFlag == 1){
        //error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_ZIP_ARCHIVE], NULL);
        return false;
    }
    
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_BACKUP_SUCCESS], NULL);

    return true;
}

static bool perform_sys_cleanup(AppObj *obj, const char* srcPath)
{
    struct stat st = {0};
    char src_path[512] = {0};
    char command[1024] = {0};
    char* result = NULL;
    
    if (!(current_state() == OW_STATE_IDLE || current_state() == OW_STATE_ESTOP))
    {        
        syslog(LOG_ERR, "[OW_MAIN::perform_sys_cleanup] Invalid state for system cleanup to be performed");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_CLEANUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        return false;
    }

    if ((srcPath != NULL) && (srcPath[0] == '\0')) {
        // empty use default archive path
        sprintf(src_path, "%s", obj->archive_file_path);

        // check if source data path exists if not error
        printf("Checking Source PATH: %s\r\n", src_path);
        if (stat(src_path, &st) == -1) {
            printf("Source PATH NOT FOUND: %s\r\n", src_path);
            syslog(LOG_ERR, "[OW_MAIN::perform_sys_cleanup] Failed to find path, possibly need to backup data first\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_CLEANUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_BACKUPS_NOTFOUND], NULL);
            return false;
        }
        
    }
    else
    {
        // use provided path (only allowing a directory that has the device ID in it)
        sprintf(src_path, "%s", srcPath);

        // check if source data path exists if not error
        printf("Checking Source PATH: %s\r\n", src_path);
        if (stat(src_path, &st) == -1) {
            printf("Source PATH NOT FOUND: %s\r\n", src_path);
            syslog(LOG_ERR, "[OW_MAIN::perform_sys_cleanup] Failed to find path, check path provided\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_CLEANUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_DIRECTORY_NOTFOUND], NULL);
            return false;
        }

        // check if the path contains the deviceID
        result = strstr(src_path, obj->g_deviceID);

        if(!result){
            printf("Source PATH does not include device ID %s\r\n", src_path);
            syslog(LOG_ERR, "[OW_MAIN::perform_sys_cleanup] deleting of data in other than device specific storage or archive storage is not permitted\n");
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_CLEANUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_CLEANUP_INVALID], NULL);
            return false;
        }
    }

    printf("rm -rf %s/*", src_path);
    sprintf(command, "rm -rf %s/*", src_path);
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_CLEANUP, NULL, command, NULL);        
    if(system(command)<0){
        //error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_CLEANUP, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_LOG_BACKUP_FAILED], NULL);
        return false;
    }
    
    system("sync");

    return true;   
}

static bool perform_fw_update(AppObj *obj, const char* updatefs)
{
    struct stat st = {0};
    char s_archive[256] = {0};
    
    if (!(current_state() == OW_STATE_IDLE || current_state() == OW_STATE_ESTOP))
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_UPDATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        return false;
    }

    if (!find_usb_stick())
    {
        syslog(LOG_ERR, "[OW_MAIN::perform_fw_update] No USB stick found\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_UPDATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_USB_NOTFOUND], NULL);
        return false;
    }

    // set drive path 
    if(updatefs && strlen(updatefs)>0){
        sprintf(s_archive, "%s", updatefs);
        printf("[OW_MAIN] Source Directory: %s\n", s_archive);
        syslog(LOG_INFO, "[OW_MAIN::perform_fw_update] Source file: %s\n", s_archive);
    }
    else
    {
        sprintf(s_archive, "%s/ow_update.tgz", usbPath);
        printf("[OW_MAIN] Source Directory:  %s\n", s_archive);
        syslog(LOG_INFO, "[OW_MAIN::perform_fw_update] Source file: %s\n", s_archive);
    }

    // check if source directory exists
    if (stat(s_archive, &st) == -1)
    {
        memset(s_generic_message, 0, sizeof(s_generic_message));
        printf("[OW_MAIN] Source file %s  does not exist!\n", s_archive);
        syslog(LOG_ERR, "[OW_MAIN::perform_fw_update] Source file %s  does not exist!\n", s_archive);
        sprintf(s_generic_message, "Source file %s does not exist!", s_archive);
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_UPDATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_UPDATE_NOSOURCE], NULL);
        return false;
    }
    
    // let user know that we're starting an rsync (as it can take
    //  a while)
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_UPDATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_UPDATE_BEGIN], NULL);
    // performe rsync
    char tar_cmd[1024] = {0};
    snprintf(tar_cmd, 1024, "tar --no-same-owner -xf %s -C /", s_archive);
    
    printf("Run Command: %s", tar_cmd);
    int ret_tar = system(tar_cmd);
    if(ret_tar == 0)
    {
        memset(tar_cmd, 0, 1024);
        system("sync"); // sync filesystem for good measure
        printf("[OW_MAIN] Unmounting USB stick \n");
        sprintf(tar_cmd, "umount %s", usbPath);
        system(tar_cmd); 
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_UPDATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_UPDATE_SUCCESS], NULL);
    }
    else
    {
        memset(s_generic_message, 0, sizeof(s_generic_message));
        sprintf(s_generic_message, "CODE: %d\n", ret_tar);
        syslog(LOG_ERR, "[OW_MAIN::perform_fw_update] Error extracting archive: %s\n", s_generic_message);
        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_UPDATE, s_generic_message, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_UPDATE_FAILED], NULL);
        return false;
    }


    return true;
}

static bool perform_log_copy(AppObj *obj)
{
    time_t current_time;
    char s_archive[256] = {0};
    char log_cmd[1024];
    
    if (!(current_state() == OW_STATE_IDLE || current_state() == OW_STATE_ESTOP))
    {
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        return false;
    }

    if (!find_usb_stick())
    {
        syslog(LOG_ERR, "[OW_MAIN::perform_log_copy] No USB stick found\n");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_USB_NOTFOUND], NULL);
        return false;
    }

	/* Obtain current time. */
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        // error
        printf("[OW_MAIN::perform_log_copy] Failure to obtain the current time.\n");
        syslog(LOG_ERR, "[OW_MAIN::perform_log_copy] Failure to obtain the current time");
        change_state(OW_STATE_IDLE, OW_UNDEFINED);
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SYSTEM_TIME], NULL);  
        return false;
    }

    // output file and path
    struct tm tm = *localtime(&current_time);    
    sprintf(s_archive, "%s/sw_logs_%04d_%02d_%02d_%02d%02d%02d.log", usbPath, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);    
    printf("[OW_MAIN::perform_log_copy] Archived log file: %s\n", s_archive);
    
    // let user know that we're starting the log file backup
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_LOG_BACKUP_BEGIN], NULL);
    
    // perform log
    snprintf(log_cmd, 1024, "journalctl > %s", s_archive);
    
    printf("Run Command: %s", log_cmd);
    int ret_log = system(log_cmd);
    if(ret_log == 0)
    {
        memset(log_cmd, 0, 1024);
        system("sync"); // sync filesystem for good measure
        printf("[OW_MAIN] Unmounting USB stick \n");
        sprintf(log_cmd, "umount %s", usbPath);
        system(log_cmd); 
        sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_LOG_BACKUP_SUCCESS], NULL);
    }
    else
    {
        memset(s_generic_message, 0, sizeof(s_generic_message));
        sprintf(s_generic_message, "CODE: %d\n", ret_log);
        syslog(LOG_ERR, "[OW_MAIN::perform_log_copy] Error extracting archive: %s\n", s_generic_message);
        
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_BACKUP_LOGS, s_generic_message, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_LOG_BACKUP_FAILED], NULL);
        return false;
    }


    return true;
}

static bool reset_state(AppObj *obj)
{
    if (!(current_state() == OW_STATE_IDLE || current_state() == OW_STATE_ESTOP))
    {
        // cleeanup close anything in process
        printf("[OW_MAIN] IDLE Complete!\n");

        // TODO - what should the current expected state be?
        if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_SYSTEM_RESET, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE], NULL);
            return false;
        }
    }

    return true;
}

static bool retrieve_patient_data(AppObj *obj, const char* val)
{
    // float dataCH0[1204];
    // float dataCH1[1204];
    char msg[262144] = {0};
    // int z = 0;
    int pair = 0;
    //int pos = 0;
    //float interval = 0.025; // per frame time red thicker
    //float time = 0.000;
    char patientFilePath[1024];

    size_t buffer_size;  
    char camera_set_str[8];
    const char* module_name = "histo_module.PlotOnDevice";
    const char* function_name = "generate_plot";

    printf("[OW_MAIN] retrieve_patient_data for pair: %s\n", val);
    
    if (current_state() != OW_STATE_READY)
    {
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, val, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
        return false;	
	}

    if(strlen(val)<1){
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, val, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_RETRIEVE_DATA], NULL);
        return false;
    }

    // calculate pair value from message
    pair = val[0] - '0';
    if(pair < 1 || pair > 4){
        // error
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, val, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_RETRIEVE_DATA], NULL);
        return false;
    }

    // this must be the state also need to have scan completed
    //TODO: track scan completed
    if (current_state() == OW_STATE_READY)
    {

        if(!change_state(OW_STATE_BUSY, OW_STATE_READY))
        {
            printf("[OW_MAIN::retrieve_patient_data] Failed to change state to STATE: %s FROM STATE: %s\n", OW_SYSTEM_STATE_STRING[OW_STATE_BUSY], OW_SYSTEM_STATE_STRING[OW_STATE_IDLE]);
            syslog(LOG_ERR, "[OW_MAIN::retrieve_patient_data] Failed to change state current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
            // error
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE_RECOVERABLE], NULL);                         
            return false;
        }

        memset(patientFilePath, 0, 1024);
        memset(camera_set_str, 0, 8);
        sprintf(patientFilePath, "%s/%s/%s", obj->left_capture.output_file_path, obj->g_patient_path, obj->g_sub_path);
        sprintf(camera_set_str, "%d", pair);
        uint8_t* image_buffer = generate_histogram(module_name, function_name, patientFilePath, camera_set_str, &buffer_size);
        if (image_buffer != NULL) {
            printf("sending image data\n");
            // sprintf(msg, "{\"P%dC4"\":\"%s\"", pair, img_data);
            sprintf(msg, "{\"content\":\"%s\",\"state\":\"%s\",\"command\":\"%s\",\"params\":\"%s\",\"msg\":\"%s\",\"data\":",
                OW_APP_CONTENT_STRING[OW_CONTENT_DATA], OW_SYSTEM_STATE_STRING[current_state()], OW_APP_COMMAND_STRING[CMD_GET_PATIENT_DATA], val, "");
            
            // copy the first `buffer_size` bytes of `image_buffer` to the end of the message string
            snprintf(msg + strlen(msg), buffer_size, "%s", image_buffer);

            // add the closing brace and newline characters to the end of the message string
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "}}\r\n");

            sendResponseMessage2(fd_serial_port, msg);
            // uint32_t sleep_time = 2 + (buffer_size/115200);
            // printf("Sleeping for %d seconds\n", sleep_time);                    
        }
        else
        {
            // send error
            printf("[OW_MAIN::retrieve_patient_data] Failed to change state to STATE: %s FROM STATE: %s\n", OW_SYSTEM_STATE_STRING[OW_STATE_BUSY], OW_SYSTEM_STATE_STRING[OW_STATE_IDLE]);
            syslog(LOG_ERR, "[OW_MAIN::retrieve_patient_data] Failed to change state current state: %s", OW_SYSTEM_STATE_STRING[current_state()]);
            // error
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_SWITCH_STATE_RECOVERABLE], NULL);                         
            
        }


        if(image_buffer){
            change_state(OW_STATE_READY, OW_UNDEFINED);
            return true;
        }
    }
    else
    {
        // error
        printf("device is in an invalid state for this function");
        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_GET_PATIENT_DATA, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_NOT_ALLOWED], NULL);
    }
    return false;
}

static vx_status app_run_graph_uart_mode(AppObj *obj)
{
    vx_status status = VX_SUCCESS;
    struct CmdMessage* pMessage = NULL;
    memset(s_generic_message, 0, sizeof(s_generic_message));
    printf("[OW_MAIN] app_run_graph \n");

    uint32_t done = 0;

    // initialize graph and start task
    status = app_init(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_uart_mode] Failed to Initialize APP");
        if (obj->en_ignore_graph_errors == 0)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_INITIALIZATION], NULL);
            return VX_FAILURE;
        }
        else
        {
            printf("[OW_MAIN::app_run_graph_uart_mode] Failed to Initialize APP\n");
        }
    }

    printf("[OW_MAIN::app_run_graph_uart_mode] APP INIT DONE! Creating Graph\n");
    status = app_create_graph(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_uart_mode] Failed to Create Graph");
        if (obj->en_ignore_graph_errors == 0)
        {            
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_GRAPH], NULL);
            return VX_FAILURE;
        }
        else
        {
            printf("[OW_MAIN::app_run_graph_uart_mode] Failed to Create Graph\n");
        }
    }

    printf("[OW_MAIN::app_run_graph_uart_mode] App Create Graph Done! \n");
    status = app_verify_graph(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_uart_mode] Failed to Verify Graph");
        if (obj->en_ignore_graph_errors == 0)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_VERIFY], NULL);
            return VX_FAILURE;
        }
        else
        {
            printf("[OW_MAIN::app_run_graph_uart_mode] Failed to Verify Graph\n");
        }
    }

    printf("[OW_MAIN] App Verify Graph Done!\n");
    
    printf("[OW_MAIN] Creating Task\n");
    status = app_run_task_create(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_uart_mode] Failed to create RUN Task");
        if (obj->en_ignore_graph_errors == 0)
        {
            sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PROCESSING], NULL);
            return VX_FAILURE;
        }
        else
        {
            printf("[OW_MAIN::app_run_graph_uart_mode] Failed to create RUN Task\n");
        }
    }
    else
    {
        // Wait 2 sec before displaying the menu so that MCU has finished displaying its messages
        sleep(1);
    }

    // set cameras to off to not cause excessive current draw
    // pair 0 to shut off
    status = app_switch_camera_pair(obj, 0, 0);
    if(status!=0)
    {
        syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_uart_mode] Failed to stop camera streaming in headless mode!");
    }

    // power off serializer and camera
    printf("-------------> Power-off Headset\n");
    lsw_headset_poweroff();       
    if(obj->en_ignore_error_state)
    {
        // ensuring we clear any laser errors before continuing
        if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
        {
            // check if it is an estop issue
        }
    }

    // send time sync to C7
    app_send_cmd_histo_time_sync(&obj->left_histo, 0);
    app_send_cmd_histo_time_sync(&obj->right_histo, 0);

    change_state(OW_STATE_IDLE, OW_UNDEFINED);
    sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_IDLE, CMD_SYSTEM_STATE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_READY], NULL);

    while(!done)
    {
        pMessage = NULL;

        pMessage = getNextCommand(fd_serial_port);
        if(pMessage)
        {
            if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_SHUTDOWN]) == 0)
            {
                printf("[OW_MAIN] Processing %s command\n", pMessage->command);
                syslog(LOG_WARNING, "[OW_MAIN::app_run_graph_uart_mode] Shutdown application request");
                // if in ready state need to stop and de-init everything
                if(current_state() != OW_STATE_IDLE) // only can be shutdown from idle
                {
                    reset_state(obj);
                }

                // snprintf(pResponse->msg, sizeof(pResponse->msg), "Performing Shutdown from CURRENT STATE: %s", OW_SYSTEM_STATE_STRING[current_state()]);
                
                sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, OW_STATE_SHUTDOWN, CMD_SYSTEM_SHUTDOWN, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SHUTDOWN_BEGIN], NULL);
                obj->stop_task = 1;
                done = 1;
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_OPEN_PATIENT]) == 0)
            {
                // power on serializer and cameras
                // initialize serializer and cameras
                sendResponseMessage(fd_serial_port, OW_CONTENT_INFO, current_state(), CMD_OPEN_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_CREATE_PATIENT_STORE], NULL);
                
                printf("+++++++++++++>  Enable Laser\n");
                if(!ta_enable(true)){
                    printf("[OW_MAIN::CMD_OPEN_PATIENT] TA_ENABLE Failed\n");
        
                    if(!obj->en_ignore_error_state)
                    {
                        syslog(LOG_ERR,"[OW_MAIN::CMD_OPEN_PATIENT] TA_ENABLE Failed");
                        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_LASER_ENABLE], NULL);                                                
                    }
                }
                usleep(500000);

                printf("+++++++++++++> Power-on Headset\n");
                if(!lsw_headset_poweron())
                {
                    printf("[OW_MAIN::CMD_OPEN_PATIENT] Power-on Headset Failed\n");
        
                    if(!obj->en_ignore_error_state)
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_CAMERA_INIT], NULL);                                                
                    }

                }

                if(obj->en_ignore_error_state)
                {
                    status = VX_SUCCESS;
                    // ensuring we clear any laser errors before continuing
                    if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
                    {
                        // check if it is an estop issue
                    }
                }
                usleep(500000);
                
                printf("+++++++++++++> Sensor init\n");
                status = app_init_sensor(&obj->left_sensor, "left_sensor");
                status |= app_init_sensor(&obj->right_sensor, "right_sensor");
            
                if (status != VX_SUCCESS)                
                {
                    printf("[OW_MAIN::CMD_OPEN_PATIENT] failed to initialize cameras\n");
                    syslog(LOG_ERR, "[OW_MAIN::CMD_OPEN_PATIENT] failed to initialize cameras\n");
                    sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_PROCESSING], NULL);
                    change_state(OW_STATE_ERROR, OW_UNDEFINED);
                }else{

                    printf("+++++++++++++> Sensor init done!\n");

                    if(obj->en_ignore_error_state)
                    {
                        // ensuring we clear any laser errors before continuing
                        if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
                        {
                            // check if it is an estop issue
                        }
                    }

                    printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                    if(set_patient_id(obj, (const char*)pMessage->params))
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_OPEN_PATIENT, pMessage->params, pMessage->params, NULL);
                    }
                    else
                    {
                        syslog(LOG_ERR, "[OW_MAIN::open_patient] Error occurred during set patient\n");
                        // recoverable error if occurred set back to patient state
                        change_state(OW_STATE_READY, OW_UNDEFINED);
                    }
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_SET_CAMERA]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %d\n", pMessage->command, ((int)pMessage->params[0]  - (int)48 ));
                if(set_cam_pair(obj, ((int)pMessage->params[0]  - (int)48 )))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_SET_CAMERA, pMessage->params, NULL, NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::sys_set_camera] Error occurred during set camera\n");
                    // recoverable error if occurred set back to patient state
                    change_state(OW_STATE_READY, OW_UNDEFINED);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SCAN_PATIENT]) == 0) // Testscan
            {
                if(strcmp(pMessage->params, "test") == 0)
                {
                    printf("[OW_MAIN] Performing %s [test] command\n", pMessage->command);
                    if(perform_scan(obj, obj->test_scan_frame_count, 1))
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SCAN_PATIENT, pMessage->params, NULL, NULL);  
                    }
                    else
                    {
                        syslog(LOG_ERR, "[OW_MAIN::scan_patient] Error occurred during test scan of patient\n");
                        // recoverable error if occurred set back to patient state
                        change_state(OW_STATE_READY, OW_UNDEFINED);
                    }
                    
                }
                else if(strcmp(pMessage->params, "full") == 0)
                {
                    printf("[OW_MAIN] Performing %s [full] command\n", pMessage->command);
                    if(perform_scan(obj, obj->full_scan_frame_count, 0))
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SCAN_PATIENT, pMessage->params, NULL, NULL);   
                    }
                    else
                    {
                        syslog(LOG_ERR, "[OW_MAIN::scan_patient] Error occurred during full scan of patient\n");
                        // recoverable error if occurred set back to patient state
                        change_state(OW_STATE_READY, OW_UNDEFINED);
                    }
                }
                else if(strcmp(pMessage->params, "long") == 0)
                {
                    printf("[OW_MAIN] Performing %s [long] command\n", pMessage->command);
                    if(perform_scan(obj, obj->full_scan_frame_count, 2))
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SCAN_PATIENT, pMessage->params, NULL, NULL);   
                    }
                    else
                    {
                        syslog(LOG_ERR, "[OW_MAIN::scan_patient] Error occurred during long scan of patient\n");
                        // recoverable error if occurred set back to patient state
                        change_state(OW_STATE_READY, OW_UNDEFINED);
                    }
                }
                else
                {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SCAN_PATIENT, pMessage->params, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_UNEXPECTED], NULL);
                }

                // after scan stop cameras from streaming to reduce power consumption
                syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_uart_mode] stopping camera streaming in headless mode!");
                status = app_switch_camera_pair(obj, 0, 0);
                if(status!=0)
                {
                    syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_uart_mode] Failed to stop camera streaming in headless mode!");
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_CLOSE_PATIENT]) == 0)
            {
                printf("[OW_MAIN] Processing %s command\n", pMessage->command);

                // power off serializer and camera
                printf("-------------> Power-off Headset\n");
                if(!lsw_headset_poweroff())
                {
                    printf("[OW_MAIN::CMD_OPEN_PATIENT] Power-off Headset Failed\n");
        
                    if(!obj->en_ignore_error_state)
                    {
                        sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, OW_STATE_ERROR, CMD_SYSTEM_STATE, NULL, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_CAMERA_INIT], NULL);                                                
                    }

                }

                printf("-------------> Disable Laser\n");
                if(!ta_enable(false)){
        
                    printf("[OW_MAIN::CMD_OPEN_PATIENT] TA_DISABLE Failed\n");
                    if(!obj->en_ignore_error_state)
                    {
                        syslog(LOG_ERR,"TA_DISABLE Failed");
                    }
                }
                usleep(100000);

                if(obj->en_ignore_error_state)
                {
                    // ensuring we clear any laser errors before continuing
                    if(!change_state(OW_STATE_IDLE, OW_UNDEFINED))
                    {
                        // check if it is an estop issue
                    }
                }

                if(reset_state(obj))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_CLOSE_PATIENT, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_RESET], NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::close_patient] Error occurred during full scan of patient\n");
                    // recoverable error if occurred set back to patient state
                    change_state(OW_STATE_READY, OW_UNDEFINED);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_STATE]) == 0)
            {
                printf("[OW_MAIN] Processing %s command\n", pMessage->command);
                printf("CURRENT STATE: %s", OW_SYSTEM_STATE_STRING[current_state()]);
                sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_STATE, NULL, NULL, NULL);
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_TIME]) == 0)            
            {
                printf("[OW_MAIN] Called %s command\n", pMessage->command);
                
                // Get the current time
                time(&current_time);
                // Convert it to local time
                time_info = localtime(&current_time);
                
                // Format the time as a string
                strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
                
                printf("CURRENT STATE: %s SYSTEM TIME: %s\n", OW_SYSTEM_STATE_STRING[current_state()], time_string);
                sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_TIME, NULL, time_string, time_string);
                printf("[OW_MAIN] Finished %s command\n", pMessage->command);

            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SAVE_PATIENT_NOTE]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(set_patient_note(obj, (const char*)pMessage->params))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SAVE_PATIENT_NOTE, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_SAVENOTE_COMPLETE], NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::save_patient_note] Error occurred during save patient note\n");
                    // recoverable error if occurred set back to patient state
                    change_state(OW_STATE_READY, OW_UNDEFINED);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_BACKUP]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(backup(fd_serial_port, obj))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_BACKUP, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_BACKUP_SUCCESS], NULL);
                }

                // recoverable error if occurred set back to idle state
                change_state(OW_STATE_IDLE, OW_UNDEFINED);
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_BACKUP_LOGS]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(perform_log_copy(obj))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_BACKUP_LOGS, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_LOG_BACKUP_SUCCESS], NULL);
                }

                // recoverable error if occurred set back to idle state
                change_state(OW_STATE_IDLE, OW_UNDEFINED);
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_VERSION]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                memset(s_generic_message, 0, sizeof(s_generic_message));
                sprintf(s_generic_message, "%s", GIT_BUILD_VERSION);                
                sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_VERSION, NULL, s_generic_message, NULL);

            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_RESET]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(reset_state(obj))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_RESET, NULL, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_RESET], NULL);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_UPDATE]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(perform_fw_update(obj, (const char*)pMessage->params))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_UPDATE, (const char*)pMessage->params, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_UPDATE_SUCCESS], NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::sys_update] Error occurred during system update\n");
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_SYSTEM_CLEANUP]) == 0)
            {
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(perform_sys_cleanup(obj, (const char*)pMessage->params))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_SYSTEM_CLEANUP, (const char*)pMessage->params, OW_APP_STATUS_MESSAGE_STRING[OW_MSG_STATUS_CLEANUP_SUCCESS], NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::sys_cleanup] Error occurred during system cleanup process\n");
                    // recoverable error set back to idle state
                    change_state(OW_STATE_IDLE, OW_UNDEFINED);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_GET_PATIENT_DATA]) == 0)
            {
                // getdata will get graph data for pair 1-4 and 0 will send 4 messages containing the data for each pair
                printf("[OW_MAIN] Processing %s command with value: %s\n", pMessage->command, pMessage->params);
                if(retrieve_patient_data(obj, (const char*)pMessage->params))
                {
                    sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_GET_PATIENT_DATA, pMessage->params, NULL, NULL);
                }
                else
                {
                    syslog(LOG_ERR, "[OW_MAIN::get_patient_graph] Error occurred during retrieve patient data\n");
                    // recoverable error set back to patient state
                    change_state(OW_STATE_READY, OW_UNDEFINED);
                }
            }
            else if(strcmp(pMessage->command, OW_APP_COMMAND_STRING[CMD_PERSONALITY_BOARD_TEMP]) == 0)            
            {
                printf("[OW_MAIN] Called %s command\n", pMessage->command);
                
                float temp = appGetPersonalityBoardTemp() / (float) 1000.0;
                
                // Format the temperature as a string
                sprintf(temp_string, "Temp: %.2f C", temp);
                
                printf("CURRENT STATE: %s SYSTEM TEMP: %s\n", OW_SYSTEM_STATE_STRING[current_state()], temp_string);
                sendResponseMessage(fd_serial_port, OW_CONTENT_RESPONSE, current_state(), CMD_PERSONALITY_BOARD_TEMP, NULL, temp_string, temp_string);
                printf("[OW_MAIN] Finished %s command\n", pMessage->command);

            }
            else
            {
                printf("[OW_MAIN] Unhandled Command: %s Value: %s\n", pMessage->command, pMessage->params);
                sendResponseMessage(fd_serial_port, OW_CONTENT_ERROR, current_state(), CMD_SYSTEM_STATE, (const char*)pMessage->params, OW_APP_ERR_MESSAGE_STRING[OW_MSG_ERR_COMMAND_UNKNOWN], NULL);
            }
        }

        // Escape from event loop if process has been killed
        if(sigkill_active){
            obj->stop_task = 1;
            done = 1;
            change_state(OW_STATE_ESTOP, OW_UNDEFINED);
        }
    }

    // we are shutting down no need to check for success
    change_state(OW_STATE_SHUTDOWN, OW_UNDEFINED);
    
    printf("[OW_MAIN] Run Task Delete\n");
    app_run_task_delete(obj);
    
    printf("[OW_MAIN] Run Delete Graph\n");
    app_delete_graph(obj);

    printf("[OW_MAIN] Run deinit\n");
    app_deinit(obj);

    printf("[OW_MAIN] Exit Graph\n");
    return status;
}

static uint32_t app_get_pair_mask(AppObj *obj, int32_t pair_num, int32_t scan_type){
    uint32_t mask = 0x00;

    if(scan_type == 1) // test scan
    {
        mask = obj->camera_config_test;
        return mask;
    }
    else if(scan_type == 2) // long scan
    {
        mask = obj->camera_config_long;
        return mask;
    }

    // full scan
    switch(pair_num){
        case -1: // all on (maybe or all the config sets)
            mask = (uint32_t)(obj->left_sensor.ch_mask | obj->right_sensor.ch_mask);
            break;
        case 0: // all off
            mask = 0x00;
            break;
        case 1:
            // mask = 0xC3;  // port 0 and 1 on CSI0
            mask = obj->camera_config_set[0];
            break;
        case 2:
            // mask = 0x33;  // port 0 and 1 on CSI0  and port 0 and 1 on CSI1
            mask = obj->camera_config_set[1];
            break;
        case 3:
            // mask = 0xC3;
            mask = obj->camera_config_set[2];
            break;
        case 4:
            // mask = 0x33;
            mask = obj->camera_config_set[3];
            break;
        default:
            printf("===============>>>>>>>> Unkown pair %i\n", pair_num);
            syslog(LOG_WARNING, "[OW_MAIN::app_get_pair_mask] Camera Pair Unkown, Requested pair: %i", pair_num);
            break;
    }
    
    return mask;
}

static int32_t app_switch_camera_pair(AppObj *obj, int32_t pair_num, int32_t scan_type)
{
    int32_t status = 0;
    uint32_t pair_mask = 0x00;
    if (obj->left_sensor.num_cameras_enabled > 0)
    {
        if(pair_num == 0)
        {
            // turn off streams and return
        }

        // stop framesync
        appStopFrameSync();

        printf("Requested Pair: %d ScanType: %d\n", pair_num, scan_type);
        obj->previousActivePair = obj->currentActivePair;
        printf("Previous Pair: %d \n",  obj->previousActivePair);
        obj->currentActivePair = pair_num;
        pair_mask = app_get_pair_mask(obj, obj->previousActivePair, obj->previousActiveScanType);
        
        // stop forwarding
        status |= appActivateCameraPairs(0);

        printf("Stop Previous Pair Mask: 0x%02X\n",  pair_mask);
        // stop current
        status = appStopImageSensor(obj->left_sensor.sensor_name, pair_mask & 0x0F);
        status = appStopImageSensor(obj->right_sensor.sensor_name, pair_mask & 0xF0);
        
        // switch pair
        obj->previousActiveScanType = obj->currentActiveScanType;
        obj->currentActiveScanType = scan_type;
        pair_mask = app_get_pair_mask(obj, obj->currentActivePair, scan_type);
        
        printf("Start New Pair Mask: 0x%02X\n",  pair_mask);

        status= appStartImageSensor(obj->left_sensor.sensor_name, pair_mask & 0x0F);
        status= appStartImageSensor(obj->right_sensor.sensor_name, pair_mask & 0xF0);
        status |= app_send_cmd_histo_change_pair(&obj->left_histo, pair_num);
        status |= app_send_cmd_histo_enableChan(&obj->left_histo, 0x07);

        status |= appActivateCameraMask(pair_mask);

        // start framesync
        if(pair_mask !=0)
        {            
            appStartFrameSync();
        }
        else
        {
            APP_PRINTF("Pair mask is zero and frame sync is off");
        }
    }
    else
    {
        // no need to switch
        status = 1;
    }

    return status;
}

static vx_status app_run_graph_interactive(AppObj *obj)
{
    vx_status status = VX_SUCCESS;

    printf("[OW_MAIN] app_run_graph_interactive \n");
    syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_interactive] Interactive mode starting");

    uint32_t done = 0;
    float temp =0;
 
    // uint32_t pair_mask = 0;
    char ch;

    // initialize graph and start task

    printf("[OW_MAIN] Initializing APP\n");
    status = app_init(obj);
    if (status != VX_SUCCESS)
    {
        printf("[OW_MAIN] Failed to Initialize APP\n");
        syslog (LOG_ERR, "[OW_MAIN::app_run_graph_interactive] Failed to Initialize APP");
        if (obj->en_ignore_graph_errors == 0)
        {
            return status;
        }
    }

    printf("[OW_MAIN::app_run_graph_interactive] APP INIT DONE! Creating Graph\n");
    status = app_create_graph(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_interactive] Failed to Create Graph");
        printf("[OW_MAIN::app_run_graph_interactive] Failed to Create Graph\n");
        if (obj->en_ignore_graph_errors == 0)
        {
            return status;
        }
    }

    printf("[OW_MAIN::app_run_graph_interactive] App Create Graph Done!\n");
    status = app_verify_graph(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_interactive] Failed to Verify Graph");
        printf("[OW_MAIN::app_run_graph_interactive] Failed to Verify Graph\n");
        if (obj->en_ignore_graph_errors == 0)
        {
            return status;
        }
    }

    printf("[OW_MAIN::app_run_graph_interactive] App Verify Graph Done! Creating Task\n");
    status = app_run_task_create(obj);
    if (status != VX_SUCCESS)
    {
        syslog(LOG_ERR, "[OW_MAIN::app_run_graph_interactive] Failed to Create Task");
        printf("[OW_MAIN::app_run_graph_interactive] Failed to create Task\n");
        if (obj->en_ignore_graph_errors == 0)
        {
            return status;
        }
    }

    printf("[OW_MAIN::app_run_graph_interactive] Task started\n");

    appPerfStatsResetAll();
    if(!change_state(OW_STATE_READY, OW_UNDEFINED))
    {
        // send message and shutdown if estop
    }

    // Wait 2 sec before displaying the menu so that MCU has finished displaying its messages
    sleep(2);

    while(!done)
    {
        printf("Build Date: %s\n", GIT_BUILD_DATE);
        printf("GIT Version: %s\n\n", GIT_BUILD_VERSION);
        printf(usecase_menu);
        do {
            ch = appGetChar();
        } while(ch == '\n' || ch == '\r');
        printf("\n");

        switch(ch)
        {
            case '1':
            case '2':
            case '3':
            case '4':
                printf("[OW_MAIN] switch camera to pair: %d requested\n", ((int)ch - (int)48));
                if(set_cam_pair(obj, ((int)ch - (int)48)))
                {
                    printf("set camera pair to: %d\n", ((int)ch - (int)48));
                }
                else
                {
                    printf("[OW_MAIN] Failed to set camera pair, response was NULL\n");
                }
                break;
            case 'p':
                appPerfStatsPrintAll();
                status = tivx_utils_graph_perf_print(obj->graph);
                appPerfPointPrint(&obj->fileio_perf);
                appPerfPointPrint(&obj->total_perf);
                printf("\n");
                appPerfPointPrintFPS(&obj->total_perf);
                appPerfPointReset(&obj->total_perf);
                printf("\n");
                if (obj->input_streaming_type == INPUT_STREAM_SERDES)
                {
                    vx_reference refs[1];
                    refs[0] = (vx_reference)obj->left_capture.raw_image_arr[0];
                    if (status == VX_SUCCESS)
                    {
                        status = tivxNodeSendCommand(obj->left_capture.node, 0u,
                                                    TIVX_CAPTURE_PRINT_STATISTICS,
                                                    refs, 1u);
                    }
                    refs[0] = (vx_reference)obj->right_capture.raw_image_arr[0];
                    if (status == VX_SUCCESS)
                    {
                        status = tivxNodeSendCommand(obj->right_capture.node, 0u,
                                                    TIVX_CAPTURE_PRINT_STATISTICS,
                                                    refs, 1u);
                    }
                }
                break;
            case 'f':
                printf("[OW_MAIN] Perform full scan requested\n");
                if(perform_scan(obj, obj->full_scan_frame_count, 0))
                {
                    printf("[OW_MAIN] Full scan completed successfully\n");
                }
                else
                {
                    printf("[OW_MAIN] Failed to perform full scan, response was NULL\n");
                }
                break;
            case 'h':
                printf("[OW_MAIN] Perform single histogram capture requested\n");
                owWriteHistoToFile(obj, 1, 0, true, 60);
                break;
            case 't':
                printf("[OW_MAIN] Perform test scan requested\n");
                if(perform_scan(obj, obj->test_scan_frame_count, 1))
                {
                    printf("[OW_MAIN] Test scan completed successfully\n");
                }
                else
                {
                    printf("[OW_MAIN] Failed to perform test scan, response was NULL\n");
                }
                break;
            case 's':
                printf("[OW_MAIN] Perform single image capture requested\n");
                owWriteImageToFile(obj, 1, 0, true, 60);
                break;
            case 'a':
                if(set_cam_pair(obj, 1))
                {
                    printf("[OW_MAIN] Running Scan Sequence\n");
                    sleep(1);
                    if(run_scan_sequence(obj, 1, 1, 1, 1, 0)){
                        printf("Error running scan sequence\n");
                    }else{
                        printf("Successfully ran scan sequence\n");
                    }
                }
                else
                {
                    printf("[OW_MAIN] Failed to set camera pair, response was NULL\n");
                }
                break;
            case 'l':
                printf("[OW_MAIN] Enable Laser requested\n");
                // enable laser
                if(!ta_enable(true) || appEnableLaser() < 0)
                {
                    printf("[OW_MAIN] Enable Laser Failed\n");
                }
                else
                {
                    printf("[OW_MAIN] Enable Laser Success\n");
                }
                break;
            case 'k':
                printf("[OW_MAIN] Disable Laser requested\n");
                // disable laser
                if(appDisableLaser() < 0 || !ta_enable(false))
                {
                    printf("[OW_MAIN] Disable Laser Failed\n");
                }
                else
                {
                    printf("[OW_MAIN] Disable Laser Success\n");
                }
                break;
            case 'u':
                run_d3_utils_menu();
                break;
            case 'v':
                printf("[OW_MAIN] View Camera sensor temperatures\n");
                printf("Temperatures Upper: %08X Lower: %08X\n", cam_temp_upper_output, cam_temp_lower_output);                
                break;
            case 'w':
                // change laser channel
                printf("\n\nEnter channel number:\n");
                do {
                    ch = appGetChar();
                } while(ch == '\n' || ch == '\r');
                if (ch >= '0' && ch <= '9')
                {
                    printf("[OW_MAIN] switch to channel: %d requested\n", (uint8_t)(ch - '0'));
                    if(!lsw_set_channel(ch - '0'))
                    {
                        printf("[OW_MAIN] Failed to set channel: %d\n", (uint8_t)(ch - '0'));
                    }
                    else
                    {
                        printf("[OW_MAIN] successfully switched to channel: %d\n", (uint8_t)(ch - '0'));
                    }
                }
                break;
            case 'z':
                // printf("[OW_MAIN] send histo time sync\n");            
                // app_send_cmd_histo_time_sync(&obj->left_histo, 0);
                printf("[OW_MAIN] Local Time\n");   
                {                    

                    // Get the current time
                    time(&current_time);
                    // Convert it to local time
                    time_info = localtime(&current_time);
                    
                    // Format the time as a string
                    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
                    
                    printf("CURRENT STATE: %s SYSTEM TIME: %s\n", OW_SYSTEM_STATE_STRING[current_state()], time_string);

                }
                break;

            case 'c':
                temp = appGetPersonalityBoardTemp();
                printf("Temp: %f", temp);
                break;
            case 'x':
                printf("[OW_MAIN] Exiting Application\n");
                obj->stop_task = 1;
                done = 1;
                break;

        }

        // Escape from event loop if process has been killed
        if(sigkill_active){
            obj->stop_task = 1;
            done = 1;
            change_state(OW_STATE_ESTOP, OW_UNDEFINED);
        }
    }

    printf("[OW_MAIN] Run Task Delete\n");
    app_run_task_delete(obj);

    app_delete_graph(obj);
    printf("[OW_MAIN] App Delete Graph Done! \n");

    app_deinit(obj);
    printf("[OW_MAIN] App De-init Done! \n");

    // exiting no reason to check
    change_state(OW_STATE_IDLE, OW_UNDEFINED);
    syslog(LOG_NOTICE, "[OW_MAIN::app_run_graph_interactive] Interactive mode exiting");

    return status;
}

static void app_delete_graph(AppObj *obj)
{
    printf("delete left\n");
    app_delete_custom_capture(&obj->left_capture);
    app_delete_histo(&obj->left_histo);
    
    printf("delete right\n");
    app_delete_custom_capture(&obj->right_capture);
    app_delete_histo(&obj->right_histo);

    vxReleaseGraph(&obj->graph);
    APP_PRINTF("Graph delete done!\n");
}

static void app_default_param_set(AppObj *obj)
{

    set_sensor_defaults(&obj->left_sensor);
    set_sensor_defaults(&obj->right_sensor);

    app_pipeline_params_defaults(obj);

    obj->is_interactive = 1;
    obj->test_mode = 0;
    obj->write_file = 0;
    obj->write_histo = 0;
    obj->scan_type = 0;
    obj->wait_histo = 0;
    obj->wait_file = 0;
    obj->num_histo_bins= 1024;

    /* Disable histogram calculation for all channels */
    obj->left_histo.params.enableChanBitFlag= 0;
    obj->right_histo.params.enableChanBitFlag= 0;

    obj->left_sensor.enable_ldc = 0;
    obj->left_sensor.num_cameras_enabled = 3;
    obj->left_sensor.ch_mask = 0x07;
    obj->left_sensor.usecase_option = APP_SENSOR_FEATURE_CFG_UC0;
    obj->left_sensor.is_interactive= 0;
    
    obj->right_sensor.enable_ldc = 0;
    obj->right_sensor.num_cameras_enabled = 3;
    obj->right_sensor.ch_mask = 0x07;
    obj->right_sensor.usecase_option = APP_SENSOR_FEATURE_CFG_UC0;
    obj->right_sensor.is_interactive= 0;

    obj->screendIdx= 0;
    obj->currentActivePair= -1; // Activate all cameras initially
    obj->previousActivePair= -1;
    obj->left_capture.enable_error_detection= 1;
    obj->right_capture.enable_error_detection= 1;
    
    obj->currentActiveScanType = 0;
    obj->previousActiveScanType = 0;
}

static void app_update_param_set(AppObj *obj)
{
    vx_uint16 resized_width, resized_height;
    appIssGetResizeParams(obj->left_sensor.image_width, obj->left_sensor.image_height, DISPLAY_WIDTH, DISPLAY_HEIGHT, &resized_width, &resized_height);
}
