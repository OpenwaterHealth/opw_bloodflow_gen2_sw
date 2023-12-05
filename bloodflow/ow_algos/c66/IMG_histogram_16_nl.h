/* ======================================================================== *
 * IMGLIB -- TI Image and Video Processing Library                          *
 *                                                                          *
 *                                                                          *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/   *
 *                                                                          *
 *                                                                          *
 *  Redistribution and use in source and binary forms, with or without      *
 *  modification, are permitted provided that the following conditions      *
 *  are met:                                                                *
 *                                                                          *
 *    Redistributions of source code must retain the above copyright        *
 *    notice, this list of conditions and the following disclaimer.         *
 *                                                                          *
 *    Redistributions in binary form must reproduce the above copyright     *
 *    notice, this list of conditions and the following disclaimer in the   *
 *    documentation and/or other materials provided with the                *
 *    distribution.                                                         *
 *                                                                          *
 *    Neither the name of Texas Instruments Incorporated nor the names of   *
 *    its contributors may be used to endorse or promote products derived   *
 *    from this software without specific prior written permission.         *
 *                                                                          *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     *
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       *
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   *
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    *
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   *
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    *
 * ======================================================================== */

#ifndef IMG_HISTOGRAM_16_H_
#define IMG_HISTOGRAM_16_H_ 1

/** @defgroup IMG_histogram_16 */
/** @ingroup  IMG_histogram_16 */
/* @{ */

/**
 * @par Description:
 *  This code calculates the histogram of an image array "image" containing 
 *  "n" pixels, with "img_bits" valid data bits per pixel. It returns a
 *  histogram with 2^img_bits bins, one for each of the possible pixel values 
 *  based on the precision. 
 * @par 
 *  The routine can either add-to or subtract-from an existing histogram, 
 *  through the "accumulate" control.  The implementation requires temporary 
 *  storage for 4 scratch histograms, to reduce bank conflicts.
 * @par 
 *  The length of the "hist" and "t_hist" arrays depend on the pixel precision
 *  as specified by "img_bits". The length of "hist" is 2^img_bits and that of     
 *  "t_hist" is 2^(img_bits+2)
 * @par 
 *    @param image        Input image pointer containing "n" unsigned 8-bit 
 *                        pixels
 *    @param n            Size of image in pixels
 *    @param accumulate   Control to add or subtract from the running 
 *                        histogram.  This control is only defined for the  
 *                        values 1 and -1 for ADD and SUBTRACT respectively 
 *    @param t_hist       Scratch buffer for temporary histogram storage 
 *                        (SIZE: 2^(img_bits+2))
 *    @param hist         Running histogram bins (SIZE: 2^img_bits)
 *    @param img_bits     Number of valid data bits per pixel
 *
 * @par Algorithm:
 *  This code operates on four interleaved histogram bins.  The loop   
 *  is divided into two halves:  The "even" half operates on the       
 *  even-numbered double-words from the input image, and the "odd"     
 *  half operates on odd double-words.  Each half processes four       
 *  pixels at a time, and both halves operate on the same four sets    
 *  of histogram bins. This introduces a memory dependency on the      
 *  histogram bins which ordinarily would degrade performance. To      
 *  break the memory depenencies, the two halves forward their results 
 *  to each other via the register file, bypassing memory.             
 * @par 
 *  Exact memory access ordering obviates the need to predicate        
 *  stores within the loop.                                            
 * @par 
 *  The algorithm is ordered as follows:                               
 *   -# Load from histogram for even half                              
 *   -# Store odd_bin to histogram for odd half (previous iteration)        
 *   -# IF data_even == previous data_odd THEN increment even_bin by 2      
 *      ELSE increment even_bin by 1, forward to odd                   
 *   -# Load from histogram for odd half (current iteration)
 *   -# Store even_bin to histogram for even half                      
 *   -# IF data_odd == previous data_even THEN increment odd_bin by 2       
 *      ELSE increment odd_bin by 1, forward to even                   
 *   -# Repeat from 1
 * @par 
 *  With this particular ordering, forwarding is necessary between the even and
 *  odd halves when pixels in adjacent halves fall in the same bin.  The store 
 *  is never predicated and occurs speculatively as it will be overwritten by 
 *  the next value containing the extra forwarded value.                       
 * @par 
 *  The four scratch histograms are interleaved with each bin spaced four half 
 *  words apart and each histogram starting in a different memory bank. This 
 *  allows the four histogram accesses to proceed in any order without worrying 
 *  about bank conflicts. The diagram below illustrates this:  (addresses are 
 *  halfword offsets)                                                            
 * @verbatim 
         0       1       2       3       4       5       6   ...        
     | hst 0 | hst 1 | hst 2 | hst 3 | hst 0 | hst 1 | ...   ...        
     | bin 0 | bin 0 | bin 0 | bin 0 | bin 1 | bin 1 | ...   ...        
   @endverbatim 
 * @par Algorithm:
 *  The natural C implementation has no restrictions. The optimized     
 *  intrinsic C code has restrictions as noted in Assumptions below.    
 *
 *
 * @par Assumptions:
 *    - The image and output histogram arrays do not overlap
 *    - The temporary array, t_hist, is initialized to zero for the first
 *      call to the routine (for "accumulate")
 *    - All arrays (image and histogram) are 64-bit aligned
 *    - The number of pixels is a non-zero multiple of 8. 
 *    - The number of bits per pixel, "img_bits" is between 1 and 16 (inclusive)
 *    - The maximum count per bin is 32767
 *
 *
 * @par Implementation Notes:
 *    - This code is fully interruptible
 *    - This code is compatible with C64x+ processors
 *    - No bank conflicts should occur in this code
 *
 *
 * @par Benchmarks:
 *  See IMGLIB_Test_Report.html for cycle and memory information.
 *
 */

void IMG_histogram_16_local
(
     uint16_t *restrict   image,  /* input image data                */
     int                  n,  /* number of input image pixels    */
     int                  accumulate,  /* define add/sum for histogtram   */
     uint16_t *restrict   t_hist,  /* temporary histogram bins        */
     uint16_t *restrict   hist,  /* running histogram bins          */
     int                  img_bits   /* number of data bits per pixel   */
);

/** @} */

void IMG_histogram_16_cn_local
(
    uint16_t *restrict  image,  /* input image data                */
    int                 n,  /* number of input image pixels    */
    int                 accumulate,  /* define add/sum for histogtram   */
    uint32_t (* restrict t_hist)[4],  /* temporary 2D histogram bins  (1024 x 4) */
    uint32_t *restrict  hist,  /* running histogram bins          */
    int                 img_bits,   /* number of data bits in a pixel  */
    uint32_t            blk
);

uint32_t IMG_dark_rows_sum
(
    uint16_t* restrict		image,           /* input image data                */
    int						num_pixels       /* number of input image pixels    */
);

#endif

/* ======================================================================== */
/*  End of file:  IMG_histogram_16.h                                        */
/* ======================================================================== */
