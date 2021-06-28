#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "poly_mul.h"

//define to use assembly
//#define PQM4_POLY_MUL
// #define TOOM_COOK

// extra memory for unrolled karatsuba
uint16_t kara_tmp_asm[16];
#define kara_tmp kara_tmp_asm

// top level karatsuba
#define KARA_TOP_K 128
#define KARA_BOTTOM_K 32
//uint16_t kara_tmp_top[KARA_TOP_K/2];

//unrolled_kara_mem(a, b, c, k)
//#define unrolled_kara_mem_bottom unrolled_kara_mem
//#define unrolled_kara_mem_bottom(a, b, c) unrolled_kara_mem(a, b, c, KARA_TOP_K)

void unrolled_kara_mem_top(const uint16_t* a, const uint8_t* c, uint16_t* d);
void unrolled_kara_mem_bottom_8(const uint16_t* a, const uint8_t* c, uint16_t* d);
void unrolled_kara_mem_bottom_16(const uint16_t* a, const uint16_t* c, uint16_t* d);

void pol_mul(uint16_t* a, uint8_t* b, uint16_t* res)
{
#ifdef TOOM_COOK
	toom_cook_4way_mem(a, b, res);
#else
	uint32_t i;

	uint16_t c[2*SABER_N];

	for (i = 0; i < 2*SABER_N; i++) {
		c[i] = 0;
	}

	unrolled_kara_mem_top(a, b, c);

	//---------------reduction-------
	for(i=SABER_N;i<2*SABER_N-1;i++){
		res[i-SABER_N] = (res[i-SABER_N]+(c[i-SABER_N]-c[i])) & (SABER_Q-1);
	}
	res[SABER_N-1] = (res[SABER_N-1] + c[SABER_N-1]) & (SABER_Q-1);
#endif
}

void unrolled_kara_mem_top(const uint16_t* a, const uint8_t* c, uint16_t* d)
{
	int i;

	uint16_t kara_tmp_top[KARA_TOP_K/2];

//loop1 & loop1_2
	for (i = 0; i < KARA_TOP_K/2; ++i) {
		d[KARA_TOP_K+i] = d[i] + d[KARA_TOP_K+i];
		d[KARA_TOP_K+KARA_TOP_K/2+i] = d[KARA_TOP_K/2+i] + d[KARA_TOP_K+KARA_TOP_K/2+i]  + d[KARA_TOP_K+i];
		d[3*KARA_TOP_K-1+i] = a[i] + a[KARA_TOP_K+i];
		d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = a[KARA_TOP_K/2+i] + a[KARA_TOP_K+KARA_TOP_K/2+i];
		kara_tmp_top[i] = d[3*KARA_TOP_K-1+i] + d[3*KARA_TOP_K+KARA_TOP_K/2-1+i];//XXX: global variable asm
		d[2*KARA_TOP_K+i] = 0;
	}
	for (i = 0; i < KARA_TOP_K/2; ++i) {
		d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] = (int8_t)(c[i] + c[KARA_TOP_K/2+i] + c[KARA_TOP_K+i] + c[KARA_TOP_K+KARA_TOP_K/2+i]);
	}

	unrolled_kara_mem(kara_tmp_top, &d[2*KARA_TOP_K+KARA_TOP_K/2-1], &d[KARA_TOP_K+KARA_TOP_K/2], KARA_BOTTOM_K);

//loop2
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K+KARA_TOP_K/2+i] + d[2*KARA_TOP_K+i];
		d[KARA_TOP_K+KARA_TOP_K/2+i] = 0;
		kara_tmp_top[i] = (int8_t)(c[i] + c[KARA_TOP_K+i]);//XXX: global variable asm
	}
	d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K+KARA_TOP_K/2+i];
	d[KARA_TOP_K+KARA_TOP_K/2+i] = 0;
	kara_tmp_top[i] = (int8_t)(c[i] + c[KARA_TOP_K+i]);//XXX: global variable asm

	unrolled_kara_mem(kara_tmp_top, &d[3*KARA_TOP_K-1], &d[KARA_TOP_K], KARA_BOTTOM_K);

//loop3
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[KARA_TOP_K+KARA_TOP_K/2+i];
		d[KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[KARA_TOP_K+i];
		d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;
		kara_tmp_top[i] = (int8_t)(c[KARA_TOP_K/2+i] + c[KARA_TOP_K+KARA_TOP_K/2+i]);//XXX: global variable asm
	}
	d[KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[KARA_TOP_K+i];
	d[2*KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;
	kara_tmp_top[i] = (int8_t)(c[KARA_TOP_K/2+i] + c[KARA_TOP_K+KARA_TOP_K/2+i]);//XXX: global variable asm

	unrolled_kara_mem_bottom_16(kara_tmp_top, &d[KARA_TOP_K/2+3*KARA_TOP_K-1], &d[2*KARA_TOP_K]);

//loop4 & loop4_2
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[KARA_TOP_K/2+i] = d[i] + d[KARA_TOP_K/2+i];
		d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[2*KARA_TOP_K+KARA_TOP_K/2+i];
		d[3*KARA_TOP_K-1+i] = d[2*KARA_TOP_K+i] + d[KARA_TOP_K+i];

		d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K+KARA_TOP_K/2+i] - d[2*KARA_TOP_K+i];//
		d[KARA_TOP_K+i] = 0;//
	}
	d[KARA_TOP_K/2+i] = d[i] + d[KARA_TOP_K/2+i];
	d[3*KARA_TOP_K-1+i] = d[2*KARA_TOP_K+i] + d[KARA_TOP_K+i];
	d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K+KARA_TOP_K/2+i] - d[2*KARA_TOP_K+i];//
	for (i = 0; i < KARA_TOP_K/2; ++i) {
		d[KARA_TOP_K+KARA_TOP_K/2-1+i] = a[i] + a[KARA_TOP_K/2+i];
		kara_tmp_top[i] = (int8_t)(c[i] + c[KARA_TOP_K/2+i]);//XXX: global variable asm
	}

	unrolled_kara_mem_bottom_16(kara_tmp_top, &d[KARA_TOP_K+KARA_TOP_K/2-1], &d[KARA_TOP_K/2]);

//loop5
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K/2+i] + d[KARA_TOP_K+i];
		d[KARA_TOP_K/2+i] = 0;
	}
	d[KARA_TOP_K+KARA_TOP_K/2-1+i] = d[KARA_TOP_K/2+i];
	d[KARA_TOP_K/2+i] = 0;

	unrolled_kara_mem_bottom_8(a, c, d);

//loop6
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[KARA_TOP_K+i] = d[KARA_TOP_K+i] - d[KARA_TOP_K/2+i];
		d[KARA_TOP_K/2+i] = d[KARA_TOP_K+KARA_TOP_K/2-1+i] - d[i];
		d[KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;
	}
	d[KARA_TOP_K/2+i] = d[KARA_TOP_K+KARA_TOP_K/2-1+i] - d[i];
	d[KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;

	unrolled_kara_mem_bottom_8(&a[KARA_TOP_K/2], &c[KARA_TOP_K/2], &d[KARA_TOP_K]);

//loop7 & loop7_2
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[KARA_TOP_K/2+i] = d[KARA_TOP_K/2+i] - d[KARA_TOP_K+i];
		d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[KARA_TOP_K+i] + d[KARA_TOP_K+KARA_TOP_K/2+i];
		d[KARA_TOP_K+i] = d[3*KARA_TOP_K-1+i] - d[i];
		d[3*KARA_TOP_K-1+i] = 0;
		d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+i] + d[2*KARA_TOP_K+KARA_TOP_K/2+i] - d[KARA_TOP_K+KARA_TOP_K/2+i];//
		d[KARA_TOP_K+KARA_TOP_K/2+i] = d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[KARA_TOP_K/2+i];//
		kara_tmp_top[i] = (int8_t)(c[KARA_TOP_K+i] + c[KARA_TOP_K+KARA_TOP_K/2+i]);
		d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = a[KARA_TOP_K+i] + a[KARA_TOP_K+KARA_TOP_K/2+i];
	}
	d[KARA_TOP_K/2+i] = d[KARA_TOP_K/2+i] - d[KARA_TOP_K+i];
	d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[KARA_TOP_K+i];
	d[KARA_TOP_K+i] = d[3*KARA_TOP_K-1+i] - d[i];
	d[3*KARA_TOP_K-1+i] = 0;
	d[KARA_TOP_K+KARA_TOP_K/2+i] = d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[KARA_TOP_K/2+i];//
	d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+i] + d[2*KARA_TOP_K+KARA_TOP_K/2+i];//
	kara_tmp_top[i] = (int8_t)(c[KARA_TOP_K+i] + c[KARA_TOP_K+KARA_TOP_K/2+i]);
	d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = a[KARA_TOP_K+i] + a[KARA_TOP_K+KARA_TOP_K/2+i];
	// for (i = 0 ; i < KARA_TOP_K/2; ++i) {
		// kara_tmp_top[i] = c[KARA_TOP_K+i] + c[KARA_TOP_K+KARA_TOP_K/2+i];
		// d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = a[KARA_TOP_K+i] + a[KARA_TOP_K+KARA_TOP_K/2+i];
	// }

	unrolled_kara_mem_bottom_16(kara_tmp_top, &d[3*KARA_TOP_K+KARA_TOP_K/2-1], &d[2*KARA_TOP_K+KARA_TOP_K/2]);

//loop8
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[2*KARA_TOP_K+KARA_TOP_K/2+i] + d[3*KARA_TOP_K+i];
		d[2*KARA_TOP_K+KARA_TOP_K/2+i] = 0;
	}
	d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = d[2*KARA_TOP_K+KARA_TOP_K/2+i];
	d[2*KARA_TOP_K+KARA_TOP_K/2+i] = 0;

	unrolled_kara_mem_bottom_8( &a[KARA_TOP_K], &c[KARA_TOP_K], &d[2*KARA_TOP_K] );

//loop9
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[3*KARA_TOP_K+i] = d[3*KARA_TOP_K+i] - d[2*KARA_TOP_K+KARA_TOP_K/2+i];
		d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[2*KARA_TOP_K+i];
		d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;
	}
	d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] - d[2*KARA_TOP_K+i];
	d[3*KARA_TOP_K+KARA_TOP_K/2-1+i] = 0;

	unrolled_kara_mem_bottom_8(&a[KARA_TOP_K+KARA_TOP_K/2], &c[KARA_TOP_K+KARA_TOP_K/2], &d[3*KARA_TOP_K]);

//loop10
	for (i = 0; i < KARA_TOP_K/2-1; ++i) {
		d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+KARA_TOP_K/2+i] - d[3*KARA_TOP_K+i];
		d[3*KARA_TOP_K+i] = d[3*KARA_TOP_K+i] - d[3*KARA_TOP_K+KARA_TOP_K/2+i];//
		d[KARA_TOP_K+i] = d[KARA_TOP_K+i] - d[2*KARA_TOP_K+i];
		d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[3*KARA_TOP_K+i];

		d[KARA_TOP_K+KARA_TOP_K/2+i] = d[KARA_TOP_K+KARA_TOP_K/2+i] - d[2*KARA_TOP_K+KARA_TOP_K/2+i];
		d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+KARA_TOP_K/2+i] - d[3*KARA_TOP_K+KARA_TOP_K/2+i];//
	}
	d[2*KARA_TOP_K+KARA_TOP_K/2+i] = d[2*KARA_TOP_K+KARA_TOP_K/2+i] - d[3*KARA_TOP_K+i];
	d[KARA_TOP_K+i] = d[KARA_TOP_K+i] - d[2*KARA_TOP_K+i];
	d[2*KARA_TOP_K+i] = d[2*KARA_TOP_K+i] - d[3*KARA_TOP_K+i];
	d[KARA_TOP_K+KARA_TOP_K/2+i] = d[KARA_TOP_K+KARA_TOP_K/2+i] - d[2*KARA_TOP_K+KARA_TOP_K/2+i];//
}


void unrolled_kara_mem_bottom_8(const uint16_t* a, const uint8_t* c, uint16_t* d)
{
	int i;
	// uint16_t kara_tmp[16];

//loop1 & loop1_2
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[KARA_BOTTOM_K+i] = d[i] + d[KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] + d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]  + d[KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K-1+i] = a[i] + a[KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K/2+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		kara_tmp[i] = d[3*KARA_BOTTOM_K-1+i] + d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i];//XXX: global variable asm
		d[2*KARA_BOTTOM_K+i] = 0;
	}
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = (int8_t)(c[i] + c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]);
	}

	school_book_mul2_16_fast_mem(kara_tmp, &d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[KARA_BOTTOM_K+KARA_BOTTOM_K/2]);

//loop2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] + d[2*KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
		kara_tmp[i] = (int8_t)(c[i] + c[KARA_BOTTOM_K+i]);//XXX: global variable asm
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
	kara_tmp[i] = (int8_t)(c[i] + c[KARA_BOTTOM_K+i]);//XXX: global variable asm

	school_book_mul2_16_fast_mem(kara_tmp, &d[3*KARA_BOTTOM_K-1], &d[KARA_BOTTOM_K]);

//loop3
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
		kara_tmp[i] = (int8_t)(c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]);//XXX: global variable asm
	}
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	kara_tmp[i] = (int8_t)(c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]);//XXX: global variable asm

	school_book_mul2_16_fast_mem(kara_tmp, &d[KARA_BOTTOM_K/2+3*KARA_BOTTOM_K-1], &d[2*KARA_BOTTOM_K]);

//loop4 & loop4_2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K/2+i] = d[i] + d[KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[3*KARA_BOTTOM_K-1+i] = d[2*KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+i];

		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+i];//
		d[KARA_BOTTOM_K+i] = 0;//
	}
	d[KARA_BOTTOM_K/2+i] = d[i] + d[KARA_BOTTOM_K/2+i];
	d[3*KARA_BOTTOM_K-1+i] = d[2*KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+i];
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+i];//
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[i] + a[KARA_BOTTOM_K/2+i];
		kara_tmp[i] = (int8_t)(c[i] + c[KARA_BOTTOM_K/2+i]);//XXX: global variable asm
	}

	school_book_mul2_16_fast_mem(kara_tmp, &d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[KARA_BOTTOM_K/2]);

//loop5
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K/2+i] + d[KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K/2+i] = 0;
		kara_tmp[i] = (int8_t)c[i];//8-bit secret support
	}
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K/2+i];
	d[KARA_BOTTOM_K/2+i] = 0;
	kara_tmp[i] = (int8_t)c[i];//8-bit secret support

	//school_book_mul2_16_fast_mem(a, c, d);
	school_book_mul2_16_fast_mem(a, kara_tmp, d);

//loop6
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
		kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K/2];//8-bit secret support
	}
	d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K/2];//8-bit secret support

	//school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K/2], &c[KARA_BOTTOM_K/2], &d[KARA_BOTTOM_K]);
	school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K/2], kara_tmp, &d[KARA_BOTTOM_K]);

//loop7 & loop7_2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K-1+i] - d[i];
		d[3*KARA_BOTTOM_K-1+i] = 0;
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+i] + d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K/2+i];//
		kara_tmp[i] = (int8_t)(c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]);
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	}
	d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K-1+i] - d[i];
	d[3*KARA_BOTTOM_K-1+i] = 0;
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K/2+i];//
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+i] + d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
	kara_tmp[i] = (int8_t)(c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]);
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	// for (i = 0 ; i < KARA_BOTTOM_K/2; ++i) {
		// kara_tmp[i] = c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		// d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	// }

	school_book_mul2_16_fast_mem(kara_tmp, &d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2]);

//loop8
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] + d[3*KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
		kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K];//8-bit secret support
	}
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
	kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K];//8-bit secret support

	//school_book_mul2_16_fast_mem( &a[KARA_BOTTOM_K], &c[KARA_BOTTOM_K], &d[2*KARA_BOTTOM_K] );
	school_book_mul2_16_fast_mem( &a[KARA_BOTTOM_K], kara_tmp, &d[2*KARA_BOTTOM_K] );

//loop9
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[3*KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[2*KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
		kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K+KARA_BOTTOM_K/2];//8-bit secret support
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[2*KARA_BOTTOM_K+i];
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	kara_tmp[i] = (int8_t)c[i+KARA_BOTTOM_K+KARA_BOTTOM_K/2];//8-bit secret support

	//school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K+KARA_BOTTOM_K/2], &c[KARA_BOTTOM_K+KARA_BOTTOM_K/2], &d[3*KARA_BOTTOM_K]);
	school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K+KARA_BOTTOM_K/2], kara_tmp, &d[3*KARA_BOTTOM_K]);

//loop10
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
		d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+i];

		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
}

void unrolled_kara_mem_bottom_16(const uint16_t* a, const uint16_t* c, uint16_t* d)
{
	int i;
	// uint16_t kara_tmp[16];

//loop1 & loop1_2
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[KARA_BOTTOM_K+i] = d[i] + d[KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] + d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i]  + d[KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K-1+i] = a[i] + a[KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K/2+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		kara_tmp[i] = d[3*KARA_BOTTOM_K-1+i] + d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i];//XXX: global variable asm
		d[2*KARA_BOTTOM_K+i] = 0;
	}
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = c[i] + c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	}

	school_book_mul2_16_fast_mem(kara_tmp, &d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[KARA_BOTTOM_K+KARA_BOTTOM_K/2]);

//loop2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] + d[2*KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
		kara_tmp[i] = c[i] + c[KARA_BOTTOM_K+i];//XXX: global variable asm
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
	kara_tmp[i] = c[i] + c[KARA_BOTTOM_K+i];//XXX: global variable asm

	school_book_mul2_16_fast_mem(kara_tmp, &d[3*KARA_BOTTOM_K-1], &d[KARA_BOTTOM_K]);

//loop3
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
		kara_tmp[i] = c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//XXX: global variable asm
	}
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	kara_tmp[i] = c[KARA_BOTTOM_K/2+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//XXX: global variable asm

	school_book_mul2_16_fast_mem(kara_tmp, &d[KARA_BOTTOM_K/2+3*KARA_BOTTOM_K-1], &d[2*KARA_BOTTOM_K]);

//loop4 & loop4_2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K/2+i] = d[i] + d[KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[3*KARA_BOTTOM_K-1+i] = d[2*KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+i];

		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+i];//
		d[KARA_BOTTOM_K+i] = 0;//
	}
	d[KARA_BOTTOM_K/2+i] = d[i] + d[KARA_BOTTOM_K/2+i];
	d[3*KARA_BOTTOM_K-1+i] = d[2*KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+i];
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+i];//
	for (i = 0; i < KARA_BOTTOM_K/2; ++i) {
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[i] + a[KARA_BOTTOM_K/2+i];
		kara_tmp[i] = c[i] + c[KARA_BOTTOM_K/2+i];//XXX: global variable asm
	}

	school_book_mul2_16_fast_mem(kara_tmp, &d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[KARA_BOTTOM_K/2]);

//loop5
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K/2+i] + d[KARA_BOTTOM_K+i];
		d[KARA_BOTTOM_K/2+i] = 0;
	}
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[KARA_BOTTOM_K/2+i];
	d[KARA_BOTTOM_K/2+i] = 0;

	school_book_mul2_16_fast_mem(a, c, d);

//loop6
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[i];
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	}
	d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;

	school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K/2], &c[KARA_BOTTOM_K/2], &d[KARA_BOTTOM_K]);

//loop7 & loop7_2
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+i] + d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K-1+i] - d[i];
		d[3*KARA_BOTTOM_K-1+i] = 0;
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+i] + d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K/2+i];//
		kara_tmp[i] = c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	}
	d[KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K/2+i] - d[KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K-1+i] - d[i];
	d[3*KARA_BOTTOM_K-1+i] = 0;
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[KARA_BOTTOM_K/2+i];//
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+i] + d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
	kara_tmp[i] = c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	// for (i = 0 ; i < KARA_BOTTOM_K/2; ++i) {
		// kara_tmp[i] = c[KARA_BOTTOM_K+i] + c[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		// d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = a[KARA_BOTTOM_K+i] + a[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	// }

	school_book_mul2_16_fast_mem(kara_tmp, &d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1], &d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2]);

//loop8
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] + d[3*KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;
	}
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = 0;

	school_book_mul2_16_fast_mem( &a[KARA_BOTTOM_K], &c[KARA_BOTTOM_K], &d[2*KARA_BOTTOM_K] );

//loop9
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[3*KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[2*KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] - d[2*KARA_BOTTOM_K+i];
	d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2-1+i] = 0;

	school_book_mul2_16_fast_mem(&a[KARA_BOTTOM_K+KARA_BOTTOM_K/2], &c[KARA_BOTTOM_K+KARA_BOTTOM_K/2], &d[3*KARA_BOTTOM_K]);

//loop10
	for (i = 0; i < KARA_BOTTOM_K/2-1; ++i) {
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+i];
		d[3*KARA_BOTTOM_K+i] = d[3*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
		d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+i];
		d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+i];

		d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];
		d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
	}
	d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[3*KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+i] = d[KARA_BOTTOM_K+i] - d[2*KARA_BOTTOM_K+i];
	d[2*KARA_BOTTOM_K+i] = d[2*KARA_BOTTOM_K+i] - d[3*KARA_BOTTOM_K+i];
	d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] = d[KARA_BOTTOM_K+KARA_BOTTOM_K/2+i] - d[2*KARA_BOTTOM_K+KARA_BOTTOM_K/2+i];//
}

