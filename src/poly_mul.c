#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "poly_mul.h"

void TC_evaluation_16(const uint32_t* a1, uint32_t bw_ar[NUM_POLY_MID][N_SB_16], uint32_t *w1,  uint32_t *w2,  uint32_t *w3,  uint32_t *w4,  uint32_t *w5,  uint32_t *w6,  uint32_t *w7);

void TC_interpol_16(uint32_t *w1, uint32_t *w2, uint32_t *w3, uint32_t *w4, uint32_t *w5, uint32_t *w6, uint32_t *w7, uint32_t *result);

static inline int16_t reduce(int16_t a, int64_t p);

void print_poly2(int16_t *a, int64_t n, uint64_t p){

	printf("-----------------------\n");
	int i;
	for (i = n - 1; i >= 0; i--){
		if (a[i] != 0){
			if(i!=0)
				printf("  Mod(%d,%lu)*x^%d + ", a[i], p,i);
			else
				printf("  Mod(%d,%lu)*x^%d ", a[i], p,i);

			}
	}

	printf("\n-----------------------\n");
}

void pol_mul(uint16_t* a, uint16_t* b, uint16_t* res, uint16_t p, uint32_t n)

{ 
	// Polynomial multiplication using the schoolbook method, c[x] = a[x]*b[x] 
	// SECURITY NOTE: TO BE USED FOR TESTING ONLY.  

	uint32_t i;

//-------------------normal multiplication-----------------

	uint16_t c[512];

	for (i = 0; i < 512; i++) c[i] = 0;

	toom_cook_4way(a, b, c);

	//---------------reduction-------
	for(i=n;i<2*n;i++){
		res[i-n]=(c[i-n]-c[i])&(p-1);
	}
	

}
//----------------------------------lazy interpolation--------------------------------

void pol_mul_noreduce_32(const uint32_t* a, const uint32_t* b, uint32_t* res, uint32_t n) //simple school book without polynomial reduction. For 32 bits 
{ 
	uint32_t i, j;
	
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			res[i+j]=res[i+j] + (a[i] * b[j]);
		}
	}
}


void evaluation_single(const uint16_t *b, uint32_t bw_ar[7][NUM_POLY_MID][N_SB_16]){ // for precomputing B

	uint32_t r0, r1, r2, r3, r4, r5, r6, r7;

	uint32_t bw1[N_SB], bw2[N_SB], bw3[N_SB], bw4[N_SB], bw5[N_SB], bw6[N_SB], bw7[N_SB]; //can be reduced

	uint16_t *B0, *B1, *B2, *B3;

	int j;

	B0 = (uint16_t*)b;
	B1 = (uint16_t*)&b[N_SB];
	B2 = (uint16_t*)&b[2*N_SB];
	B3 = (uint16_t*)&b[3*N_SB];

	for (j = 0; j < N_SB; ++j) {
		r0 = B0[j];
		r1 = B1[j];
		r2 = B2[j];
		r3 = B3[j];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw3[j] = r6;
		bw4[j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw5[j] = r6;
		bw6[j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw2[j] = r4; 
		bw7[j] = r0;
		bw1[j] = r3;
	}

	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw1[j];
		r1 = bw1[j+N_SB_16];
		r2 = bw1[j+2*N_SB_16];
		r3 = bw1[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[0][2][j] = r6;
		bw_ar[0][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[0][4][j] = r6;
		bw_ar[0][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[0][1][j] = r4; 
		bw_ar[0][6][j] = r0;
		bw_ar[0][0][j] = r3;
	}


	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw2[j];
		r1 = bw2[j+N_SB_16];
		r2 = bw2[j+2*N_SB_16];
		r3 = bw2[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[1][2][j] = r6;
		bw_ar[1][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[1][4][j] = r6;
		bw_ar[1][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[1][1][j] = r4; 
		bw_ar[1][6][j] = r0;
		bw_ar[1][0][j] = r3;
	}

	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw3[j];
		r1 = bw3[j+N_SB_16];
		r2 = bw3[j+2*N_SB_16];
		r3 = bw3[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[2][2][j] = r6;
		bw_ar[2][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[2][4][j] = r6;
		bw_ar[2][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[2][1][j] = r4; 
		bw_ar[2][6][j] = r0;
		bw_ar[2][0][j] = r3;
	}


	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw4[j];
		r1 = bw4[j+N_SB_16];
		r2 = bw4[j+2*N_SB_16];
		r3 = bw4[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[3][2][j] = r6;
		bw_ar[3][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[3][4][j] = r6;
		bw_ar[3][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[3][1][j] = r4; 
		bw_ar[3][6][j] = r0;
		bw_ar[3][0][j] = r3;
	}

	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw5[j];
		r1 = bw5[j+N_SB_16];
		r2 = bw5[j+2*N_SB_16];
		r3 = bw5[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[4][2][j] = r6;
		bw_ar[4][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[4][4][j] = r6;
		bw_ar[4][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[4][1][j] = r4; 
		bw_ar[4][6][j] = r0;
		bw_ar[4][0][j] = r3;
	}


	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw6[j];
		r1 = bw6[j+N_SB_16];
		r2 = bw6[j+2*N_SB_16];
		r3 = bw6[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[5][2][j] = r6;
		bw_ar[5][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[5][4][j] = r6;
		bw_ar[5][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[5][1][j] = r4; 
		bw_ar[5][6][j] = r0;
		bw_ar[5][0][j] = r3;
	}

	for (j = 0; j < N_SB_16; ++j) {
		r0 = bw7[j];
		r1 = bw7[j+N_SB_16];
		r2 = bw7[j+2*N_SB_16];
		r3 = bw7[j+3*N_SB_16];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[6][2][j] = r6;
		bw_ar[6][3][j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw_ar[6][4][j] = r6;
		bw_ar[6][5][j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw_ar[6][1][j] = r4; 
		bw_ar[6][6][j] = r0;
		bw_ar[6][0][j] = r3;
	}


}

void TC_evaluation_64_unrolled(const uint16_t* a1, uint32_t bw_ar[7][NUM_POLY_MID][N_SB_16], uint32_t w_ar[7][NUM_POLY_MID][N_SB_16_RES])//TC+TC unrolled
{

	uint32_t aw1[N_SB], aw2[N_SB], aw3[N_SB], aw4[N_SB], aw5[N_SB], aw6[N_SB], aw7[N_SB];

	uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
	uint16_t *A0, *A1, *A2, *A3;

	A0 = (uint16_t*)a1;
	A1 = (uint16_t*)&a1[N_SB];
	A2 = (uint16_t*)&a1[2*N_SB];
	A3 = (uint16_t*)&a1[3*N_SB];

	int j;

// EVALUATION
	for (j = 0; j < N_SB; ++j) {
		r0 = A0[j];
		r1 = A1[j];
		r2 = A2[j];
		r3 = A3[j];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw3[j] = r6;
		aw4[j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw5[j] = r6;
		aw6[j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		aw2[j] = r4; aw7[j] = r0;
		aw1[j] = r3;
	}

// MULTIPLICATION

	TC_evaluation_16(aw1, bw_ar[0], w_ar[0][0], w_ar[0][1], w_ar[0][2], w_ar[0][3], w_ar[0][4], w_ar[0][5], w_ar[0][6]);
	TC_evaluation_16(aw2, bw_ar[1], w_ar[1][0], w_ar[1][1], w_ar[1][2], w_ar[1][3], w_ar[1][4], w_ar[1][5], w_ar[1][6]);
	TC_evaluation_16(aw3, bw_ar[2], w_ar[2][0], w_ar[2][1], w_ar[2][2], w_ar[2][3], w_ar[2][4], w_ar[2][5], w_ar[2][6]);
	TC_evaluation_16(aw4, bw_ar[3], w_ar[3][0], w_ar[3][1], w_ar[3][2], w_ar[3][3], w_ar[3][4], w_ar[3][5], w_ar[3][6]);	
	TC_evaluation_16(aw5, bw_ar[4], w_ar[4][0], w_ar[4][1], w_ar[4][2], w_ar[4][3], w_ar[4][4], w_ar[4][5], w_ar[4][6]);
	TC_evaluation_16(aw6, bw_ar[5], w_ar[5][0], w_ar[5][1], w_ar[5][2], w_ar[5][3], w_ar[5][4], w_ar[5][5], w_ar[5][6]);
	TC_evaluation_16(aw7, bw_ar[6], w_ar[6][0], w_ar[6][1], w_ar[6][2], w_ar[6][3], w_ar[6][4], w_ar[6][5], w_ar[6][6]);

}

void TC_interpol_64_unrolled(uint32_t w_ar[7][NUM_POLY_MID][N_SB_16_RES], uint16_t *result){ //unrolled

	//printf("\nInterpolation called\n");

	uint32_t r0, r1, r2, r3, r4, r5, r6;
	uint16_t inv3 = 43691, inv9 = 36409, inv15 = 61167;

	uint32_t w1[N_SB_RES] = {0}, w2[N_SB_RES] = {0}, w3[N_SB_RES] = {0}, w4[N_SB_RES] = {0}, w5[N_SB_RES] = {0}, w6[N_SB_RES] = {0}, w7[N_SB_RES] = {0};

	int i;
	
	uint16_t * C;
	C = result;

	TC_interpol_16(w_ar[0][0], w_ar[0][1], w_ar[0][2], w_ar[0][3], w_ar[0][4], w_ar[0][5], w_ar[0][6], w1);
	TC_interpol_16(w_ar[1][0], w_ar[1][1], w_ar[1][2], w_ar[1][3], w_ar[1][4], w_ar[1][5], w_ar[1][6], w2);
	TC_interpol_16(w_ar[2][0], w_ar[2][1], w_ar[2][2], w_ar[2][3], w_ar[2][4], w_ar[2][5], w_ar[2][6], w3);
	TC_interpol_16(w_ar[3][0], w_ar[3][1], w_ar[3][2], w_ar[3][3], w_ar[3][4], w_ar[3][5], w_ar[3][6], w4);
	TC_interpol_16(w_ar[4][0], w_ar[4][1], w_ar[4][2], w_ar[4][3], w_ar[4][4], w_ar[4][5], w_ar[4][6], w5);
	TC_interpol_16(w_ar[5][0], w_ar[5][1], w_ar[5][2], w_ar[5][3], w_ar[5][4], w_ar[5][5], w_ar[5][6], w6);
	TC_interpol_16(w_ar[6][0], w_ar[6][1], w_ar[6][2], w_ar[6][3], w_ar[6][4], w_ar[6][5], w_ar[6][6], w7);	

	for (i = 0; i < N_SB_RES; ++i) {
		r0 = w1[i];
		r1 = w2[i];
		r2 = w3[i];
		r3 = w4[i];
		r4 = w5[i];
		r5 = w6[i];
		r6 = w7[i];

		r1 = r1 + r4;
		r5 = r5 - r4;
		r3 = ((r3-r2) >> 1);
		r4 = r4 - r0;
		r4 = r4 - (r6 << 6);
		r4 = (r4 << 1) + r5;
		r2 = r2 + r3;
		r1 = r1 - (r2 << 6) - r2;
		r2 = r2 - r6;
		r2 = r2 - r0;
		r1 = r1 + 45*r2;
		r4 = (((r4 - (r2 << 3))*inv3) >> 3);
		r5 = r5 + r1;
		r1 = (((r1 + (r3 << 4))*inv9) >> 1);
		r3 = -(r3 + r1);
		r5 = (((30*r1 - r5)*inv15) >> 2);
		r2 = r2 - r4;
		r1 = r1 - r5;

		C[i]     += r6;
		C[i+64]  += r5;
		C[i+128] += r4;
		C[i+192] += r3;
		C[i+256] += r2;
		C[i+320] += r1;
		C[i+384] += r0;
	}



}


void TC_evaluation_16(const uint32_t* a1, uint32_t bw_ar[NUM_POLY_MID][N_SB_16], uint32_t *w1,  uint32_t *w2,  uint32_t *w3,  uint32_t *w4,  uint32_t *w5,  uint32_t *w6,  uint32_t *w7){

	uint32_t aw1[N_SB_16], aw2[N_SB_16], aw3[N_SB_16], aw4[N_SB_16], aw5[N_SB_16], aw6[N_SB_16], aw7[N_SB_16];

	uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
	uint32_t *A0, *A1, *A2, *A3;
	A0 = (uint32_t*)a1;
	A1 = (uint32_t*)&a1[N_SB_16];
	A2 = (uint32_t*)&a1[2*N_SB_16];
	A3 = (uint32_t*)&a1[3*N_SB_16];
	/*
	B0 = (uint32_t*)b1;
	B1 = (uint32_t*)&b1[N_SB_16];
	B2 = (uint32_t*)&b1[2*N_SB_16];
	B3 = (uint32_t*)&b1[3*N_SB_16];
	*/
	int j;

// EVALUATION
	for (j = 0; j < N_SB_16; ++j) {
		r0 = A0[j];
		r1 = A1[j];
		r2 = A2[j];
		r3 = A3[j];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw3[j] = r6;
		aw4[j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw5[j] = r6;
		aw6[j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		aw2[j] = r4; aw7[j] = r0;
		aw1[j] = r3;
	}
// MULTIPLICATION

	pol_mul_noreduce_32(aw1,bw_ar[0],w1, 16);
	pol_mul_noreduce_32(aw2,bw_ar[1],w2, 16);
	pol_mul_noreduce_32(aw3,bw_ar[2],w3, 16);
	pol_mul_noreduce_32(aw4,bw_ar[3],w4, 16);
	pol_mul_noreduce_32(aw5,bw_ar[4],w5, 16);
	pol_mul_noreduce_32(aw6,bw_ar[5],w6, 16);
	pol_mul_noreduce_32(aw7,bw_ar[6],w7, 16);
}

void TC_interpol_16(uint32_t *w1, uint32_t *w2, uint32_t *w3, uint32_t *w4, uint32_t *w5, uint32_t *w6, uint32_t *w7, uint32_t *result){


	uint32_t r0, r1, r2, r3, r4, r5, r6;
	uint32_t inv3 = 174763, inv9 = 233017, inv15 = 454383;
	uint32_t * C;
	C = result;
	int i;

		for (i = 0; i < N_SB_16_RES; ++i) {
		r0 = w1[i];
		r1 = w2[i];
		r2 = w3[i];
		r3 = w4[i];
		r4 = w5[i];
		r5 = w6[i];
		r6 = w7[i];

		r1 = r1 + r4;
		r5 = r5 - r4;
		r3 = ((r3-r2) >> 1);
		r4 = r4 - r0;
		r4 = r4 - (r6 << 6);
		r4 = (r4 << 1) + r5;
		r2 = r2 + r3;
		r1 = r1 - (r2 << 6) - r2;
		r2 = r2 - r6;
		r2 = r2 - r0;
		r1 = r1 + 45*r2;
		r4 = (((r4 - (r2 << 3))*inv3) >> 3);
		r5 = r5 + r1;
		r1 = (((r1 + (r3 << 4))*inv9) >> 1);
		r3 = -(r3 + r1);
		r5 = (((30*r1 - r5)*inv15) >> 2);
		r2 = r2 - r4;
		r1 = r1 - r5;
		
		C[i]    +=r6;
		C[i+16] +=r5;
		C[i+32] +=r4;
		C[i+48] +=r3;
		C[i+64] +=r2;
		C[i+80] +=r1;
		C[i+96] +=r0;
		
	}



}



//----------------------------------lazy interpolation ends---------------------------


void karatsuba_simple(const uint16_t* a_1,const uint16_t* b_1, uint16_t* result_final){//uses 10 registers

	uint16_t N=64;
	uint16_t d01[N/2-1];
	uint16_t d0123[N/2-1];
	uint16_t d23[N/2-1];
	uint16_t result_d01[N-1];	

	int32_t i,j;

	memset(result_d01,0,(N-1)*sizeof(uint16_t));
	memset(d01,0,(N/2-1)*sizeof(uint16_t));
	memset(d0123,0,(N/2-1)*sizeof(uint16_t));
	memset(d23,0,(N/2-1)*sizeof(uint16_t));
	memset(result_final,0,(2*N-1)*sizeof(uint16_t));

	uint16_t acc1,acc2,acc3,acc4,acc5,acc6,acc7,acc8,acc9,acc10;


	for (i = 0; i < N/4; i++) {
		acc1=a_1[i];//a0
		acc2=a_1[i+N/4];//a1
		acc3=a_1[i+2*N/4];//a2
		acc4=a_1[i+3*N/4];//a3	
		for (j = 0; j < N/4; j++) {

			acc5=b_1[j];//b0
			acc6=b_1[j+N/4];//b1

			result_final[i+j+0*N/4]=result_final[i+j+0*N/4]+acc1*acc5;
			result_final[i+j+2*N/4]=result_final[i+j+2*N/4]+acc2*acc6;

			acc7=acc5+acc6;//b01
			acc8=acc1+acc2;//a01
			d01[i+j]=d01[i+j] + acc7*acc8;
	//--------------------------------------------------------

			acc7=b_1[j+2*N/4];//b2
			acc8=b_1[j+3*N/4];//b3			
			result_final[i+j+4*N/4]=result_final[i+j+4*N/4]+acc7*acc3;

			result_final[i+j+6*N/4]=result_final[i+j+6*N/4]+acc8*acc4;

			acc9=acc3+acc4;
			acc10=acc7+acc8;
			d23[i+j]=d23[i+j] + acc9*acc10;
	//--------------------------------------------------------

			acc5=acc5+acc7;//b02
			acc7=acc1+acc3;//a02
			result_d01[i+j+0*N/4]=result_d01[i+j+0*N/4]+acc5*acc7;

			acc6=acc6+acc8;//b13
			acc8=acc2+acc4;			
			result_d01[i+j+ 2*N/4]=result_d01[i+j+ 2*N/4]+acc6*acc8;

			acc5=acc5+acc6;
			acc7=acc7+acc8;
			d0123[i+j]=d0123[i+j] + acc5*acc7;
		}
	}

//------------------2nd last stage-------------------------

	for(i=0;i<N/2-1;i++){
		d0123[i]=d0123[i]-result_d01[i+0*N/4]-result_d01[i+2*N/4];
		d01[i]=d01[i]-result_final[i+0*N/4]-result_final[i+2*N/4];
		d23[i]=d23[i]-result_final[i+4*N/4]-result_final[i+6*N/4];
	}

	for(i=0;i<N/2-1;i++){
		result_d01[i+1*N/4]=result_d01[i+1*N/4]+d0123[i];
		result_final[i+1*N/4]=result_final[i+1*N/4]+d01[i];
		result_final[i+5*N/4]=result_final[i+5*N/4]+d23[i];
	}

//------------Last stage---------------------------
	for(i=0;i<N-1;i++){
		result_d01[i]=result_d01[i]-result_final[i]-result_final[i+N];
	}
	
	for(i=0;i<N-1;i++){
		result_final[i+1*N/2]=result_final[i+1*N/2]+result_d01[i];//-result_d0[i]-result_d1[i];		
	}

}



void toom_cook_4way (const uint16_t* a1,const uint16_t* b1, uint16_t* result)
{
	uint16_t inv3 = 43691, inv9 = 36409, inv15 = 61167;

	uint16_t aw1[N_SB], aw2[N_SB], aw3[N_SB], aw4[N_SB], aw5[N_SB], aw6[N_SB], aw7[N_SB];
	uint16_t bw1[N_SB], bw2[N_SB], bw3[N_SB], bw4[N_SB], bw5[N_SB], bw6[N_SB], bw7[N_SB];
	uint16_t w1[N_SB_RES] = {0}, w2[N_SB_RES] = {0}, w3[N_SB_RES] = {0}, w4[N_SB_RES] = {0},
			 w5[N_SB_RES] = {0}, w6[N_SB_RES] = {0}, w7[N_SB_RES] = {0};
	uint16_t r0, r1, r2, r3, r4, r5, r6, r7;
	uint16_t *A0, *A1, *A2, *A3, *B0, *B1, *B2, *B3;
	A0 = (uint16_t*)a1;
	A1 = (uint16_t*)&a1[N_SB];
	A2 = (uint16_t*)&a1[2*N_SB];
	A3 = (uint16_t*)&a1[3*N_SB];
	B0 = (uint16_t*)b1;
	B1 = (uint16_t*)&b1[N_SB];
	B2 = (uint16_t*)&b1[2*N_SB];
	B3 = (uint16_t*)&b1[3*N_SB];

	uint16_t * C;
	C = result;

	int i,j;

// EVALUATION
	for (j = 0; j < N_SB; ++j) {
		r0 = A0[j];
		r1 = A1[j];
		r2 = A2[j];
		r3 = A3[j];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw3[j] = r6;
		aw4[j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		aw5[j] = r6;
		aw6[j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		aw2[j] = r4; aw7[j] = r0;
		aw1[j] = r3;
	}
	for (j = 0; j < N_SB; ++j) {
		r0 = B0[j];
		r1 = B1[j];
		r2 = B2[j];
		r3 = B3[j];
		r4 = r0 + r2;
		r5 = r1 + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw3[j] = r6;
		bw4[j] = r7;
		r4 = ((r0 << 2)+r2) << 1;
		r5 = (r1 << 2) + r3;
		r6 = r4 + r5; r7 = r4 - r5;
		bw5[j] = r6;
		bw6[j] = r7;
		r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
		bw2[j] = r4; bw7[j] = r0;
		bw1[j] = r3;
	}

// MULTIPLICATION

	karatsuba_simple(aw1, bw1, w1);
	karatsuba_simple(aw2, bw2, w2);
	karatsuba_simple(aw3, bw3, w3);
	karatsuba_simple(aw4, bw4, w4);
	karatsuba_simple(aw5, bw5, w5);
	karatsuba_simple(aw6, bw6, w6);
	karatsuba_simple(aw7, bw7, w7);

// INTERPOLATION
	for (i = 0; i < N_SB_RES; ++i) {
		r0 = w1[i];
		r1 = w2[i];
		r2 = w3[i];
		r3 = w4[i];
		r4 = w5[i];
		r5 = w6[i];
		r6 = w7[i];

		r1 = r1 + r4;
		r5 = r5 - r4;
		r3 = ((r3-r2) >> 1);
		r4 = r4 - r0;
		r4 = r4 - (r6 << 6);
		r4 = (r4 << 1) + r5;
		r2 = r2 + r3;
		r1 = r1 - (r2 << 6) - r2;
		r2 = r2 - r6;
		r2 = r2 - r0;
		r1 = r1 + 45*r2;
		r4 = (((r4 - (r2 << 3))*inv3) >> 3);
		r5 = r5 + r1;
		r1 = (((r1 + (r3 << 4))*inv9) >> 1);
		r3 = -(r3 + r1);
		r5 = (((30*r1 - r5)*inv15) >> 2);
		r2 = r2 - r4;
		r1 = r1 - r5;

		C[i]     += r6;
		C[i+64]  += r5;
		C[i+128] += r4;
		C[i+192] += r3;
		C[i+256] += r2;
		C[i+320] += r1;
		C[i+384] += r0;
	}
}


static inline int16_t reduce(int16_t a, int64_t p)
{
    return a&(p-1);
}

