
#include "app_cust_cap_module.h"
#include <VX/vx.h>
#include <TI/tivx.h>
#include <iss_sensors.h>
#include <iss_sensor_if.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static vx_status configure_custom_capture_params(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, uint32_t instanceID);
static vx_status create_custom_capture_output(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj,  int32_t bufq_depth, uint32_t instanceID);
static vx_status create_custom_error_detection_frame(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, uint32_t instanceID);
static vx_status configure_custom_capture_output_write(vx_context context, CaptureObj *captureObj, uint32_t instanceID);
static vx_status app_create_graph_capture_write_output(vx_graph graph, CaptureObj *captureObj, uint32_t instanceID);
static tivx_raw_image read_error_image_raw(vx_context context,
                             IssSensor_Info *sensorInfo, char raw_image_fname[],
                             vx_int32 *bytes_read);

static vx_status configure_custom_capture_params(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;
    vx_int32 id, lane;

    printf("Instance = %d ch_mask = 0x%02x Num of Cameras = %d\n", instanceID, sensorObj->ch_mask, sensorObj->num_cameras_enabled); 

    captureObj->capture_format = sensorObj->sensor_out_format;

    tivx_capture_params_init(&captureObj->params);

    if (captureObj->enable_error_detection || captureObj->test_mode)
    {
        captureObj->params.timeout        = 90;
        captureObj->params.timeoutInitial = 500;
    }
    captureObj->params.numInst  = 1;
    captureObj->params.numCh    = sensorObj->num_cameras_enabled;
    
    captureObj->params.instId[0]                       = instanceID;
    captureObj->params.instCfg[0].enableCsiv2p0Support = (uint32_t)vx_true_e;
    captureObj->params.instCfg[0].numDataLanes         = sensorObj->sensorParams.sensorInfo.numDataLanes;
    printf("captureObj->params.numDataLanes = %d \n", captureObj->params.instCfg[0].numDataLanes);
    captureObj->params.instCfg[0].laneBandSpeed        = sensorObj->sensorParams.sensorInfo.csi_laneBandSpeed;
    printf("laneBandSpeed = %d \n", captureObj->params.instCfg[0].laneBandSpeed);

    for (lane = 0; lane < captureObj->params.instCfg[0].numDataLanes; lane++)
    {
        captureObj->params.instCfg[0].dataLanesMap[lane] = lane + 1;
        printf("captureObj->params.dataLanesMap[%d] = %d \n",
                    lane,
                    captureObj->params.instCfg[0].dataLanesMap[lane]);
    }

    for(id = 0; id < sensorObj->num_cameras_enabled; id++)
    {
        captureObj->params.chVcNum[id] = id;
        captureObj->params.chInstMap[id] = instanceID;
    }
            
    captureObj->config = vxCreateUserDataObject(context, "tivx_capture_params_t", sizeof(tivx_capture_params_t), &captureObj->params);
    status = vxGetStatus((vx_reference)captureObj->config);

    if(status != VX_SUCCESS)
    {
        printf("[CAPTURE_MODULE] - Unable to create capture config object! \n");
    }
    else
    {
        if(instanceID == CAPT1_INST_ID)
        {
            vxSetReferenceName((vx_reference)captureObj->config, "capture_left_node_config");
        }
        else
        {
            vxSetReferenceName((vx_reference)captureObj->config, "capture_right_node_config");
        }
    }

    return status;
}

static vx_status create_custom_capture_output(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj,  int32_t bufq_depth, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    IssSensor_CreateParams *sensorParams = &sensorObj->sensorParams;
    vx_int32 q;
    
    printf("Instance = %d ch_mask = 0x%02x Num of Cameras = %d\n", instanceID, sensorObj->ch_mask, sensorObj->num_cameras_enabled); 

    if(0 == captureObj->capture_format)
    {
        tivx_raw_image raw_image = tivxCreateRawImage(context, &sensorParams->sensorInfo.raw_params);
        status = vxGetStatus((vx_reference)raw_image);

        if(status == VX_SUCCESS)
        {
            for(q = 0; q < bufq_depth; q++)
            {
                captureObj->raw_image_arr[q] = vxCreateObjectArray(context, (vx_reference)raw_image, sensorObj->num_cameras_enabled);
                status = vxGetStatus((vx_reference)captureObj->raw_image_arr[q]);
                if(status != VX_SUCCESS)
                {
                    printf("[CAPTURE-MODULE] Unable to create RAW image object array! \n");
                    break;
                }
                else
                {
                    vx_char name[VX_MAX_REFERENCE_NAME];

                    if(instanceID == CAPT1_INST_ID)
                    {
                        snprintf(name, VX_MAX_REFERENCE_NAME, "capture_left_raw_image_arr_%d", q);
                    }
                    else
                    {
                        snprintf(name, VX_MAX_REFERENCE_NAME, "capture_right_raw_image_arr_%d", q);
                    }

                    vxSetReferenceName((vx_reference)captureObj->raw_image_arr[q], name);
                }
            }
            tivxReleaseRawImage(&raw_image);
        }
        else
        {
            printf("[CAPTURE-MODULE] Unable to create RAW image object! for instance %d \n", instanceID);
        }
    }
    else
    {
        printf("[CAPTURE-MODULE] YUV image Type not supported! \n");
    }

    return status;
}

static vx_status create_custom_error_detection_frame(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    IssSensor_CreateParams *sensorParams = &sensorObj->sensorParams;

    /*Error detection is currently enabled only for RAW input*/

    /* If in test mode, send the test frame */
    if(0 != captureObj->capture_format)
    {
        captureObj->enable_error_detection = 0;
    }

    /* If we are doing error detection, just send a black frame to the dead sensor */
    if(captureObj->enable_error_detection)
    {
        vx_map_id map_id;
        vx_rectangle_t rect;
        vx_imagepatch_addressing_t image_addr;
        void * data_ptr;

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = sensorParams->sensorInfo.raw_params.width;
        rect.end_y = sensorParams->sensorInfo.raw_params.height;

        captureObj->error_frame_raw_image = tivxCreateRawImage(context, &(sensorParams->sensorInfo.raw_params));

        status = vxGetStatus((vx_reference)captureObj->error_frame_raw_image);

        if(status == VX_SUCCESS)
        {
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->error_frame_raw_image, "capture_left_error_frame_raw_image");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->error_frame_raw_image, "capture_right_error_frame_raw_image");
            }

            status = tivxMapRawImagePatch((tivx_raw_image)captureObj->error_frame_raw_image, &rect, 0, &map_id, &image_addr, &data_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, TIVX_RAW_IMAGE_ALLOC_BUFFER);

            if ((vx_status)VX_SUCCESS == status)
            {
                memset(data_ptr, 0x00, image_addr.stride_y*(sensorParams->sensorInfo.raw_params.height+sensorParams->sensorInfo.raw_params.meta_height_before+sensorParams->sensorInfo.raw_params.meta_height_after));
            }
        }
        else
        {
            printf("[CAPTURE_MODULE] Unable to crate error frame RAW image!\n");
        }
    }
    else
    {
        captureObj->error_frame_raw_image = NULL;
    }
    
    return status;
}

static vx_status configure_custom_capture_output_write(vx_context context, CaptureObj *captureObj, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    if(captureObj->en_out_capture_write == 1)
    {
        char file_path[TIVX_FILEIO_FILE_PATH_LENGTH];
        char file_prefix[TIVX_FILEIO_FILE_PREFIX_LENGTH];

        strcpy(file_path, captureObj->output_file_path);
        captureObj->file_path   = vxCreateArray(context, VX_TYPE_UINT8, TIVX_FILEIO_FILE_PATH_LENGTH);
        status = vxGetStatus((vx_reference)captureObj->file_path);

        if(status == VX_SUCCESS)
        {
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->file_path, "capture_write_left_file_path");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->file_path, "capture_write_right_file_path");
            }

            vxAddArrayItems(captureObj->file_path, TIVX_FILEIO_FILE_PATH_LENGTH, &file_path[0], 1);
        }
        else
        {
            printf("[CAPTURE-MODULE] Unable to create file path object for writing capture output! \n");
        }

        strcpy(file_prefix, "csix_raw_output");
        captureObj->file_prefix = vxCreateArray(context, VX_TYPE_UINT8, TIVX_FILEIO_FILE_PREFIX_LENGTH);

        status = vxGetStatus((vx_reference)captureObj->file_prefix);

        if(status == VX_SUCCESS)
        {
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->file_prefix, "capture_write_left_file_prefix");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->file_prefix, "capture_write_right_file_prefix");
            }

            vxAddArrayItems(captureObj->file_prefix, TIVX_FILEIO_FILE_PREFIX_LENGTH, &file_prefix[0], 1);
        }
        else
        {
            printf("[CAPTURE-MODULE] Unable to create file prefix object for writing capture output! \n");
        }
        

        captureObj->write_cmd = vxCreateUserDataObject(context, "tivxFileIOWriteCmd", sizeof(tivxFileIOWriteCmd), NULL);
        status = vxGetStatus((vx_reference)captureObj->write_cmd);

        if(status != VX_SUCCESS)
        {
            printf("[CAPTURE-MODULE] Unable to create file write cmd object for writing capture output! \n");
        }
        else
        {
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->write_cmd, "capture_write_left_write_cmd");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->write_cmd, "capture_write_right_write_cmd");
            }
        }
    }
    else
    {
        captureObj->file_path   = NULL;
        captureObj->file_prefix = NULL;
        captureObj->write_node  = NULL;
        captureObj->write_cmd   = NULL;
    }

    return status;
}

vx_status app_deinit_custom_capture(CaptureObj *captureObj, vx_int32 bufq_depth)
{
    vx_int32 i;

    vxReleaseUserDataObject(&captureObj->config);
    for(i = 0; i < bufq_depth; i++)
    {
        vxReleaseObjectArray(&captureObj->raw_image_arr[i]);
    }

    if(NULL != captureObj->error_frame_raw_image)
    {
        tivxReleaseRawImage(&captureObj->error_frame_raw_image);
    }

    if(captureObj->en_out_capture_write == 1)
    {
        vxReleaseArray(&captureObj->file_path);
        vxReleaseArray(&captureObj->file_prefix);
        vxReleaseUserDataObject(&captureObj->write_cmd);
    }

    return VX_SUCCESS;
}

vx_status app_delete_custom_capture(CaptureObj *captureObj)
{
    if(captureObj->node != NULL)
    {
        vxReleaseNode(&captureObj->node);
    }
    if(captureObj->write_node != NULL)
    {
        vxReleaseNode(&captureObj->write_node);
    }
    return VX_SUCCESS;
}

vx_status app_create_custom_graph_capture(vx_graph graph, CaptureObj *captureObj, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    captureObj->node = tivxCaptureNode(graph, captureObj->config, captureObj->raw_image_arr[0]);
    status = vxGetStatus((vx_reference)captureObj->node);

    if(status == VX_SUCCESS)
    {
        if(instanceID == CAPT1_INST_ID)
        {
            vxSetNodeTarget(captureObj->node, VX_TARGET_STRING, TIVX_TARGET_CAPTURE1);
            vxSetReferenceName((vx_reference)captureObj->node, "left_capture");
        }
        else
        {
            vxSetNodeTarget(captureObj->node, VX_TARGET_STRING, TIVX_TARGET_CAPTURE2);
            vxSetReferenceName((vx_reference)captureObj->node, "right_capture");
        }

        if(captureObj->en_out_capture_write == 1)
        {
            status = app_create_graph_capture_write_output(graph, captureObj, instanceID);
        }
    }
    else
    {
        printf("[CAPTURE-MODULE] Unable to create capture node! \n");
    }
    
    return status;
}

static vx_status app_create_graph_capture_write_output(vx_graph graph, CaptureObj *captureObj, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    if(0 == captureObj->capture_format)
    {
        tivx_raw_image raw_img = (tivx_raw_image)vxGetObjectArrayItem(captureObj->raw_image_arr[0], 0);
        captureObj->write_node = tivxWriteRawImageNode(graph, raw_img, captureObj->file_path, captureObj->file_prefix);
        tivxReleaseRawImage(&raw_img);

        status = vxGetStatus((vx_reference)captureObj->write_node);
        if(status == VX_SUCCESS)
        {
            vxSetNodeTarget(captureObj->write_node, VX_TARGET_STRING, TIVX_TARGET_A72_0);
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->write_node, "capture_left_write_node");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->write_node, "capture_right_write_node");
            }

            vx_bool replicate[] = { vx_true_e, vx_false_e, vx_false_e};
            vxReplicateNode(graph, captureObj->write_node, replicate, 3);
        }
        else
        {
            printf("[CAPTURE-MODULE] Unable to create RAW image write node! \n");
        }
    }
    else
    {
        vx_image raw_img = (vx_image)vxGetObjectArrayItem(captureObj->raw_image_arr[0], 0);
        captureObj->write_node = tivxWriteImageNode(graph, raw_img, captureObj->file_path, captureObj->file_prefix);
        vxReleaseImage(&raw_img);

        status = vxGetStatus((vx_reference)captureObj->write_node);
        if(status == VX_SUCCESS)
        {
            vxSetNodeTarget(captureObj->write_node, VX_TARGET_STRING, TIVX_TARGET_A72_0);
            if(instanceID == CAPT1_INST_ID)
            {
                vxSetReferenceName((vx_reference)captureObj->write_node, "capture_left_write_node");
            }
            else
            {
                vxSetReferenceName((vx_reference)captureObj->write_node, "capture_right_write_node");
            }

            vx_bool replicate[] = { vx_true_e, vx_false_e, vx_false_e};
            vxReplicateNode(graph, captureObj->write_node, replicate, 3);
        }
        else
        {
            printf("[CAPTURE-MODULE] Unable to create YUV image write node! \n");
        }
    }
    return (status);
}

vx_status app_init_custom_capture(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj, char *objName, int32_t bufq_depth, uint32_t instanceID)
{
    vx_status status = VX_SUCCESS;

    status = configure_custom_capture_params(context, captureObj, sensorObj, instanceID);

    if(status == VX_SUCCESS)
    {
        status = create_custom_capture_output(context, captureObj, sensorObj, bufq_depth, instanceID);
    }

    if(status == VX_SUCCESS)
    {
        status = create_custom_error_detection_frame(context, captureObj, sensorObj, instanceID);
    }

    if(status == VX_SUCCESS)
    {
        status = configure_custom_capture_output_write(context, captureObj, instanceID);
    }

    return (status);
}

vx_status app_send_cmd_capture_write_node(CaptureObj *captureObj, vx_uint32 enableChanBitFlag, vx_uint32 start_frame, vx_uint32 num_frames, vx_uint32 num_skip,  vx_uint32 scan_type, const char* patient_path, const char* sub_path)
{
    vx_status status = VX_SUCCESS;

    tivxFileIOWriteCmd write_cmd;

    write_cmd.start_frame = start_frame;
    write_cmd.num_frames = num_frames;
    write_cmd.num_skip = num_skip;
    write_cmd.scan_type = scan_type;
    write_cmd.enableChanBitFlag= enableChanBitFlag;

    // set path to send to the mcu
    memset(write_cmd.patient_path, 0, 64);
    if(patient_path)
    {
        strcpy(write_cmd.patient_path, patient_path);
    }

    memset(write_cmd.sub_path, 0, 32);
    if(sub_path)
    {
        strcpy(write_cmd.sub_path, sub_path);
    }
    
    status = vxCopyUserDataObject(captureObj->write_cmd, 0, sizeof(tivxFileIOWriteCmd),\
                  &write_cmd, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    if(status == VX_SUCCESS)
    {
        vx_reference refs[2];

        refs[0] = (vx_reference)captureObj->write_cmd;
        status = tivxNodeSendCommand(captureObj->write_node, TIVX_CONTROL_CMD_SEND_TO_ALL_REPLICATED_NODES,
                                 TIVX_FILEIO_CMD_SET_FILE_WRITE,
                                 refs, 1u);

        if(VX_SUCCESS != status)
        {
            printf("Capture node send command failed!\n");
        }

        printf("Capture node send command success!\n");
    }
    else
    {
        printf("failed to create vxCopyUserDataObject!\n");
    }

    return (status);
}

vx_status create_error_detection_frame(vx_context context, CaptureObj *captureObj, SensorObj *sensorObj)
{
    vx_status status = VX_SUCCESS;
    IssSensor_CreateParams *sensorParams = &sensorObj->sensorParams;

    /*Error detection is currently enabled only for RAW input*/

    /* If in test mode, send the test frame */
    if(0 != captureObj->capture_format)
    {
        captureObj->enable_error_detection = 0;
    }

    if (captureObj->test_mode)
    {
        char test_data_paths[2][255] =
        {
            "/opt/vision_apps/test_data/psdkra/app_single_cam/IMX390_001/input2.raw",
            "/opt/vision_apps/test_data/psdkra/app_single_cam/AR0233_001/input2.raw"
        };
        struct stat sb;
        
        /* Check that we are not using data from QNX, in which case we should search
           from ti_fs as the root */
        const char * base_path = "/opt/vision_apps";
        if (stat(base_path, &sb) != 0) {
            memcpy(test_data_paths[0], "/ti_fs/vision_apps/test_data/psdkra/app_single_cam/IMX390_001/input2.raw", 73);
            memcpy(test_data_paths[1], "/ti_fs/vision_apps/test_data/psdkra/app_single_cam/AR0233_001/input2.raw", 73);
        }

        vx_int32 bytes_read = 0;
        captureObj->error_frame_raw_image = read_error_image_raw(context, &(sensorParams->sensorInfo),
                                                test_data_paths[sensorObj->sensor_index],
                                                &bytes_read);

        APP_PRINTF("%d bytes were read by read_error_image_raw() from path %s\n", bytes_read, test_data_paths[sensorObj->sensor_index]);
        status = vxGetStatus((vx_reference)captureObj->error_frame_raw_image);

        if(status == VX_SUCCESS)
        {
            APP_PRINTF("%d bytes were read by read_error_image_raw()\n", bytes_read);
            if(bytes_read <= 0)
            {
                printf("[CAPTURE_MODULE] Bytes read by error frame for RAW image is < 0! \n");
                tivxReleaseRawImage(&captureObj->error_frame_raw_image);
                captureObj->error_frame_raw_image = NULL; /* Is this required after releasing the reference? */
            }
        }
        else
        {
            printf("[CAPTURE_MODULE] Unable to crate error frame RAW image!\n");
        }
    }
    else
    {
        /* If we are doing error detection, just send a black frame to the dead sensor */
        if(captureObj->enable_error_detection)
        {
            vx_map_id map_id;
            vx_rectangle_t rect;
            vx_imagepatch_addressing_t image_addr;
            void * data_ptr;

            rect.start_x = 0;
            rect.start_y = 0;
            rect.end_x = sensorParams->sensorInfo.raw_params.width;
            rect.end_y = sensorParams->sensorInfo.raw_params.height;

            captureObj->error_frame_raw_image = tivxCreateRawImage(context, &(sensorParams->sensorInfo.raw_params));

            status = vxGetStatus((vx_reference)captureObj->error_frame_raw_image);

            if(status == VX_SUCCESS)
            {
                vxSetReferenceName((vx_reference)captureObj->error_frame_raw_image, "capture_node_error_frame_raw_image");

                status = tivxMapRawImagePatch((tivx_raw_image)captureObj->error_frame_raw_image, &rect, 0, &map_id, &image_addr, &data_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, TIVX_RAW_IMAGE_ALLOC_BUFFER);

                if ((vx_status)VX_SUCCESS == status)
                {
                    memset(data_ptr, 0x00, image_addr.stride_y*(sensorParams->sensorInfo.raw_params.height+sensorParams->sensorInfo.raw_params.meta_height_before+sensorParams->sensorInfo.raw_params.meta_height_after));
                }
            }
            else
            {
                printf("[CAPTURE_MODULE] Unable to crate error frame RAW image!\n");
            }
        }
        else
        {
            captureObj->error_frame_raw_image = NULL;
        }
    }

    return status;
}

/* This function is largely taken from single_cam_app :
    this function takes a raw_image that is unpopulated
    and populates it with the path set below */
static tivx_raw_image read_error_image_raw(vx_context context,
                             IssSensor_Info *sensorInfo, char raw_image_fname[],
                             vx_int32 *bytes_read)
{
    /* Taking the test data from the app_single_cam demo to save space in the repo */
    FILE * fp;
    vx_uint32 width, height, i;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    void *data_ptr;
    vx_uint32 num_bytes_per_pixel = 2; /*Supports only RAW 12b Unpacked format*/
    vx_uint32 num_bytes_read_from_file;
    tivx_raw_image raw_image = NULL;
    tivx_raw_image_format_t format;
    vx_uint32 imgaddr_width, imgaddr_height, imgaddr_stride;
    /* Provision for injecting RAW frame into the sensor */
    /* rect defines the outer bounds of the raw_image frame
        which will be defined */
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = sensorInfo->raw_params.width;
    rect.end_y = sensorInfo->raw_params.height;

    /* Nothing is being populated here - just an empty frame */
    raw_image = tivxCreateRawImage(context, &(sensorInfo->raw_params));

    APP_PRINTF("Reading test RAW image %s \n", raw_image_fname);
    fp = fopen(raw_image_fname, "rb");
    if(!fp)
    {
        printf("read_test_image_raw : Unable to open file %s\n", raw_image_fname);
        return NULL;
    }
    tivxQueryRawImage(raw_image, TIVX_RAW_IMAGE_WIDTH, &width, sizeof(vx_uint32));
    tivxQueryRawImage(raw_image, TIVX_RAW_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
    tivxQueryRawImage(raw_image, TIVX_RAW_IMAGE_FORMAT, &format, sizeof(format));

    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;

    tivxMapRawImagePatch(raw_image,
        &rect,
        0,
        &map_id,
        &image_addr,
        &data_ptr,
        VX_READ_AND_WRITE,
        VX_MEMORY_TYPE_HOST,
        TIVX_RAW_IMAGE_PIXEL_BUFFER
        );

    if(!data_ptr)
    {
        APP_PRINTF("data_ptr is NULL \n");
        fclose(fp);
        return NULL;
    }
    num_bytes_read_from_file = 0;

    imgaddr_width  = image_addr.dim_x;
    imgaddr_height = image_addr.dim_y;
    imgaddr_stride = image_addr.stride_y;

    for(i=0;i<imgaddr_height;i++)
    {
        num_bytes_read_from_file += fread(data_ptr, 1, imgaddr_width*num_bytes_per_pixel, fp);
        data_ptr += imgaddr_stride;
    }

    tivxUnmapRawImagePatch(raw_image, map_id);

    fclose(fp);
    APP_PRINTF("%d bytes read from %s\n", num_bytes_read_from_file, raw_image_fname);
    *bytes_read = num_bytes_read_from_file;
    return raw_image;
}

vx_status app_send_error_frame(CaptureObj *captureObj)
{
    vx_status status = VX_SUCCESS;

    status = tivxCaptureRegisterErrorFrame(captureObj->node, (vx_reference)captureObj->error_frame_raw_image);

    return status;
}
