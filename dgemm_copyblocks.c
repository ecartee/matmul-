#include<stdlib.h>

const char* dgemm_desc = "dgemm with copying each block into fixed memory";

#ifndef BLOCK_SIZE
#define BLOCK_SIZE ((int) 256)
#endif

/* Assume A has been changed to row-major order */
void multiply_blocks(const double* restrict A, const double* restrict B, double* restrict C)
{
    int i, j, k;
    double cij;
    
    for (i = 0; i < BLOCK_SIZE; ++i) {
        for (j = 0; j < BLOCK_SIZE; ++j) {
            cij = C[j*BLOCK_SIZE+i];
            for (k = 0; k < BLOCK_SIZE; ++k) {
                cij += A[i*BLOCK_SIZE+k] * B[j*BLOCK_SIZE+k];
            }
            C[j*BLOCK_SIZE+i] = cij;
        }
    }
}

/* Copy matrix B into a block starting at entry (i,j), padding with zeroes if necessary */
// Both column-major
void copy_block(const double* restrict B, double* restrict B_block, const int i, const int j, const int lda)
{
    int bi,bj;
    int out_i,out_j;
    for(bj = 0; bj < BLOCK_SIZE; ++bj) {
        out_j = j + bj >= lda;
        for(bi = 0; bi < BLOCK_SIZE; ++bi) {
            out_i = i + bi >= lda; // Can just inline this?
            B_block[bj*BLOCK_SIZE+bi] = (out_i || out_j ? 0.0 : B[(j+bj)*lda+(i+bi)]);
        }
    }
}

/* Copy matrix A into a block, changing from column-major to row-major, padding with zeroes if necessary */
void copy_transpose(const double* restrict A, double* restrict A_block, const int i, const int j, const int lda)
{
    int bi,bj;
    int out_i,out_j;
    for(bj = 0; bj < BLOCK_SIZE; ++bj) {
        out_j = j + bj >= lda;
        for(bi = 0; bi < BLOCK_SIZE; ++bi) {
            out_i = i + bi >= lda;
            A_block[bi*BLOCK_SIZE+bj] = (out_i || out_j ? 0.0 : A[(j+bj)*lda+(i+bi)]);
        }
    }
}

/* Copy C_block back into MxN C, getting rid of padded zeroes */
void store_block(double* restrict C, const double* restrict C_block,const int i, const int j, const int lda)
{
    int bi,bj;
    int out_i,out_j;
    for(bj = 0; bj < BLOCK_SIZE; ++bj) {
        out_j = j + bj >= lda;
        for(bi = 0; bi < BLOCK_SIZE; ++bi) {
            out_i = i + bi >= lda;
            if(!(out_i || out_j)) {
                C[(j+bj)*lda+(i+bi)] = C_block[bj*BLOCK_SIZE+bi];
            }
        }
    }
}

void square_dgemm(const int M,
                  const double* restrict A,
                  const double* restrict B,
                  double* restrict C)
{
    // Const anywhere here????
    double* A_block = malloc(BLOCK_SIZE*BLOCK_SIZE*sizeof(double));
    double* B_block = malloc(BLOCK_SIZE*BLOCK_SIZE*sizeof(double));
    double* C_block = malloc(BLOCK_SIZE*BLOCK_SIZE*sizeof(double));
    
    const int n_blocks = M / BLOCK_SIZE + (M%BLOCK_SIZE? 1 : 0);
    int bi, bj, bk;
    for (bi = 0; bi < n_blocks; ++bi) {
        const int i = bi * BLOCK_SIZE;
        for (bk = 0; bk < n_blocks; ++bk) {
            const int k = bk * BLOCK_SIZE;
            copy_transpose(A,A_block,i,k,M);
            for (bj = 0; bj < n_blocks; ++bj) {
                const int j = bj * BLOCK_SIZE;
                copy_block(B,B_block,k,j,M);
                copy_block(C,C_block,i,j,M);
                multiply_blocks(A_block,B_block,C_block);
                store_block(C,C_block,i,j,M);
            }
        }
    }
    free(A_block);
    free(B_block);
    free(C_block);
}