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

#ifndef TIVX_OW_ALGOS_KERNELS_H_
#define TIVX_OW_ALGOS_KERNELS_H_

#include <VX/vx.h>
#include <VX/vx_kernels.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * \brief The list of supported kernels in this kernel extension.
 */

/*! \brief Name for OpenVX Extension kernel module: ow_algos
 * \ingroup group_tivx_ext
 */
#define TIVX_MODULE_NAME_OW_ALGOS    "ow_algos"

/*! \brief The list of kernels supported in ow_algos module
 *
 * Each kernel listed here can be used with the <tt>\ref vxGetKernelByName</tt> call.
 * When programming the parameters, use
 * \arg <tt>\ref VX_INPUT</tt> for [in]
 * \arg <tt>\ref VX_OUTPUT</tt> for [out]
 * \arg <tt>\ref VX_BIDIRECTIONAL</tt> for [in,out]
 *
 * When programming the parameters, use
 * \arg <tt>\ref VX_TYPE_IMAGE</tt> for a <tt>\ref vx_image</tt> in the size field of <tt>\ref vxGetParameterByIndex</tt> or <tt>\ref vxSetParameterByIndex</tt>
 * \arg <tt>\ref VX_TYPE_ARRAY</tt> for a <tt>\ref vx_array</tt> in the size field of <tt>\ref vxGetParameterByIndex</tt> or <tt>\ref vxSetParameterByIndex</tt>
 * \arg or other appropriate types in \ref vx_type_e.
 * \ingroup group_kernel
 */

/*! \brief ow_calc_histo kernel name
 *  \see group_vision_function_ow_algos
 */
#define TIVX_KERNEL_OW_CALC_HISTO_NAME     "com.ti.ow_algos.ow_calc_histo"

/*! End of group_vision_function_ow_algos */

/*!
 * \brief Used for the Application to load the ow_algos kernels into the context.
 * \ingroup group_kernel
 */
void tivxOwAlgosLoadKernels(vx_context context);

/*!
 * \brief Used for the Application to unload the ow_algos kernels from the context.
 * \ingroup group_kernel
 */
void tivxOwAlgosUnLoadKernels(vx_context context);

/*!
 * \brief Used to print the performance of the kernels.
 * \ingroup group_kernel
 */
void tivxOwAlgosPrintPerformance(vx_perf_t performance, uint32_t numPixels, const char* testName);

/*!
 * \brief Function to register OW_ALGOS Kernels on the c66 Target
 * \ingroup group_tivx_ext
 */
void tivxRegisterOwAlgosTargetC66Kernels(void);

/*!
 * \brief Function to un-register OW_ALGOS Kernels on the c66 Target
 * \ingroup group_tivx_ext
 */
void tivxUnRegisterOwAlgosTargetC66Kernels(void);

#ifdef __cplusplus
}
#endif

#endif /* TIVX_OW_ALGOS_KERNELS_H_ */
