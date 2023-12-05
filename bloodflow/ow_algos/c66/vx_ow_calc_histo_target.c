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

#include "TI/tivx.h"
#include "TI/tivx_ow_algos.h"
#include "VX/vx.h"
#include "tivx_ow_algos_kernels_priv.h"
#include "tivx_kernel_ow_calc_histo.h"
#include "TI/tivx_target_kernel.h"
#include "tivx_kernels_target_utils.h"
#include <TI/tivx_task.h>
//#include <ti/sysbios/family/c66/Cache.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <utils/udma/include/app_udma.h>
#include <utils/mem/include/app_mem.h>
#include <utils/ipc/include/app_ipc.h>
#include <utils/remote_service/include/app_remote_service.h>

#include "IMG_histogram_16_nl.h"

#ifndef x86_64
#include <ti/osal/HwiP.h>
//#define DISABLE_INTERRUPTS_DURING_PROCESS
#endif

#define EXEC_PIPELINE_STAGE1(x) ((x) & 0x00000001)
#define EXEC_PIPELINE_STAGE2(x) ((x) & 0x00000002)
#define EXEC_PIPELINE_STAGE3(x) ((x) & 0x00000004)

#define J721E_C66X_DSP_L2_SRAM_SIZE  (224 * 1024) /* size in bytes */

#define PROFILE_WITH_C6000_COUNTER 0

#include <c6x.h>


/* < DEVELOPER_TODO: Uncomment if kernel context is needed > */

#define C66X_CYCLE_DURATION_SECS    (1.0 / (1.35 * 1000000000.0))

#define NUM_SCRATCH_BUFS    4
#define BITS_PER_PIXEL      10
#define TARGET_HISTO_BUF_SIZE ( (1 << BITS_PER_PIXEL) * sizeof (uint32_t))
#define SCRATCH_BUFFER_SIZE ( ((1 << BITS_PER_PIXEL) *  NUM_SCRATCH_BUFS) * sizeof(uint32_t) )   // 4096 for target histo buffer

typedef enum {
    DATA_COPY_CPU = 0,
    DATA_COPY_DMA = 1,
    DATA_COPY_DEFAULT = DATA_COPY_CPU

} DataCopyStyle;

typedef struct {

    app_udma_ch_handle_t udmaChHdl;
    app_udma_copy_nd_prms_t tfrPrms;

    vx_uint32 transfer_type;

    vx_uint32 icnt1_next;
    vx_uint32 icnt2_next;
    vx_uint32 icnt3_next;

    vx_uint32 dma_ch;

}DMAObj;

typedef struct
{
    vx_int32 ch_num;
    uint32_t enableChanBitFlag;

    DMAObj inDma;
    DMAObj outDma;

    vx_uint32 blkWidth;
    vx_uint32 blkHeight;
    vx_uint32 remHeight;
    vx_uint32 numSets;

    vx_uint32 l2_req_size;
    vx_uint32 l2_avail_size;
    vx_uint32 l1_req_size;
    vx_uint32 l1_avail_size;

    uint32_t *pL1;
    uint32_t l1_heap_id;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
    uint64_t l1d_global_base;
    uint16_t *pL2;
    uint32_t l2_heap_id;
    uint64_t l2_global_base;

    uint32_t numHist;
    uint32_t numBins;

    uint64_t ref_ts;
    uint64_t ref_tick;

    uint32_t *scratch_mem_out;
    uint32_t *out_histo_mem;
    uint32_t (* restrict hist_2d)[4];

} tivxOwCalcHistoParams;

static vx_int32 inst_id = 0;

void tivxSetSelfCpuId(vx_enum cpu_id);

static tivx_target_kernel vx_ow_calc_histo_target_kernel = NULL;

static vx_status VX_CALLBACK tivxOwCalcHistoProcess(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);
static vx_status VX_CALLBACK tivxOwCalcHistoCreate(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);
static vx_status VX_CALLBACK tivxOwCalcHistoDelete(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);
static vx_status VX_CALLBACK tivxOwCalcHistoControl(
       tivx_target_kernel_instance kernel,
       uint32_t node_cmd_id, tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg);

static vx_status dma_create(DMAObj *dmaObj, vx_size transfer_type, vx_uint32 dma_ch)
{
    vx_status status = VX_SUCCESS;

    dmaObj->transfer_type = transfer_type;

    memset(&dmaObj->tfrPrms, 0, sizeof(app_udma_copy_nd_prms_t));

    dmaObj->icnt1_next = 0;
    dmaObj->icnt2_next = 0;
    dmaObj->icnt3_next = 0;

    dmaObj->dma_ch = dma_ch;

    if(transfer_type == DATA_COPY_DMA)
    {
#ifndef x86_64
        dmaObj->udmaChHdl = appUdmaCopyNDGetHandle(dma_ch);
#endif
    }
    else
    {
        dmaObj->udmaChHdl = NULL;
    }

    //printf("dmaObj->udmaChHdl: %p\n", dmaObj->udmaChHdl);
    return status;
}

static vx_status dma_transfer_trigger(DMAObj *dmaObj)
{
    vx_status status = VX_SUCCESS;

    if(dmaObj->transfer_type == DATA_COPY_DMA)
    {
        //printf("dma src: 0x%llx, dma dst: 0x%llx\n", dmaObj->tfrPrms.src_addr, dmaObj->tfrPrms.dest_addr);

#ifndef x86_64
        appUdmaCopyNDTrigger(dmaObj->udmaChHdl);
#endif
    }
    else
    {
        app_udma_copy_nd_prms_t *tfrPrms;
        vx_uint32 icnt1, icnt2, icnt3;

        tfrPrms = (app_udma_copy_nd_prms_t *)&dmaObj->tfrPrms;

        /* This is for case where for every trigger ICNT0 * ICNT1 bytes get transferred */
        icnt3 = dmaObj->icnt3_next;
        icnt2 = dmaObj->icnt2_next;
        icnt1 = dmaObj->icnt1_next;

        /* As C66 is a 32b processor, address will be truncated to use only lower 32b address */
        /* So user is responsible for providing correct address */
#ifndef x86_64
        vx_uint8 *pSrcNext = (vx_uint8 *)((uint32_t)tfrPrms->src_addr + (icnt3 * tfrPrms->dim3) + (icnt2 * tfrPrms->dim2));
        vx_uint8 *pDstNext = (vx_uint8 *)((uint32_t)tfrPrms->dest_addr + (icnt3 * tfrPrms->ddim3) + (icnt2 * tfrPrms->ddim2));
#else
        vx_uint8 *pSrcNext = (vx_uint8 *)(tfrPrms->src_addr + (icnt3 * tfrPrms->dim3) + (icnt2 * tfrPrms->dim2));
        vx_uint8 *pDstNext = (vx_uint8 *)(tfrPrms->dest_addr + (icnt3 * tfrPrms->ddim3) + (icnt2 * tfrPrms->ddim2));
#endif
        for(icnt1 = 0; icnt1 < tfrPrms->icnt1; icnt1++)
        {
            memcpy(pDstNext, pSrcNext, tfrPrms->icnt0);

            pSrcNext += tfrPrms->dim1;
            pDstNext += tfrPrms->ddim1;
        }

        icnt2++;

        if(icnt2 == tfrPrms->icnt2)
        {
            icnt2 = 0;
            icnt3++;
        }

        dmaObj->icnt3_next = icnt3;
        dmaObj->icnt2_next = icnt2;
    }

    return status;
}

static vx_status dma_transfer_wait(DMAObj *dmaObj)
{
    vx_status status = VX_SUCCESS;

    if(dmaObj->transfer_type == DATA_COPY_DMA)
    {
#ifndef x86_64
        appUdmaCopyNDWait(dmaObj->udmaChHdl);
#endif
    }

    return status;
}

static vx_status dma_init(DMAObj *dmaObj)
{
    vx_status status = VX_SUCCESS;

    if(dmaObj->transfer_type == DATA_COPY_DMA)
    {
#ifndef x86_64
        appUdmaCopyNDInit(dmaObj->udmaChHdl, &dmaObj->tfrPrms);
#endif
    }
    else
    {
        dmaObj->icnt1_next = 0;
        dmaObj->icnt2_next = 0;
        dmaObj->icnt3_next = 0;
    }

    return status;
}

static vx_status dma_deinit(DMAObj *dmaObj)
{
    vx_status status = VX_SUCCESS;

    if(dmaObj->transfer_type == DATA_COPY_DMA)
    {
#ifndef x86_64
        appUdmaCopyNDDeinit(dmaObj->udmaChHdl);
#endif
    }
    else
    {
        dmaObj->icnt1_next = 0;
        dmaObj->icnt2_next = 0;
        dmaObj->icnt3_next = 0;
    }
    return status;
}

static vx_status dma_delete(DMAObj *dmaObj)
{
    vx_status status = VX_SUCCESS;

    dmaObj->udmaChHdl = NULL;

    if(dmaObj->transfer_type == DATA_COPY_DMA)
    {
#ifndef x86_64
        int32_t retVal = appUdmaCopyNDReleaseHandle(dmaObj->dma_ch);
        if(retVal != 0)
        {
            VX_PRINT(VX_ZONE_ERROR, "Unable to release DMA handle %d\n", dmaObj->dma_ch);
            status = VX_FAILURE;
        }
#endif
    }

    return status;
}

static vx_status histo_dma_setup_input_image(tivxOwCalcHistoParams *prms,
                             DMAObj *in_dmaObj,
                             vx_uint16 *pL2,
                             tivx_obj_desc_raw_image_t *image_desc,         //tivx_obj_desc_image_t *image_desc,
                             void *img_target_ptr)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 sizeOfPix = 2;
    app_udma_copy_nd_prms_t *tfrPrms = (app_udma_copy_nd_prms_t *)&in_dmaObj->tfrPrms;
    vx_imagepatch_addressing_t *pImage = (vx_imagepatch_addressing_t *)&image_desc->imagepatch_addr[0];
    vx_uint32 img_width  = pImage->dim_x;
    vx_int32  img_stride = pImage->stride_y;
    vx_uint8 *pDDR = NULL;

    //printf("Image Src addr: %p, Image Dst addr: 0x%llx\n", img_target_ptr, ((uintptr_t)pL2) + prms->l2_global_base);

    vx_uint32 rawImageBitDepth;

    rawImageBitDepth = image_desc->params.format[0].msb + 1;
    if (rawImageBitDepth <= 8)
    {
        sizeOfPix = 1;
    }
    else if (rawImageBitDepth <= 16)
    {
        sizeOfPix = 2;
    }
    else
    {
        printf("Raw image bit depth exceeds 16 bits !\n");
        status = VX_FAILURE;
    }

    pDDR = (vx_uint8 *)img_target_ptr;

    tfrPrms->copy_mode    = 2;
    tfrPrms->eltype       = sizeof(uint8_t);

    tfrPrms->dest_addr    = (uintptr_t)pL2 + prms->l2_global_base;
    tfrPrms->src_addr     = (uint64_t)pDDR;

    tfrPrms->icnt0        = img_width * sizeOfPix;
    tfrPrms->icnt1        = prms->blkHeight;
    tfrPrms->icnt2        = 2;
    tfrPrms->icnt3        = prms->numSets / 2;
    tfrPrms->dim1         = img_stride;
    tfrPrms->dim2         = (prms->blkHeight * img_stride);
    tfrPrms->dim3         = (prms->blkHeight * img_stride * 2);

    tfrPrms->dicnt0       = tfrPrms->icnt0;
    tfrPrms->dicnt1       = tfrPrms->icnt1;
    tfrPrms->dicnt2       = 2; /* Ping-pong */
    tfrPrms->dicnt3       = prms->numSets / 2;
    tfrPrms->ddim1        = prms->blkWidth * sizeOfPix;
    tfrPrms->ddim2        = (prms->blkHeight * prms->blkWidth) * sizeOfPix;
    tfrPrms->ddim3        = 0;

    dma_init(in_dmaObj);

    #if 0
    printf("Input Image DMA setup:\n");
    printf("tfrPrms->src_addr: 0x%llx, tfrPrms->dest_addr: 0x%llx\n", tfrPrms->src_addr, tfrPrms->dest_addr);
    printf("tfrPrms->icnt0: %d, tfrPrms->icnt1: %d, tfrPrms->icnt2: %d, tfrPrms->icnt3: %d\n",
             tfrPrms->icnt0, tfrPrms->icnt1, tfrPrms->icnt2, tfrPrms->icnt3);
    printf("tfrPrms->dim1: %d, tfrPrms->dim2: %d, tfrPrms->dim3: %d\n",
             tfrPrms->dim1, tfrPrms->dim2, tfrPrms->dim3);
    printf("tfrPrms->dicnt0: %d, tfrPrms->dicnt1: %d, tfrPrms->dicnt2: %d, tfrPrms->dicnt3: %d\n",
             tfrPrms->dicnt0, tfrPrms->dicnt1, tfrPrms->dicnt2, tfrPrms->dicnt3);
    printf("tfrPrms->ddim1: %d, tfrPrms->ddim2: %d, tfrPrms->ddim3: %d\n",
             tfrPrms->ddim1, tfrPrms->ddim2, tfrPrms->ddim3);
    #endif

    return status;
}

static vx_status histo_dma_setup_output_image(tivxOwCalcHistoParams *prms,
                             DMAObj *out_dmaObj,
                             vx_uint32 *pL1,
                             tivx_obj_desc_distribution_t *dst_desc,
                             void *hist_target_ptr)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 numBins = dst_desc->num_bins;
    vx_uint32 sizeOfBin = 4;
    app_udma_copy_nd_prms_t *tfrPrms = (app_udma_copy_nd_prms_t *)&out_dmaObj->tfrPrms;
    vx_uint8 *pDDR = NULL;

    /*
    printf("Histo Src addr: 0x%llx, Histo Dst addr: %p\n",
            (uintptr_t)prms->out_histo_mem + prms->l2_global_base, hist_target_ptr);
    */

    pDDR = (vx_uint8 *)hist_target_ptr;
    // one-shot transfer to write out the histogram
    tfrPrms->copy_mode    = 2;
    tfrPrms->eltype       = sizeof(uint8_t);

    tfrPrms->dest_addr    = (uint64_t)pDDR;
    tfrPrms->src_addr     = (uintptr_t)prms->out_histo_mem + prms->l2_global_base;

    tfrPrms->icnt0        = numBins*sizeOfBin;
    tfrPrms->icnt1        = 1;
    tfrPrms->icnt2        = 1;
    tfrPrms->icnt3        = 1;
    tfrPrms->dim1         = numBins*sizeOfBin;
    tfrPrms->dim2         = numBins*sizeOfBin;
    tfrPrms->dim3         = 0;

    tfrPrms->dicnt0       = tfrPrms->icnt0;
    tfrPrms->dicnt1       = tfrPrms->icnt1;
    tfrPrms->dicnt2       = tfrPrms->icnt2; /* No Ping-pong */
    tfrPrms->dicnt3       = tfrPrms->icnt3;
    tfrPrms->ddim1        = numBins*sizeOfBin;
    tfrPrms->ddim2        = numBins*sizeOfBin;
    tfrPrms->ddim3        = 0;

    //Cache_setMar((Ptr)pDDR, tfrPrms->icnt0, Cache_Mar_Disable);

    dma_init(out_dmaObj);

#if 0
    printf("Output Histogram DMA setup:\n");
    printf("tfrPrms->src_addr: 0x%llx, tfrPrms->dest_addr: 0x%llx\n", tfrPrms->src_addr, tfrPrms->dest_addr);
    printf("tfrPrms->icnt0: %d, tfrPrms->icnt1: %d, tfrPrms->icnt2: %d, tfrPrms->icnt3: %d\n",
             tfrPrms->icnt0, tfrPrms->icnt1, tfrPrms->icnt2, tfrPrms->icnt3);
    printf("tfrPrms->dim1: %d, tfrPrms->dim2: %d, tfrPrms->dim3: %d\n",
             tfrPrms->dim1, tfrPrms->dim2, tfrPrms->dim3);
    printf("tfrPrms->dicnt0: %d, tfrPrms->dicnt1: %d, tfrPrms->dicnt2: %d, tfrPrms->dicnt3: %d\n",
             tfrPrms->dicnt0, tfrPrms->dicnt1, tfrPrms->dicnt2, tfrPrms->dicnt3);
    printf("tfrPrms->ddim1: %d, tfrPrms->ddim2: %d, tfrPrms->ddim3: %d\n",
             tfrPrms->ddim1, tfrPrms->ddim2, tfrPrms->ddim3);
#endif

    return status;
}

static vx_status histo_dma_setup(tivxOwCalcHistoParams *prms,
                             tivx_obj_desc_raw_image_t *in_img_desc,        // tivx_obj_desc_image_t *in_img_desc,
                             tivx_obj_desc_distribution_t *out_histo_desc,
                             void *in_img_target_ptr,
                             void *out_hist_target_ptr)
{
    vx_status status = VX_SUCCESS;
    vx_uint16 *pImgDstL2[2] = {NULL};
    vx_uint32 *pHistSrc = NULL;

    vx_uint32 numBins = out_histo_desc->num_bins;
    // printf("histo_dma_setup ......\n");

    /* Setting first image pointer to the base of L2 then the second buffer
       directly following the pointer location. */
    pImgDstL2[0] = (vx_uint16 *)prms->pL2;
    pImgDstL2[1] = (vx_uint16 *)pImgDstL2[0] + (prms->blkWidth * prms->blkHeight);

    pHistSrc  = (vx_uint32 *)prms->out_histo_mem;

    //printf("pImgDstL2[0]: %p, pImgDstL2[1]: %p, pHistSrc: %p\n", pImgDstL2[0], pImgDstL2[1], pHistSrc);

    /* Setting up input image dma */    
    // printf("histo_dma_setup_input_image\n");
    status = histo_dma_setup_input_image(prms, &prms->inDma, pImgDstL2[0], in_img_desc, in_img_target_ptr);

    if (VX_SUCCESS == status)
    {
        /* Setting up output histo dma */
        // printf("histo_dma_setup_output_image\n");
        status = histo_dma_setup_output_image(prms, &prms->outDma, pHistSrc, out_histo_desc, out_hist_target_ptr);
    }
    else
    {
        printf("dma setup for input image failed ......\n");
    }

    return status;
}

static vx_status histo_dma_teardown(tivxOwCalcHistoParams *prms)
{
    vx_status status = VX_SUCCESS;
    // printf("histo_dma_teardown ......\n");

    dma_deinit(&prms->inDma);
    dma_deinit(&prms->outDma);

    return status;
}

static vx_status histo_pipeline_blocks
(
    tivxOwCalcHistoParams *prms,
    tivx_obj_desc_raw_image_t *in_img_desc,
    tivx_obj_desc_distribution_t *out_histo_desc,
    void *img_in_ptr,
    void *histo_out_ptr,
    void *dark_mean_ptr,
    int  skiprows
)
{
    vx_status status = VX_SUCCESS;

    int num_input_pixels = 0;

    vx_uint32 pipeup = 2;
    vx_uint32 pipedown = 2;
    vx_uint32 exec_cmd = 1;
    vx_uint32 pipeline = exec_cmd;
    vx_uint32 ping_npong = 0;
    uint32_t  blk;
    uint32_t dsum = 0;
    float32_t dmean = 0.0f;

    vx_uint16 *pImgDstL2[2] = {NULL};
    vx_uint32 *pHistSrc = NULL;

    int skipCnt = skiprows / prms->blkHeight;

    /* Setting first image buffer pointer to the base of L2 (ping),
       then the second image buffer (pong) directly following the first buffer. */
    pImgDstL2[0] = (vx_uint16 *)prms->pL2;
    pImgDstL2[1] = (vx_uint16 *)pImgDstL2[0] + (prms->blkWidth * prms->blkHeight);

    /* Set the histogram output buffer following the image ping-n-pong buffers.
       And, there is no ping-pong for histogram! */
    pHistSrc= (vx_uint32 *)((vx_uint16 *)pImgDstL2[1] + (prms->blkWidth * prms->blkHeight));

    num_input_pixels = prms->blkWidth * prms->blkHeight;

    //printf("pImgDstL2[0]: %p, pImgDstL2[1]: %p, num_input_pixels: %d\n",
    //        pImgDstL2[0], pImgDstL2[1], num_input_pixels);

    #if PROFILE_WITH_C6000_COUNTER
    uint64_t pf_start_time, pf_end_time, pf_overhead, pf_cycle_count;
    double pf_time_elapsed_secs;

    TSCL = 0;       //enable TSC
    pf_start_time = _itoll(TSCH, TSCL);
    pf_end_time = _itoll(TSCH, TSCL);
    pf_overhead = pf_end_time - pf_start_time;

    pf_start_time = _itoll(TSCH, TSCL);
    #endif

    for(blk = 0; blk < (prms->numSets + pipeup + pipedown); blk++)
    {
        if(EXEC_PIPELINE_STAGE1(pipeline))
        {
            dma_transfer_trigger(&prms->inDma);
        }

        if(EXEC_PIPELINE_STAGE2(pipeline))
        {
            /*
            int i;
            if( (blk - 1) % 10 == 0 )
            {

                uint16_t *tptr = (uint16_t *)((uintptr_t)(prms->inDma.tfrPrms.src_addr + ((blk - 1) * prms->inDma.tfrPrms.icnt0)));
                for( i = 0; i < 4; i++ )
                {
                    printf("blk: %d, input pixel[%d] right before calling histogram func: 0x%x\n", (blk - 1), i, tptr[i]);
                }
            }
            */

            if( skipCnt > 0 )
            {
                dsum += IMG_dark_rows_sum((uint16_t *)pImgDstL2[ping_npong], num_input_pixels);
                //printf("dsum: %d\n", dsum);
                skipCnt--;
            }
            else
            {
                // call histogram compute function
                IMG_histogram_16_cn_local
                (
                    (uint16_t *)pImgDstL2[ping_npong],  /* input image data                */
                    num_input_pixels,   /* number of input image pixels    */
                    1,  /* define add/sum for histogtram   */
                    prms->hist_2d,  /* temporary histogram bins        */
                    (uint32_t *)pHistSrc,  /* running histogram bins          */
                    10,   /* number of data bits per pixel   */
                    (blk - 1)
                );

            }
            ping_npong = !ping_npong;
        }

        if(EXEC_PIPELINE_STAGE1(pipeline))
        {
            dma_transfer_wait(&prms->inDma);
        }

        if(blk == (prms->numSets - 1))
        {
            exec_cmd = 0;
        }

        pipeline = (pipeline << 1) | exec_cmd;
    }

    int32_t bin = 0;

    // aggregate histogram output from scratch bins!
    for (bin = 0; bin < out_histo_desc->num_bins; bin++)
    {
        // Add code to accumulate and copy scratch histo bins here
        pHistSrc[bin] = prms->hist_2d[bin][0]
                        + prms->hist_2d[bin][1]
                        + prms->hist_2d[bin][2]
                        + prms->hist_2d[bin][3];

        /*
        if( bin >= 600 && bin <= 750 )
        {
            printf("pHistSrc[%d]: %d\n", bin, pHistSrc[bin]);
        }
        */
    }
    
    // calculate mean on dark rows!
    dmean = (float32_t)dsum / (float32_t)(prms->blkWidth * skiprows);
    
    #if PROFILE_WITH_C6000_COUNTER
    pf_end_time = _itoll(TSCH, TSCL);
    pf_cycle_count = pf_end_time - pf_start_time - pf_overhead;
    pf_time_elapsed_secs = (double)pf_cycle_count * C66X_CYCLE_DURATION_SECS;

    printf("Histogram computation took: %lld CPU cycles or %f millisecs\n",
            pf_cycle_count, (pf_time_elapsed_secs * 1000));
    #endif

    // transfer dark mean value to host memory
    memcpy(dark_mean_ptr, (void *)&dmean, sizeof(float32_t));

    // Transfer the histogram buffer to DDR!
    dma_transfer_trigger(&prms->outDma);

    dma_transfer_wait(&prms->outDma);

    //printf("Hist Out srcaddr: 0x%llx, Hist Out dstaddr: 0x%llx\n", prms->outDma.tfrPrms.src_addr, prms->outDma.tfrPrms.dest_addr);
    
    #if PROFILE_WITH_C6000_COUNTER
    pf_end_time = _itoll(TSCH, TSCL);
    pf_cycle_count = pf_end_time - pf_start_time - pf_overhead;
    pf_time_elapsed_secs = (double)pf_cycle_count * C66X_CYCLE_DURATION_SECS;

    printf("Histogram computation + out DMA took: %lld CPU cycles or %f millisecs\n",
            pf_cycle_count, (pf_time_elapsed_secs * 1000));
    #endif

    return status;
}

static vx_status histo_run(tivxOwCalcHistoParams *prms,
                           tivx_obj_desc_raw_image_t *in_img_desc,      // tivx_obj_desc_image_t *in_img_desc,
                           void *srcPtr,
                           tivx_obj_desc_distribution_t *out_histo_desc,
                           void *dstPtr,
                           void *dmean)
{
    vx_status status = VX_SUCCESS;
    int skiprows = 20;

    // printf("histo_dma_setup\n");
    status = histo_dma_setup(prms, in_img_desc, out_histo_desc, srcPtr, dstPtr);
    if( status == VX_SUCCESS )
    {
        // printf("histo_pipeline_blocks\n");
        status = histo_pipeline_blocks(prms, in_img_desc, out_histo_desc, srcPtr, dstPtr, dmean, skiprows);
        if( VX_SUCCESS == status )
        {
            status = histo_dma_teardown(prms);
        }
        else
        {
            printf("histo_pipeline_blocks() failed ......\n");
        }
    }
    else
    {
        printf("histo_dma_setup() failed ......\n");
    }

    return status;
}

static vx_status VX_CALLBACK tivxOwCalcHistoProcess(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status;
    vx_enum cpuId;

    cpuId = tivxGetSelfCpuId();
    
    if( (cpuId == TIVX_CPU_ID_DSP1) || (cpuId == TIVX_CPU_ID_DSP2) )
    {
        status = appC66HistoProcess(kernel, obj_desc, num_params, priv_arg);
    }
    else
    { 
        VX_PRINT(VX_ZONE_ERROR, "================> ONLY SUPPORTS RUNNING ON C6x Cores!!!\n");           
        status = VX_FAILURE;        
    }

    return status;
}

vx_status VX_CALLBACK appC66HistoProcess(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status = (vx_status)VX_SUCCESS;
    //vx_uint32 x = 0;

    /* < DEVELOPER_TODO: Uncomment if kernel context is needed > */
 
    tivxOwCalcHistoParams *prms = NULL;
    uint32_t prms_size;
 
    vx_uint32 szBin = 4;

    //tivx_obj_desc_image_t *input_desc;
    tivx_obj_desc_raw_image_t *input_desc;
    tivx_obj_desc_distribution_t *histo_desc;
    tivx_obj_desc_scalar_t *dark_mean;

    //printf("tivxOwCalcHistoProcess called\n");

    if ( num_params != TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS )
    {
        printf("Invalid number of parameters: %d, expected: %d\n",
                num_params, TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS);
        status = (vx_status)VX_FAILURE;
    }

    if( status == VX_SUCCESS )
    {
        int i;
        for( i = 0; i < TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS; i++ )
        {
            if( obj_desc[i] == NULL )
            {
                printf("Null Descriptor at index: %d\n", i);
                status = VX_FAILURE;
                break;
            }
        }
    }

    if( status != VX_SUCCESS )
    {
        return status;
    }

    status = tivxGetTargetKernelInstanceContext(kernel, (void **)&prms, &prms_size);

    if( prms == NULL )
    {
        printf("tivxGetTargetKernelInstanceContext() returned NULL pointer ......\n");
        return VX_FAILURE;
    }

    if((vx_status)VX_SUCCESS == status)
    {
        /* < DEVELOPER_TODO: Uncomment if kernel context is needed > */
     
        //input_desc = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_CONFIG_IDX];
        input_desc = (tivx_obj_desc_raw_image_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX];
        histo_desc = (tivx_obj_desc_distribution_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX];
        dark_mean = (tivx_obj_desc_scalar_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_MEAN_IDX];
        
        // int32_t w = (int32_t)input_desc->imagepatch_addr[0].dim_x;
        // int32_t h = (int32_t)input_desc->imagepatch_addr[0].dim_y;
 
        // printf(">>> HISTOGRAM INPUT Image size %dx%d\n", w, h);
    }

    if((vx_status)VX_SUCCESS == status)
    {

        void *input_target_ptr;
        void *histo_target_ptr;

        int num_input_pixels = input_desc->mem_size[0];

        input_target_ptr = tivxMemShared2TargetPtr(&input_desc->mem_ptr[0]);
        tivxCheckStatus(&status, tivxMemBufferMap(input_target_ptr,
           input_desc->mem_size[0], (vx_enum)TIVX_MEMORY_TYPE_DMA,
           (vx_enum)VX_READ_ONLY));

        histo_target_ptr = tivxMemShared2TargetPtr(&histo_desc->mem_ptr);
        tivxCheckStatus(&status, tivxMemBufferMap(histo_target_ptr,
           histo_desc->mem_size, (vx_enum)TIVX_MEMORY_TYPE_DMA,
           (vx_enum)VX_WRITE_ONLY));

        {
            /*
            VXLIB_bufParams2D_t vxlib_input;
            tivxInitBufParams(input_desc, &vxlib_input);
            printf("DATATYPE: %d, X: %d Y: %d Stride_Y: %d\n", vxlib_input.data_type, vxlib_input.dim_x, vxlib_input.dim_y, vxlib_input.stride_y);
            */

            /*
            printf("distribution:%p bins:%u off:%u range:%u\n", histo_target_ptr, (uint16_t)histo_desc->num_bins, (uint8_t)histo_desc->offset, (uint16_t)histo_desc->range);
            printf("INPUT DESC: %dx%d PIXEL Count %d\n", input_desc->imagepatch_addr->dim_x, input_desc->imagepatch_addr->dim_y, input_desc->mem_size[0]);
            */

            /* call kernel processing function */

            /* clear the distribution */
            memset(histo_target_ptr, 0, histo_desc->mem_size);
            // clear final histogram buffer in L2 RAM
            memset(prms->out_histo_mem, 0, (prms->numBins * szBin));
            // clear histogram scratch memory in L1 RAM
            memset(prms->scratch_mem_out, 0, (prms->numBins * szBin * NUM_SCRATCH_BUFS));
   
            histo_run(prms, input_desc, input_target_ptr, histo_desc, histo_target_ptr, (void *)&dark_mean->data.f32);

            /* kernel processing function complete */

            //printf("tivxOwCalcHistoProcess exited\n");

        }
        tivxCheckStatus(&status, tivxMemBufferUnmap(input_target_ptr,
           input_desc->mem_size[0], (vx_enum)VX_MEMORY_TYPE_HOST,
            (vx_enum)VX_READ_ONLY));

        tivxCheckStatus(&status, tivxMemBufferUnmap(histo_target_ptr,
           histo_desc->mem_size, (vx_enum)VX_MEMORY_TYPE_HOST,
            (vx_enum)VX_WRITE_ONLY));
    }

    return status;
}

static vx_status VX_CALLBACK tivxOwCalcHistoCreate(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status;
    vx_enum cpuId;
    
    cpuId = tivxGetSelfCpuId();

    printf("==========> tivxOwCalcHistoCreate self_cpu: %d ==========\n", cpuId);

    if( (cpuId == TIVX_CPU_ID_DSP1) || (cpuId == TIVX_CPU_ID_DSP2) )
    {
        status = appC66HistoCreate(kernel, obj_desc, num_params, priv_arg);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "================> ONLY SUPPORTS RUNNING ON C6x Cores!!!\n");           
        status = VX_FAILURE;        
    }

    return status;
}

vx_status appC66HistoCreate(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status = (vx_status)VX_SUCCESS;
    // printf("appC66HistoCreate called ....\n");

    tivx_obj_desc_user_data_object_t *config_desc;
    void                             *config_target_ptr;
    tivxC66HistoParams               *cfgParams;
    int32_t                          rawImageBitDepth;

    static DMAObj dmaObjSrc0;
    static DMAObj dmaObjDst;

    /* < DEVELOPER_TODO: Uncomment if kernel context is needed > */

    //printf("tivxOwCalcHistoCreate Called\n");

    tivxOwCalcHistoParams *prms = NULL;

    /* < DEVELOPER_TODO: (Optional) Add any target kernel create code here (e.g. allocating */
    /*                   local memory buffers, one time initialization, etc) > */

    TSCL = 0;       //enable TSC

    if( num_params != TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS )
    {
        status = (vx_status)VX_FAILURE;
    }

    if( status == VX_SUCCESS )
    {
        int i;
        for(i = 0; i < TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS; i++)
        {
            if(obj_desc[i] == NULL)
            {
                status = VX_ERROR_NO_MEMORY;
                printf("Error: obj_desc[%d] is NULL\n", i);
                break;
            }
        }
    }

    if( status == VX_SUCCESS )
    {
        /* < DEVELOPER_TODO: Uncomment if kernel context is needed > */

    	vx_uint32 blk_width, blk_height, mblk_height, rem_height;
	    vx_uint32 in_size, out_size, total_size, l1_req_size, l1_avail_size, l2_req_size, l2_avail_size, num_sets;
 
        tivx_mem_stats  l1Stats;
        tivx_mem_stats  l2Stats;

        // reset L1 memory
        tivxMemFree(NULL, 0, (vx_enum)TIVX_MEM_INTERNAL_L1);
        tivxMemStats(&l1Stats, (vx_enum)TIVX_MEM_INTERNAL_L1);
        //printf("L1 Memory, total: %d, free: %d\n", l1Stats.mem_size, l1Stats.free_size);

        // reset L2 memory
        tivxMemFree(NULL, 0, (vx_enum)TIVX_MEM_INTERNAL_L2);
        tivxMemStats(&l2Stats, (vx_enum)TIVX_MEM_INTERNAL_L2);
        //printf("L2 Memory, total: %d, free: %d\n", l2Stats.mem_size, l2Stats.free_size);
       
    	l2_avail_size = l2Stats.free_size;          // 224KB of L2 RAM is free to use for DMA blocks!
    	l2_req_size = J721E_C66X_DSP_L2_SRAM_SIZE;

    	if( l2_req_size > l2_avail_size )
    	{
	    	l2_req_size = l2_avail_size;
    	}

        prms = tivxMemAlloc(sizeof(tivxOwCalcHistoParams), TIVX_MEM_EXTERNAL);
        if (prms == NULL)
        {
            status = (vx_status)VX_ERROR_NO_MEMORY;
            printf("tivxOwCalcHistoParams{} allocation failed ......\n");
        }
        else
        {
            tivx_obj_desc_raw_image_t *in_img_desc = (tivx_obj_desc_raw_image_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX];
            vx_imagepatch_addressing_t *pIn = (vx_imagepatch_addressing_t *)&in_img_desc->imagepatch_addr[0];
            vx_uint32 in_width = pIn->dim_x;
            vx_uint32 in_height = pIn->dim_y;
            vx_uint32 in_stride = pIn->stride_y;
            vx_uint32 szPixel, szBin;

            /* Get config structure */
            config_desc = (tivx_obj_desc_user_data_object_t *) obj_desc[TIVX_KERNEL_OW_CALC_HISTO_CONFIG_IDX];
            config_target_ptr = tivxMemShared2TargetPtr(&config_desc->mem_ptr);
            tivxMemBufferMap(config_target_ptr, config_desc->mem_size, VX_MEMORY_TYPE_HOST, VX_READ_ONLY);
            cfgParams = (tivxC66HistoParams *)config_target_ptr;
            //TODO: Hardcoded 3 channels for testing
            cfgParams->enableChanBitFlag = 0x07;
            prms->enableChanBitFlag = cfgParams->enableChanBitFlag;
            
            prms->ch_num= inst_id;
            inst_id++;

            printf("tivxOwCalcHistoCreate(): Histogram created for channel %d mask 0x%04x\n", prms->ch_num, prms->enableChanBitFlag);

            if ((prms->enableChanBitFlag & (1 << prms->ch_num)) != 0)
            {
                printf("channel num %d is enabled\n", prms->ch_num);
            }
            else
            {
                printf("channel num %d is disabled\n", prms->ch_num);
            }

            if (prms->ch_num == 0)
            {
                dma_create(&dmaObjSrc0, DATA_COPY_DMA, 8);
                dma_create(&dmaObjDst, DATA_COPY_DMA, 9);

                prms->inDma = dmaObjSrc0;
                prms->outDma = dmaObjDst;
            }
            else
            {
                prms->inDma = dmaObjSrc0;
                prms->outDma = dmaObjDst;
            }

            rawImageBitDepth = in_img_desc->params.format[0].msb + 1;
            //printf("rawImageBitDepth is %d bits\n", rawImageBitDepth);
            if (rawImageBitDepth <= 8)
            {
                szPixel = 1;
            }
            else if (rawImageBitDepth <= 16)
            {
                szPixel = 2;
            }
            else
            {
                printf("Raw image bit depth exceeds 16 bits !\n");
                status = VX_FAILURE;
            }

            if( status == VX_SUCCESS )
            {
                blk_width = in_stride / sizeof(uint16_t);
                if( blk_width != pIn->dim_x )
                {
                    printf("blk_width (%d) calculated from in_stride (%d) doesn't match pIn->dim_x (%d)......\n", blk_width, in_stride, pIn->dim_x);
                    blk_width = pIn->dim_x;
                }
                //szPixel = 2;

                //printf("in_width: %d, in_height: %d, in_stride: %d, blk_width: %d\n", in_width, in_height, in_stride, blk_width);

                tivx_obj_desc_distribution_t *out_histo_desc = (tivx_obj_desc_distribution_t *)obj_desc[TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX];
                prms->numBins = out_histo_desc->num_bins;
                szBin = 4;

                total_size = 0;
                mblk_height = 1;
                blk_height = mblk_height;
                rem_height = 0;

                in_size = 0;

                /*
                    Using a block width equal to the width of the image.
                    Calculating the block height in this loop by starting
                    with a height of 1 then multiplying the height by 2
                    through each loop.  The loop terminates when either
                    the total size required exceeds the available L2
                    or the height can longer be divided evenly into the
                    total height of the image (so that we can have the
                    SAME SIZE blocks being used in the DMA)
                */
                while ( (total_size < l2_req_size) && (rem_height == 0) )
                {
                    prms->blkWidth = blk_width;
                    prms->blkHeight = blk_height;
                    prms->remHeight = rem_height;

                    blk_height = mblk_height;
                    mblk_height *= 2;

                    in_size = (blk_width * mblk_height) * szPixel;
                    //printf("mblk_height: %d, in_size: %d\n", mblk_height, in_size);

                    out_size = prms->numBins * szBin;

                    /* Double input buffer size */
                    in_size = in_size * 2;
                    total_size = in_size + out_size;
                    rem_height = in_height % mblk_height;
                    //printf("total_size: %d, l2_req_size: %d, in_height: %d, rem_height: %d\n",
                    //        total_size, l2_req_size, in_height, rem_height);
                }

                num_sets = in_height / prms->blkHeight;
                if( (in_height % prms->blkHeight) != 0 )
                {
                    printf("num_sets is not an exact multiple of %d\n", prms->blkHeight);
                    status = VX_FAILURE;
                }
                prms->numSets = num_sets;
                prms->l2_avail_size = l2_avail_size;
                prms->l2_req_size = l2_req_size;

                in_size = (prms->blkWidth * prms->blkHeight) * szPixel;
                out_size = prms->numBins * szBin;

                /*
                printf("Input image DMA block information:\n");
                printf("prms->blkWidth: %d, prms->blkHeight: %d, prms->numSets: %d, prms->remHeight: %d\n",
                        prms->blkWidth, prms->blkHeight, prms->numSets, prms->remHeight);
                printf("in_size: %d, out_size: %d\n", in_size, out_size);
                */

                /* Double input buffer */
                in_size = in_size * 2;
                total_size = in_size + out_size;    // out_size - final histogram of 8KB is in L2 RAM, while scratch mem of 16KB is in L1!
                                                    // so, factor out_size into L2 calculations (not L1)!
                l1_avail_size = l1Stats.free_size;  // free_size should be 24KB
                l1_req_size = 32 * 1024;            // Out of available 32KB, 8KB is reserved for cache!

                if (l1_req_size > l1_avail_size)
                {
                    l1_req_size = l1_avail_size;
                }

                if ((prms->numBins * szBin * NUM_SCRATCH_BUFS) > l1_req_size)
                {
                    printf("histogram out_size + SCRATCH_BUFFER_SIZE exceeed l1 size!\n");
                    status = VX_FAILURE;
                }
                else
                {
                    prms->l1_avail_size = l1_avail_size;
                    prms->l1_req_size = l1_req_size;
                }
            }

            if (status == VX_SUCCESS)
            {
                prms->l2_heap_id = TIVX_MEM_INTERNAL_L2;

#ifndef x86_64
                if( appIpcGetSelfCpuId() == APP_IPC_CPU_C6x_1 )
                {
                    prms->l2_global_base = 0x4D80000000;
                    prms->l1d_global_base = 0x4D80F00000;
                }
                else
                {
                    prms->l2_global_base = 0x4D81000000;
                    prms->l1d_global_base = 0x4D81F00000;
                }
#else
                prms->l2_global_base = 0;
#endif
                prms->pL2 = (uint16_t *)tivxMemAlloc(prms->l2_req_size, prms->l2_heap_id);
                if( prms->pL2 == NULL )
                {
                    printf("Unable to allocate L2 SRAM memory!\n");
                    status = VX_FAILURE;
                }
                else
                {
                    prms->out_histo_mem  = (uint32_t *)( ((uintptr_t)prms->pL2) + in_size );
                    memset(prms->out_histo_mem, 0, (prms->numBins * szBin));

                    prms->l1_heap_id = TIVX_MEM_INTERNAL_L1;

                    prms->pL1 = (uint32_t *)tivxMemAlloc(prms->l1_req_size, prms->l1_heap_id);
                    if (prms->pL1 == NULL)
                    {
                        printf("Unable to allocate L1 scratch!\n");
                        status = VX_FAILURE;
                    }
                    else
                    {
                        prms->scratch_mem_out = prms->pL1;
                        prms->hist_2d = (uint32_t (*)[4])prms->scratch_mem_out;

                        memset(prms->scratch_mem_out, 0, (prms->numBins * szBin * NUM_SCRATCH_BUFS));

                        tivxSetTargetKernelInstanceContext(kernel, prms, sizeof(tivxOwCalcHistoParams));
                        //printf("set kernel context successfully!\n");
                    }
                }
            }

            tivxMemBufferUnmap(config_target_ptr, config_desc->mem_size,
                               VX_MEMORY_TYPE_HOST, VX_READ_ONLY);
            /*
            printf("prms->pL1: %p, prms->l1_req_size: %d, prms->out_histo_mem: %p, prms->scratch_mem_out: %p\n",
                    prms->pL1, prms->l1_req_size, prms->out_histo_mem, prms->scratch_mem_out);

            printf("prms->pL2: %p, prms->l2_req_size: %d, prms->l2_global_base: 0x%llx\n",
                    prms->pL2, prms->l2_req_size, prms->l2_global_base);
            */
        }
    }

    return status;
}

static vx_status VX_CALLBACK tivxOwCalcHistoDelete(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status;
    vx_enum cpuId;

    cpuId = tivxGetSelfCpuId();
    printf("==========> tivxOwCalcHistoDelete self_cpu: %d ==========\n", cpuId);

    if( (cpuId == TIVX_CPU_ID_DSP1) || (cpuId == TIVX_CPU_ID_DSP2) )
    {
        status = appC66HistoDelete(kernel, obj_desc, num_params, priv_arg);
    }
    else
    {        
        VX_PRINT(VX_ZONE_ERROR, "================> ONLY SUPPORTS RUNNING ON C6x Cores!!!\n");           
        status = VX_FAILURE;
    }

    return status;
}

vx_status appC66HistoDelete(
       tivx_target_kernel_instance kernel,
       tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status = (vx_status)VX_SUCCESS;
    printf("appC66HistoDelete called ....\n");
    /* < DEVELOPER_TODO: Uncomment if kernel context is needed > */

    tivxOwCalcHistoParams *prms = NULL;
    uint32_t prms_size;

    //printf("tivxOwCalcHistoDelete() called ....\n");

    if ( (num_params != TIVX_KERNEL_OW_CALC_HISTO_MAX_PARAMS)
        || (NULL == obj_desc[TIVX_KERNEL_OW_CALC_HISTO_INPUT_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_OW_CALC_HISTO_HISTO_IDX])
    )
    {
        status = (vx_status)VX_FAILURE;
    }
    else
    {
        status = tivxGetTargetKernelInstanceContext(kernel, (void **)&prms, &prms_size);

        //printf("Got kernel instance ....\n");
        if (VX_SUCCESS != status)
        {
            status = (vx_status)VX_ERROR_NO_MEMORY;
            printf("Returned VX_ERROR_NO_MEMORY ....\n");
        }
        else
        {
            if(prms->ch_num == 0)
            {
                dma_delete(&prms->inDma);
                dma_delete(&prms->outDma);
            }

            tivxMemFree(prms->pL1, prms->l1_req_size, prms->l1_heap_id);
            tivxMemFree(prms->pL2, prms->l2_req_size, prms->l2_heap_id);
            tivxMemFree(prms, prms_size, (vx_enum)TIVX_MEM_EXTERNAL);
            // printf("Returned from tivxMemFree() ....\n");
        }
    }
    
    // reset inst_id
    inst_id = 0;

    return status;
}

static vx_status tivxOwCalcHistoTimestampSync
(
    tivxOwCalcHistoParams *prms,
    const tivx_obj_desc_user_data_object_t *usr_data_obj
)
{
    tivxC66HistoTimeSync time_sync;
    vx_status status = (vx_status)VX_SUCCESS;

    printf("app_c66_timestamp_sync enter cpu: %d\n", tivxGetSelfCpuId());

    if (NULL != usr_data_obj)
    {
        void  *target_ptr = tivxMemShared2TargetPtr(&usr_data_obj->mem_ptr);

        tivxMemBufferMap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);

        if (sizeof(tivxC66HistoTimeSync) ==  usr_data_obj->mem_size)
        {
            memcpy(&time_sync, target_ptr, sizeof(tivxC66HistoTimeSync));
        }

        prms->ref_ts = time_sync.refTimestamp;
        //prms->ref_tick = __TSC;
        prms->ref_tick = _itoll(TSCH, TSCL);
        //printf("tivxOwCalcHistoTimestampSync: timestamp = 0x%llx\n", prms->ref_ts);

        tivxMemBufferUnmap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);

        //printf("tivxOwCalcHistoTimestampSync(): target unmapped\n");

    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxOwCalcHistoTimestampSync(): Data Object is NULL\n");
        status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
    }

    //printf("tivxOwCalcHistoTimestampSync(): Done!\n");
    return (status);
}

static vx_status tivxOwCalcHistoChangePair
(
    tivxOwCalcHistoParams *prms,
    const tivx_obj_desc_user_data_object_t *usr_data_obj
)
{
    tivxC66HistoCmd cmd;
    vx_status status = (vx_status)VX_SUCCESS;

    if (NULL != usr_data_obj)
    {
        void  *target_ptr = tivxMemShared2TargetPtr(&usr_data_obj->mem_ptr);

        tivxMemBufferMap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);

        if (sizeof(tivxC66HistoCmd) ==  usr_data_obj->mem_size)
        {
            memcpy(&cmd, target_ptr, sizeof(tivxC66HistoCmd));
        }

        tivxMemBufferUnmap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxOwCalcHistoChangePair(): Data Object is NULL\n");
        status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
    }

    return (status);
}

static vx_status tivxOwCalcHistoEnableChan
(
    tivxOwCalcHistoParams *prms,
    const tivx_obj_desc_user_data_object_t *usr_data_obj
)
{
    tivxC66HistoCmd cmd;
    vx_status status = (vx_status)VX_SUCCESS;

    if (NULL != usr_data_obj)
    {
        void  *target_ptr = tivxMemShared2TargetPtr(&usr_data_obj->mem_ptr);

        tivxMemBufferMap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);

        if (sizeof(tivxC66HistoCmd) ==  usr_data_obj->mem_size)
        {
            memcpy(&cmd, target_ptr, sizeof(tivxC66HistoCmd));
        }

        prms->enableChanBitFlag = 0x07;         // 6 channels?

        tivxMemBufferUnmap(target_ptr, usr_data_obj->mem_size,
            (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxOwCalcHistoEnableChan(): Data Object is NULL\n");
        status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
    }

    return (status);
}

static vx_status VX_CALLBACK tivxOwCalcHistoControl(
       tivx_target_kernel_instance kernel,
       uint32_t node_cmd_id, tivx_obj_desc_t *obj_desc[],
       uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    vx_enum self_cpu;

    self_cpu = tivxGetSelfCpuId();


    printf("==========> tivxOwCalcHistoControl self_cpu: %d ==========\n", self_cpu);

    /* < DEVELOPER_TODO: (Optional) Add any target kernel control code here (e.g. commands */
    /*                   the user can call to modify the processing of the kernel at run-time) > */

    tivxOwCalcHistoParams *prms = NULL;

    if( NULL == obj_desc[0] )
    {
        printf("Command handle is NULL!\n");
        status = VX_FAILURE;
    }

    if( status == VX_SUCCESS )
    {
        uint32_t size;

        status = tivxGetTargetKernelInstanceContext(kernel, (void **)&prms, &size);
        if ( (VX_SUCCESS != status) ||
             (NULL == prms) ||
             (sizeof(tivxOwCalcHistoParams) != size)
           )
        {
            status = VX_FAILURE;
        }
    }

    switch (node_cmd_id)
    {
        case TIVX_C66_HISTO_CMD_ENABLE_CHAN:
        {
            tivxOwCalcHistoEnableChan(prms, (tivx_obj_desc_user_data_object_t *)obj_desc[0U]);
            break;
        }
        case TIVX_C66_HISTO_CMD_CHANGE_PAIR:
        {
            tivxOwCalcHistoChangePair(prms, (tivx_obj_desc_user_data_object_t *)obj_desc[0U]);
            break;
        }        
        case TIVX_C66_HISTO_TIMESTAMP_SYNC:
        {
            tivxOwCalcHistoTimestampSync(prms, (tivx_obj_desc_user_data_object_t *)obj_desc[0U]);
            break;
        }
        case TIVX_C66_HISTO_CMD_SET_DSP_CORE_1:
        {
            printf("================> Requesting TARGET CORE 1\n");
            break;
        }
        case TIVX_C66_HISTO_CMD_SET_DSP_CORE_2:
        {
            printf("================> Requesting TARGET CORE 2\n");
            break;
        }
        default:
        {
            VX_PRINT(VX_ZONE_ERROR, "tivxOwCalcHistoControl(): Invalid Command Id\n");
            status = (vx_status)VX_FAILURE;
            break;
        }
    }

    return (status);
}

void tivxAddTargetKernelOwCalcHisto(void)
{
    vx_status status = (vx_status)VX_FAILURE;
    char target_name[TIVX_TARGET_MAX_NAME];
    vx_enum self_cpu;

    self_cpu = tivxGetSelfCpuId();

    printf("==========> tivxAddTargetKernelOwCalcHisto self_cpu: %d ==========\n", self_cpu);

    if ( self_cpu == (vx_enum)TIVX_CPU_ID_DSP1 )
    {
        strncpy(target_name, TIVX_TARGET_DSP1, TIVX_TARGET_MAX_NAME);
        status = (vx_status)VX_SUCCESS;
    }
    else
    if ( self_cpu == (vx_enum)TIVX_CPU_ID_DSP2 )
    {
        strncpy(target_name, TIVX_TARGET_DSP2, TIVX_TARGET_MAX_NAME);
        status = (vx_status)VX_SUCCESS;
    }
    else
    {
        status = (vx_status)VX_FAILURE;
    }

    if (status == (vx_status)VX_SUCCESS)
    {
        vx_ow_calc_histo_target_kernel = tivxAddTargetKernelByName(
                            TIVX_KERNEL_OW_CALC_HISTO_NAME,
                            target_name,
                            tivxOwCalcHistoProcess,
                            tivxOwCalcHistoCreate,
                            tivxOwCalcHistoDelete,
                            tivxOwCalcHistoControl,
                            NULL);
    }
}

void tivxRemoveTargetKernelOwCalcHisto(void)
{
    vx_status status = (vx_status)VX_SUCCESS;

    status = tivxRemoveTargetKernel(vx_ow_calc_histo_target_kernel);
    if (status == (vx_status)VX_SUCCESS)
    {
        vx_ow_calc_histo_target_kernel = NULL;
    }
}
