#include<stdlib.h>


const char* dgemm_desc = "dgemm with all the fixins";

#ifndef BLOCK_SIZE
#define BLOCK_SIZE ((int) 64)
#endif

/*
  A is M-by-K
  B is K-by-N
  C is M-by-N
  lda is the leading dimension of the matrix (the M of square_dgemm).
*/
void basic_dgemm(const int lda, const int M, const int N, const int K,
                 const double* restrict A,
                 const double* restrict B,
                 double* restrict C)
{
    int i, j, k;

    double* AA = (double*) malloc(M * K * sizeof(double));

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < K; ++j) {
            AA[i*K + j] = A[j*lda + i];
        }
    }
/*    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < K; ++j) {
            BB[i*K + j] = B[i*lda + j];
        }
    }
*/
    for (j = 0; j < N; ++j) {
        for (i = 0; i < M; ++i) {
            double cij = C[j*lda+i];
            for (k = 0; k < K; ++k) {
                cij += AA[i*K+k] * B[j*lda+k];
            }
            C[j*lda+i] = cij;
        }
    }

    free(AA);
}

void do_block(const int lda,
              const double* restrict A,
              const double* restrict B,
              double* restrict C,
              const int i, const int j, const int k)
{
    const int M = (i+BLOCK_SIZE > lda? lda-i : BLOCK_SIZE);
    const int N = (j+BLOCK_SIZE > lda? lda-j : BLOCK_SIZE);
    const int K = (k+BLOCK_SIZE > lda? lda-k : BLOCK_SIZE);
    basic_dgemm(lda, M, N, K,
                A + i + k*lda, B + k + j*lda, C + i + j*lda);
}

void square_dgemm(const int M,
                  const double* restrict A,
                  const double* restrict B,
                  double* restrict C)
{
    const int n_blocks = M / BLOCK_SIZE + (M%BLOCK_SIZE? 1 : 0);
    int bi, bj, bk;
    for (bi = 0; bi < n_blocks; ++bi) {
        const int i = bi * BLOCK_SIZE;
        for (bk = 0; bk < n_blocks; ++bk) {
            const int k = bk * BLOCK_SIZE;
            for (bj = 0; bj < n_blocks; ++bj) {
                const int j = bj * BLOCK_SIZE;
                do_block(M, A, B, C, i, j, k);
            }
        }
    }
}