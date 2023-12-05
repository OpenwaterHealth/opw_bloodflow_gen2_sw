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
* Description :	Wrapper functions for the streaming module, which allow
*               to feed images from files stored in the file system to the
*               graph.
*
*
***************************************************************************
* Revision History
* Ver	Date			Author		Description
* 1.0	09/21/2021		VC		Initial version
***************************************************************************/

#include "app_streaming_module.h"
#include "app_common.h"

vx_status app_init_streaming(vx_context context, StreamingObj *obj, SensorObj *sensorObj, char *objName, vx_int32 bufq_depth)
{
    vx_status status = VX_SUCCESS;
    IssSensor_CreateParams *sensorParams = &sensorObj->sensorParams;

    printf("app_init_streaming\n");

    obj->visionStreamInitParams.sensingId = INPUT_STREAM_VISION;
    obj->visionStreamInitParams.typeId = INPUT_STREAM_FILE;
    obj->visionStreamInitParams.numChannels = 2;
    //if (obj->demo_mode == 1)
    //{
    obj->visionStreamInitParams.mode = INPUT_STREAM_INFINITE_LOOP;
    //}
    //else
    //{
    //    obj->visionStreamInitParams.mode = 0;
    //}
    status = inputStreamInit(&obj->visionStreamObj, &obj->visionStreamInitParams);

    if (status == VX_SUCCESS)
    {
        uint32_t numFiles;
        uint32_t maxNumFiles = 0;

        for (int32_t chan = 0; chan < 2; chan++)
        {
            status = obj->visionStreamObj.getStreamLen(&obj->visionStreamObj, chan, &numFiles);
            if (numFiles > maxNumFiles)
            {
                maxNumFiles = numFiles;
            }
        }
        obj->num_frames = maxNumFiles;
    }

    if (status == VX_SUCCESS)
    {
        tivx_raw_image raw_image = tivxCreateRawImage(context, &sensorParams->sensorInfo.raw_params);
        status = vxGetStatus((vx_reference)raw_image);

        if (status == VX_SUCCESS)
        {
            for (int32_t q = 0; q < bufq_depth; q++)
            {
                obj->raw_image_arr[q] = vxCreateObjectArray(context, (vx_reference)raw_image, 2);
                status = vxGetStatus((vx_reference)obj->raw_image_arr[q]);
                if (status != VX_SUCCESS)
                {
                    printf("[STREAMING MODULE ERROR] Unable to create RAW image object array! \n");
                    break;
                }
                obj->raw_image_ch0[q] = (tivx_raw_image)vxGetObjectArrayItem((vx_object_array)obj->raw_image_arr[q], 0);
            }
            tivxReleaseRawImage(&raw_image);
        }
        else
        {
            printf("[STREAMING MODULE ERROR] Unable to create RAW image object! \n");
        }
    }

    return status;
}

vx_status app_read_streaming(StreamingObj *obj, int32_t queueIdx)
{
    vx_status status = VX_SUCCESS;
    vx_object_array objArr;

    objArr = obj->raw_image_arr[queueIdx];
    status = vxGetStatus((vx_reference)objArr);

    if (status == VX_SUCCESS)
    {
        int32_t channel;
        tivx_raw_image inRawFrame;
        vx_imagepatch_addressing_t image_addr;
        vx_rectangle_t rect;
        vx_map_id map_id;
        uint8_t *data_ptr;
        vx_uint32 num_bytes=0;
        tivx_raw_image_format_t format[3];
        vx_size numChannels;
        uint32_t numBytesRead;

        /* Get the total number of channels */
        vxQueryObjectArray(objArr, VX_OBJECT_ARRAY_NUMITEMS, &numChannels, sizeof(vx_size));

        for (channel = 0; channel < numChannels; channel++)
        {
            vx_uint32 width, height;

            inRawFrame = (tivx_raw_image)vxGetObjectArrayItem(objArr, channel);
            status = vxGetStatus((vx_reference)inRawFrame);

            if (status == VX_SUCCESS)
            {
                tivxQueryRawImage(inRawFrame, TIVX_RAW_IMAGE_WIDTH, &width, sizeof(vx_uint32));
                tivxQueryRawImage(inRawFrame, TIVX_RAW_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
                tivxQueryRawImage(inRawFrame, TIVX_RAW_IMAGE_FORMAT, &format, sizeof(format));

                rect.start_x = 0;
                rect.start_y = 0;
                rect.end_x = width;
                rect.end_y = height;

                if (format[0].pixel_container == TIVX_RAW_IMAGE_16_BIT)
                {
                    num_bytes = 2;
                    //printf("Pixel container is 16 bits, width= %d, height=%d\n", width, height);
                }
                else if (format[0].pixel_container == TIVX_RAW_IMAGE_8_BIT)
                {
                    num_bytes = 1;
                }
                else if (format[0].pixel_container == TIVX_RAW_IMAGE_P12_BIT)
                {
                    num_bytes = 0;
                }

                status = tivxMapRawImagePatch(inRawFrame,
                                              &rect,
                                              0,
                                              &map_id,
                                              &image_addr,
                                              (void*)&data_ptr,
                                              VX_WRITE_ONLY,
                                              VX_MEMORY_TYPE_HOST,
                                              TIVX_RAW_IMAGE_PIXEL_BUFFER);

                //printf("width= %d, height=%d, stride= %d\n", image_addr.dim_x, image_addr.dim_y, image_addr.stride_y);
                if (status == VX_SUCCESS)
                {
                    status = obj->visionStreamObj.open(&obj->visionStreamObj, channel);

                    if (status == VX_FAILURE)
                    {
                        printf("[STREAMING MODULE ERROR], Unable to open file for input %d\n", channel);
                    }
                    else
                    {
                        for (int32_t h = 0; h < height; h++)
                        {
                            status = obj->visionStreamObj.read(&obj->visionStreamObj, channel, data_ptr, width*num_bytes, &numBytesRead);
                            if (status == VX_FAILURE)
                            {
                                printf("[STREAMING MODULE ERROR], unable to read raw images, expected = %d bytes. Read %d bytes.\n", width * num_bytes, numBytesRead);
                                break;
                            }
                            data_ptr+= image_addr.stride_y;
                        }
                        obj->visionStreamObj.close(&obj->visionStreamObj, channel);
                    }
                    tivxUnmapRawImagePatch(inRawFrame, map_id);
                }
                else
                {
                    printf("[STREAMING MODULE ERROR], Error mapping raw image buffer\n");
                }
                tivxReleaseRawImage(&inRawFrame);
            }
            else
            {
                printf("[STREAMING MODULE ERROR], Error obtaining raw image buffer\n");
            }
        }
        // Advance to next file
        obj->visionStreamObj.next(&obj->visionStreamObj);
    }

    return status;
}

void app_deinit_streaming(StreamingObj *obj, int32_t bufq_depth)
{

    inputStreamDeInit(&obj->visionStreamObj);

    for (int32_t i = 0; i < bufq_depth; i++)
    {
        vxReleaseObjectArray(&obj->raw_image_arr[i]);
        tivxReleaseRawImage(&obj->raw_image_ch0[i]);
    }
}