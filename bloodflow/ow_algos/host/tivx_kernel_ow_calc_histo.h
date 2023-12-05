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

#ifndef _TIVX_KERNEL_OW_CALC_HISTO_
#define _TIVX_KERNEL_OW_CALC_HISTO_

#include <TI/tivx_obj_desc.h>
#include <TI/tivx_target_kernel.h>

#ifdef __cplusplus
extern "C" {
#endif


#define TIVX_KERNEL_OW_CALC_HISTO_CONFIG_IDX (0U)
#define TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX  (1U)
#define TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX  (2U)
#define TIVX_KERNEL_OW_CALC_HISTO_MEAN_IDX   (3U)
#define TIVX_KERNEL_OW_CALC_HISTO_SD_IDX     (4U)

#define TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS (5U)

#define TIVX_C66_HISTO_CMD_ENABLE_CHAN 0
#define TIVX_C66_HISTO_CMD_CHANGE_PAIR 1
#define TIVX_C66_HISTO_TIMESTAMP_SYNC 2
#define TIVX_C66_HISTO_CMD_SET_DSP_CORE_1 3
#define TIVX_C66_HISTO_CMD_SET_DSP_CORE_2 4

#define TIVX_C66_HISTO_MAX_NUM_CHANNELS 8
#define TIVX_C66_HISTO_MAX_NUM_CHANNELS_ENABLED_AT_ONCE 2
#define TIVX_C66_HISTO_MAX_NUM_CHANNELS_ENABLED_AT_ONCE_MASK ((1 << TIVX_C66_HISTO_MAX_NUM_CHANNELS_ENABLED_AT_ONCE) - 1)

#define C66_HISTO_SERVICE_NAME "com.ti.c66_histo_pipeline"

typedef enum
{
    C66_DSP_INIT_HISTO = 0,
    C66_DSP_COMPUTE_HISTO,
    C66_DSP_DEINIT_HISTO
} app_c66_dsp_cmd_t;

typedef struct {
    app_c66_dsp_cmd_t cmd;
    tivx_obj_desc_t **objDesc;
    tivx_obj_desc_raw_image_t *inDesc;
    tivx_obj_desc_distribution_t *outDesc;
    tivx_obj_desc_scalar_t *dark_mean;
    void *inPtr;
    void *outPtr;
    
    uint32_t dspCore;
} app_c66_histo_msg_t;

typedef struct {
    uint32_t enableChanBitFlag;
} tivxC66HistoParams;

typedef struct {
    uint32_t enableChanBitFlag;
} tivxC66HistoCmd;

typedef struct {
    uint64_t refTimestamp;
} tivxC66HistoTimeSync;

vx_status appC66HistoCreate(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);

vx_status VX_CALLBACK appC66HistoProcess(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);

vx_status appC66HistoDelete(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);

#ifdef __cplusplus
}
#endif


#endif /* _TIVX_KERNEL_OW_CALC_HISTO_ */
