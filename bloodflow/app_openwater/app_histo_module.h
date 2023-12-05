/**************************************************************************
*  Copyright 2021   Neither this file nor any part of it may be used for
*     any purpose without the written consent of D3 Engineering, LLC.
*     All rights reserved.
*
*				D 3   E N G I N E E R I N G
*
* File Name   :	app_histo_module.h
* Release     :	1.0
* Author      :	D3 Engineering Technical Staff
* Created     :	09/21/2021
* Revision    :	1.0
* Description :	Header file for the histogram module
*
*
***************************************************************************
* Revision History
* Ver	Date			Author		Description
* 1.0	09/21/2021		VC		Initial version
***************************************************************************/

#ifndef _APP_HISTO_MODULE
#define _APP_HISTO_MODULE

#include "app_common.h"
#include "app_sensor_module.h"

#include <tivx_kernel_ow_calc_histo.h>


#define APP_HISTO_NUM_EXTRA_PARAMS 2

typedef struct {
    vx_node  node;
    vx_node  write_node;

    vx_user_data_object config;
    tivxC66HistoParams params;

    vx_object_array outHistoArr[APP_MAX_BUFQ_DEPTH];
    vx_distribution outHistoCh0[APP_MAX_BUFQ_DEPTH];

    vx_object_array outMeanArr[APP_MAX_BUFQ_DEPTH];
    vx_scalar outMeanCh0[APP_MAX_BUFQ_DEPTH];

    vx_object_array outSdArr[APP_MAX_BUFQ_DEPTH];
    vx_scalar outSdCh0[APP_MAX_BUFQ_DEPTH];

    vx_int32 graph_parameter_distribution_index;
    vx_int32 graph_parameter_mean_index;
    vx_int32 graph_parameter_sd_index;
    vx_char objName[APP_MAX_FILE_PATH];
    uint32_t num_channels;
    uint32_t range;
    uint32_t numBins;
    vx_array file_path;
    vx_array file_prefix;
    vx_user_data_object write_cmd;
    vx_user_data_object cmd;
    vx_char output_file_path[APP_MODULES_MAX_OBJ_NAME_SIZE];
    vx_user_data_object time_sync;
} HistoObj;

vx_status app_init_histo(vx_context context, HistoObj *histoObj, SensorObj *sensorObj, char *objName, vx_int32 bufq_depth, uint32_t num_histo_bins, uint32_t chan_count, uint32_t instanceID);
void app_deinit_histo(HistoObj *histoObj, vx_int32 bufq_depth);
void app_delete_histo(HistoObj *histoObj);
vx_status app_create_graph_histo(vx_graph graph,
                           HistoObj *histoObj,
                           vx_object_array inputRawImageArr,
                           uint32_t instanceID);
vx_status app_create_graph_histo_write_output(vx_graph graph, HistoObj *histoObj, uint32_t instanceID);
vx_status app_send_cmd_histo_write_node(HistoObj *histoObj, vx_uint32 enableChanBitFlag, vx_uint32 start_frame, vx_uint32 num_frames, vx_uint32 num_skip, vx_int32 cam_pair, vx_int32 scan_type, const char* patient_path, const char* sub_path, void* virt_ptr);
vx_status app_send_cmd_histo_enableChan(HistoObj *histoObj, uint32_t enableChanBitFlag);
vx_status app_send_cmd_histo_change_pair(HistoObj *histoObj, uint32_t chanelpair);
vx_status app_send_cmd_histo_time_sync(HistoObj *histoObj, uint64_t ref_ts);

#endif
