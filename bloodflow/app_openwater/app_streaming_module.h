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
* Description :	Header file for the streaming module
*               Wrapper functions for the streaming module, which allow
*               to feed images from files stored in the file system to the
*               graph. 
* 
*		
***************************************************************************
* Revision History
* Ver	Date			Author		Description 
* 1.0	09/21/2021		VC		Initial version	
***************************************************************************/

#ifndef _APP_STREAMING_MODULE
#define _APP_STREAMING_MODULE

#include "app_common.h"
#include "app_sensor_module.h"
#include "inputStream.h"

typedef struct {
    vx_object_array raw_image_arr[APP_MAX_BUFQ_DEPTH];
    tivx_raw_image raw_image_ch0[APP_MAX_BUFQ_DEPTH];
    vx_int32 graph_parameter_index;
    vx_int32 vision_input_streaming_type;
    vx_int32 demo_mode;
    vx_int32 num_frames;
    InputStream visionStreamObj;
    InputStream_InitParams visionStreamInitParams;
} StreamingObj;

vx_status app_init_streaming(vx_context context, StreamingObj *obj, SensorObj *sensorObj, char *objName, vx_int32 bufq_depth);
void app_deinit_streaming(StreamingObj *streamingObj, vx_int32 bufq_depth);
vx_status app_read_streaming(StreamingObj *obj, int32_t queueIdx);

#endif