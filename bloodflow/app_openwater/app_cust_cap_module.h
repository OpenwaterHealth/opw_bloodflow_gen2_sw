#ifndef _APP_CUST_CAP_MODULE_H
#define _APP_CUST_CAP_MODULE_H

#include <VX/vx.h>
#include <TI/tivx.h>
#include "app_common.h"
#include "app_sensor_module.h"

/** \brief Capture Module Data Structure
 *
 * Contains the data objects required to use the tivxHwaCapture node
 *
 */
typedef struct {
    /*! Capture node object */
    vx_node node;

    /*! User data object for config parameter, used as node parameter of Capture node */
    vx_user_data_object config;

    /*! Capture node params structure to initialize config object */
    tivx_capture_params_t params;

    /*! Capture node object array output */
    vx_object_array raw_image_arr[APP_MODULES_MAX_BUFQ_DEPTH];

    /*! Capture node graph parameter index */
    vx_int32 graph_parameter_index;

    /*! Flag to indicate whether or not the intermediate output is written */
    vx_int32 en_out_capture_write;

    /*! Flag to enable test mode */
    vx_int32 test_mode;

    /*! Node used to write capture output */
    vx_node write_node;

    /*! File path used to write capture node output */
    vx_array file_path;

    /*! File path prefix used to write capture node output */
    vx_array file_prefix;

    /*! User data object containing write cmd parameters */
    vx_user_data_object write_cmd;

    /*! Output file path for capture node output */
    vx_char output_file_path[TIVX_FILEIO_FILE_PATH_LENGTH];

    /*! Name of capture module */
    vx_char objName[APP_MODULES_MAX_OBJ_NAME_SIZE];

    /*! Raw image used when camera gets disconnected */
    tivx_raw_image error_frame_raw_image;

    /*! Flag to enable detection of camera disconnection */
    vx_uint8 enable_error_detection;

    /*! Capture node format, taken from sensor_out_format */
    vx_uint32 capture_format;
}CaptureObj;

vx_status app_init_custom_capture(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, char *objName, int32_t bufq_depth, uint32_t instanceID);
vx_status app_create_custom_graph_capture(vx_graph graph, CaptureObj *captureObj, uint32_t instanceID);
vx_status app_deinit_custom_capture(CaptureObj *captureObj, vx_int32 bufq_depth);
vx_status app_delete_custom_capture(CaptureObj *captureObj);
vx_status app_send_error_frame(CaptureObj *captureObj);
vx_status app_send_cmd_capture_write_node(CaptureObj *captureObj, vx_uint32 enableChanBitFlag, vx_uint32 start_frame, vx_uint32 num_frames, vx_uint32 num_skip,  vx_uint32 scan_type, const char* patient_path, const char* sub_path);

#endif