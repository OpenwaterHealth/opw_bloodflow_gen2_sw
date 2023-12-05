/*
 *
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>

#include "TI/tivx.h"
#include "TI/tivx_ow_algos.h"
#include "tivx_ow_algos_kernels_priv.h"
#include "tivx_kernel_ow_calc_histo.h"
#include "TI/tivx_target_kernel.h"

static vx_kernel vx_ow_calc_histo_kernel = NULL;

static vx_status VX_CALLBACK tivxAddKernelOwCalcHistoValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[]);
/*
static vx_status VX_CALLBACK tivxAddKernelOwCalcHistoInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params);
*/
vx_status tivxAddKernelOwCalcHisto(vx_context context);
vx_status tivxRemoveKernelOwCalcHisto(vx_context context);

static vx_status VX_CALLBACK tivxAddKernelOwCalcHistoValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[])
{
    vx_status status = (vx_status)VX_SUCCESS;

    //vx_image input = NULL;
    //vx_df_image input_fmt;
    //tivx_raw_image input;
    vx_enum ref_type;
    vx_int32 offset = 0;
    vx_uint32 range = 0;
    vx_size numBins = 0;

    vx_distribution histo = NULL;

    if( num != TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS )
    {
        status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "Invalid number (%d) of parameters, expected (%d)\n", num, TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS);
    }

    if( status == VX_SUCCESS )
    {
        for(int i = 0; i < TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS; i++)
        {
            if( parameters[i] == NULL )
            {
                status = (vx_status)VX_ERROR_NO_MEMORY;
                VX_PRINT(VX_ZONE_ERROR, "parameter[%d] is set to NULL\n", i);
                break;
            }
        }
    }

    if ((vx_status)VX_SUCCESS == status)
    {
        //input = (tivx_raw_image)parameters[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX];
        histo = (vx_distribution)parameters[TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX];
    }

    if (VX_SUCCESS == status)
    {
        /* Get the image width/heigh and format */
        status = vxQueryReference(parameters[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX], (vx_enum)VX_REFERENCE_TYPE, &ref_type, sizeof(ref_type));

        if( status != VX_SUCCESS )
        {
            printf("ERROR: Unable to query input image !!!\n");
        }
    }

    if (VX_SUCCESS == status)
    {
        /* Get the distribution attributes */
        status = vxQueryDistribution(histo, (vx_enum)VX_DISTRIBUTION_BINS, &numBins, sizeof(numBins));
        status |= vxQueryDistribution(histo, (vx_enum)VX_DISTRIBUTION_RANGE, &range, sizeof(range));
        status |= vxQueryDistribution(histo, (vx_enum)VX_DISTRIBUTION_OFFSET, &offset, sizeof(offset));

        if(status!=VX_SUCCESS)
        {
            printf("ERROR: Unable to query output histogram !!!\n");
        }
    }

    if (VX_SUCCESS == status)
    {
        /* Check for validity of data format */
        if (TIVX_TYPE_RAW_IMAGE != ref_type)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            printf(" C66x Histogram kernel ERROR: Input image format not correct !!!\n");
        }
        if (numBins > range) 
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            printf(" C66x Histogram kernel ERROR: Histogram number of bins %u greater than range %d !!!\n",
                    (uint32_t)numBins, range);
        }
        if (offset != 0) 
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            printf(" C66x Histogram kernel ERROR: Histogram's distribution offset not equal to 0 !!!\n");
        }
    }

    if (VX_SUCCESS == status)
    {
        vx_enum scalar_type;
        vxQueryScalar((vx_scalar)parameters[TIVX_KERNEL_OW_CALC_HISTO_MEAN_IDX], (vx_enum)VX_SCALAR_TYPE, &scalar_type, sizeof(scalar_type));
        if ((vx_enum)VX_TYPE_UINT32 != scalar_type)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            printf(" C66x Histogram kernel ERROR: mean scalar type not float32 !!!\n");
        }
        vxQueryScalar((vx_scalar)parameters[TIVX_KERNEL_OW_CALC_HISTO_SD_IDX], (vx_enum)VX_SCALAR_TYPE, &scalar_type, sizeof(scalar_type));
        if ((vx_enum)VX_TYPE_UINT32 != scalar_type)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            printf(" C66x Histogram kernel ERROR: sd scalar type not float64 !!!\n");
        }
    }

    #if 0
    /* PARAMETER ATTRIBUTE FETCH */

    if ((vx_status)VX_SUCCESS == status)
    {
        tivxCheckStatus(&status, vxQueryImage(input, (vx_enum)VX_IMAGE_FORMAT, &input_fmt, sizeof(input_fmt)));

    }

    /* PARAMETER CHECKING */

    if ((vx_status)VX_SUCCESS == status)
    {
        vxQueryDistribution(histo, VX_DISTRIBUTION_BINS, &numBins, sizeof(numBins));
        vxQueryDistribution(histo, VX_DISTRIBUTION_RANGE, &range, sizeof(range));
        vxQueryDistribution(histo, VX_DISTRIBUTION_OFFSET, &offset, sizeof(offset));
        /* < DEVELOPER_TODO: Replace <Add type here> with correct data type > */
        if ((vx_df_image)VX_DF_IMAGE_U16 != input_fmt)
        {
            status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'input' should be an image of type:\n VX_DF_IMAGE_U16 \n");
        }

    }
    #endif //0

    /* CUSTOM PARAMETER CHECKING */

    /* < DEVELOPER_TODO: (Optional) Add any custom parameter type or range checking not */
    /*                   covered by the code-generation script.) > */

    /* < DEVELOPER_TODO: (Optional) If intending to use a virtual data object, set metas using appropriate TI API. */
    /*                   For a code example, please refer to the validate callback of the follow file: */
    /*                   tiovx/kernels/openvx-core/host/vx_absdiff_host.c. For further information regarding metas, */
    /*                   please refer to the OpenVX 1.1 spec p. 260, or search for vx_kernel_validate_f. > */

    return status;
}

#if 0
static vx_status VX_CALLBACK tivxAddKernelOwCalcHistoInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params)
{
    vx_status status = (vx_status)VX_SUCCESS;
    tivxKernelValidRectParams prms;

    if ( (num_params != TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS)
        || (NULL == parameters[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX])
        || (NULL == parameters[TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX])
    )
    {
        status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "One or more REQUIRED parameters are set to NULL\n");
    }
    if ((vx_status)VX_SUCCESS == status)
    {
        tivxKernelValidRectParams_init(&prms);

        prms.in_img[0U] = (vx_image)parameters[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX];

        prms.num_input_images = 1;
        prms.num_output_images = 0;

        /* < DEVELOPER_TODO: (Optional) Set padding values based on valid region if border mode is */
        /*                    set to VX_BORDER_UNDEFINED and remove the #if 0 and #endif lines. */
        /*                    Else, remove this entire #if 0 ... #endif block > */
        #if 0
        prms.top_pad = 0;
        prms.bot_pad = 0;
        prms.left_pad = 0;
        prms.right_pad = 0;
        prms.border_mode = VX_BORDER_UNDEFINED;
        #endif

        status = tivxKernelConfigValidRect(&prms);
    }

    return status;
}
#endif  //0

vx_status tivxAddKernelOwCalcHisto(vx_context context)
{
    vx_kernel kernel;
    vx_status status;
    uint32_t index;
    vx_enum kernel_id;

    printf("==========> tivxAddKernelOwCalcHisto \n");

    status = vxAllocateUserKernelId(context, &kernel_id);
    if(status != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Unable to allocate user kernel ID\n");
    }

    if (status == (vx_status)VX_SUCCESS)
    {
        kernel = vxAddUserKernel(
                    context,
                    TIVX_KERNEL_OW_CALC_HISTO_NAME,
                    kernel_id,
                    NULL,
                    TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS,
                    tivxAddKernelOwCalcHistoValidate,
                    NULL,                                   //tivxAddKernelOwCalcHistoInitialize,
                    NULL);

        status = vxGetStatus((vx_reference)kernel);
    }
    if (status == (vx_status)VX_SUCCESS)
    {
        index = 0;

        status = vxAddParameterToKernel(kernel,
                    index,
                    VX_INPUT,
                    VX_TYPE_USER_DATA_OBJECT,
                    VX_PARAMETER_STATE_REQUIRED
                 );
        index++;

        if( status == VX_SUCCESS )
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        (vx_enum)VX_INPUT,
                        (vx_enum)TIVX_TYPE_RAW_IMAGE,               // (vx_enum)VX_TYPE_IMAGE,
                        (vx_enum)VX_PARAMETER_STATE_REQUIRED
                     );
            index++;
        }
        if (status == (vx_status)VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        (vx_enum)VX_OUTPUT,
                        (vx_enum)VX_TYPE_DISTRIBUTION,
                        (vx_enum)VX_PARAMETER_STATE_REQUIRED
                     );
            index++;
        }
        if ( status == VX_SUCCESS )
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        VX_OUTPUT,
                        VX_TYPE_SCALAR,
                        VX_PARAMETER_STATE_REQUIRED
                     );
            index++;
        }
        if ( status == VX_SUCCESS )
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        VX_OUTPUT,
                        VX_TYPE_SCALAR,
                        VX_PARAMETER_STATE_REQUIRED
                     );
            index++;
        }

        if (status == (vx_status)VX_SUCCESS)
        {
            /* add supported target's */
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP1);
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP2);
        }
        if (status == (vx_status)VX_SUCCESS)
        {
            status = vxFinalizeKernel(kernel);
        }
        if (status != (vx_status)VX_SUCCESS)
        {
            vxReleaseKernel(&kernel);
            kernel = NULL;
        }
    }
    else
    {
        kernel = NULL;
    }
    vx_ow_calc_histo_kernel = kernel;

    return status;
}

vx_status tivxRemoveKernelOwCalcHisto(vx_context context)
{
    vx_status status;
    vx_kernel kernel = vx_ow_calc_histo_kernel;
    
    printf("==========> tivxRemoveKernelOwCalcHisto \n");

    status = vxRemoveKernel(kernel);
    vx_ow_calc_histo_kernel = NULL;

    return status;
}