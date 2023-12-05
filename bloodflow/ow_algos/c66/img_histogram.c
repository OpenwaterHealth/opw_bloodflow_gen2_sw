// gcc -O3 -march=native -c img_histogram.c -o img_histogram.o

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void IMG_histogram_16_cn_local
(
    uint16_t* restrict		image,  /* input image data                */
    int						n,  /* number of input image pixels    */
    int						accumulate,  /* define add/sum for histogtram   */
    uint32_t (* restrict t_hist)[4],  /* temporary 2D histogram bins (1024 x 4)        */
    uint32_t* restrict		hist,  /* running histogram bins          */
    int						img_bits,  /* number of data bits in a pixel  */
	uint32_t                blk
)
{
	/* -------------------------------------------------------------------- */
	/*  This loop is unrolled four times, producing four interleaved        */
	/*  histograms into a temporary buffer.                                 */
	/* -------------------------------------------------------------------- */
	// if you hard code the image bits it will get faster ~8-9ms to 7-8ms

	int i;

	_nassert((int)image % 8 == 0);
	_nassert((int)t_hist % 8 == 0);
	_nassert((int)hist % 8 == 0);

	uint32_t (* restrict hist_2d)[4] = t_hist;

	double p0123;
	double p4567;

	for (i = 0; i < n; i += 8)
	{
	    p0123 = _amemd8((void*)&image[i]);		// load 4-pixels

        uint16_t c00 = (_extu(_hi(p0123), 0, 16)); /* extract pix 3   */
        uint16_t c01 = (_extu(_hi(p0123), 16,16)); /* extract pix 2   */
        uint16_t c02 = (_extu(_lo(p0123), 0, 16)); /* extract pix 1   */
        uint16_t c03 = (_extu(_lo(p0123), 16,16)); /* extract pix 0   */

		uint32_t * restrict hist0 = &hist_2d[c00][0];
		uint32_t * restrict hist1 = &hist_2d[c01][1];
		uint32_t * restrict hist2 = &hist_2d[c02][2];
		uint32_t * restrict hist3 = &hist_2d[c03][3];

		_nassert((int)hist0 % 8 == 0);
		_nassert((int)hist1 % 8 == 0);
		_nassert((int)hist2 % 8 == 0);
		_nassert((int)hist3 % 8 == 0);

		uint32_t c_p0 = *hist0;
		uint32_t c_p1 = *hist1;
		uint32_t c_p2 = *hist2;
		uint32_t c_p3 = *hist3;

		*hist0 = c_p0 + 1;
		*hist1 = c_p1 + 1;
		*hist2 = c_p2 + 1;
		*hist3 = c_p3 + 1;

	    p4567 = _amemd8((void*)&image[i+4]);
        uint16_t c10 = (_extu(_hi(p4567), 0, 16)); /* extract pix 3   */
        uint16_t c11 = (_extu(_hi(p4567), 16,16)); /* extract pix 2   */
        uint16_t c12 = (_extu(_lo(p4567), 0, 16)); /* extract pix 1   */
        uint16_t c13 = (_extu(_lo(p4567), 16,16)); /* extract pix 0   */

		uint32_t * restrict hist4 = &hist_2d[c10][0];
		uint32_t * restrict hist5 = &hist_2d[c11][1];
		uint32_t * restrict hist6 = &hist_2d[c12][2];
		uint32_t * restrict hist7 = &hist_2d[c13][3];

		_nassert((int)hist4 % 8 == 0);
		_nassert((int)hist5 % 8 == 0);
		_nassert((int)hist6 % 8 == 0);
		_nassert((int)hist7 % 8 == 0);

		uint32_t c_p4 = *hist4;
		uint32_t c_p5 = *hist5;
		uint32_t c_p6 = *hist6;
		uint32_t c_p7 = *hist7;

		*hist4 = c_p4 + 1;
		*hist5 = c_p5 + 1;
		*hist6 = c_p6 + 1;
		*hist7 = c_p7 + 1;
	}

	#if 0
	for (i = 600; i < 750; i++)
	{
		//hist[i] = hist_2d[i][0]
		//+ hist_2d[i][1]
		//+ hist_2d[i][2]
		//+ hist_2d[i][3];

		//printf("bin[%d]: %d        %d        %d        %d, hist[%d]: %d\n", i, hist_2d[i][0], hist_2d[i][1], hist_2d[i][2], hist_2d[i][3], i, hist[i]);
		printf("bin[%d]: %d        %d        %d        %d\n", i, hist_2d[i][0], hist_2d[i][1], hist_2d[i][2], hist_2d[i][3]);
	}
	#endif
}

uint32_t IMG_dark_rows_sum
(
    uint16_t* restrict		image,           /* input image data                */
    int						num_pixels       /* number of input image pixels    */
)
{
	/* -------------------------------------------------------------------- */
	/*  This loop is unrolled four times, producing four interleaved        */
	/*  histograms into a temporary buffer.                                 */
	/* -------------------------------------------------------------------- */
	// if you hard code the image bits it will get faster ~8-9ms to 7-8ms

	int i;
	uint32_t sum = 0;
	int acc = 1;
	int acc_pack;

	_nassert((int)image % 8 == 0);

	double p0123;

	for (i = 0; i < num_pixels; i += 4)
	{
	    p0123 = _amemd8((void*)&image[i]);		// load 4-pixels

		acc_pack = _pack2(acc, acc);

		sum += _dotp2(_lo(p0123), acc_pack) + _dotp2(_hi(p0123), acc_pack);
	}

	return sum;
}
