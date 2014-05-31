/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose_matrix_32_32(int N, int M, int A[N][M], int B[M][N]) {
    int i, j, n, m;
    int r0, r1, r2, r3, r4, r5, r6, r7;
    // Transpose upper triangle of A into lower triangle of B
    // total misses = 6 blocks x 16 per blocks
    for (i = 0; i < N-8; i += 8) {
        for (j = i + 8; j < M; j += 8) {
            for (n = 0; n < 8; ++n) {
                for (m = 0; m < 8; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }
        }
    }

    // Transpose lower triangle of A into upper triangle of B
    // total misses = 6 blocks x 16 per blocks
    for (i = 8; i < N; i += 8) {
        for (j = 0; j < i; j += 8) {
            for (n = 0; n < 8; ++n) {
                for (m = 0; m < 8; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }
        }
    }

    // Transpose diagonal
    // total misses = 4 x 16
    for (i = 0; i < N; i += 8) {
        // total 16 miss
        for (n = 0; n < 8; ++n) {
            // 1 miss
            r0 = A[i + n][i + 0];
            r1 = A[i + n][i + 1];
            r2 = A[i + n][i + 2];
            r3 = A[i + n][i + 3];
            r4 = A[i + n][i + 4];
            r5 = A[i + n][i + 5];
            r6 = A[i + n][i + 6];
            r7 = A[i + n][i + 7];

            // 1 miss
            B[i + n][i + 0] = r0;
            B[i + n][i + 1] = r1;
            B[i + n][i + 2] = r2;
            B[i + n][i + 3] = r3;
            B[i + n][i + 4] = r4;
            B[i + n][i + 5] = r5;
            B[i + n][i + 6] = r6;
            B[i + n][i + 7] = r7;
        }

        // in-place transpose of upper left and lower right,
        // upper right and lower left
        // total 0 misses
        for (n = 0; n < 4; ++n) {
            for (m = n; m < 4; ++m) {
                r1 = B[i + n][i + m];
                B[i + n][i + m] = B[i + m][i + n];
                B[i + m][i + n] = r1;

                r1 = B[i + n + 4][i + m + 4];
                B[i + n + 4][i + m + 4] = B[i + m + 4][i + n + 4];
                B[i + m + 4][i + n + 4] = r1;

                r1 = B[i + n][i + m + 4];
                B[i + n][i + m + 4] = B[i + m][i + n + 4];
                B[i + m][i + n + 4] = r1;

                r1 = B[i + n + 4][i + m];
                B[i + n + 4][i + m] = B[i + m + 4][i + n];
                B[i + m + 4][i + n] = r1;
            }
        }

        // exchange
        // total misses = 0
        for (n = 0; n < 4; ++n) {
            for (m = 0; m < 4; ++m) {
                r1 = B[i + n][i + m + 4];
                B[i + n][i + m + 4] = B[i + n + 4][i + m];
                B[i + n + 4][i + m] = r1;
            }
        }
    }
}

void transpose_matrix_64_64(int N, int M, int A[N][M], int B[M][N]) {
    int i, j, n, m;
    // use 8 registers as temp
    int r0, r1, r2, r3, r4, r5, r6, r7;
    // Transpose 8x8 diagonal block, numbered 0 ~ 6, from upper left
    // to lower right, the last one will not tranpose now
    // for each diagonal block in [i ~ i+8][i ~ i+8], we borrow the
    // [0 ~ 3][56 ~ 63] (in other words, the last block) as a cache
    for (i = 0; i < 56; i += 8) {
        // copy A upper 4 lines into corresponding B position,
        // no transpose
        // total 12 misses in the first time
        // then decrease to 8 misses
        for (n = 0; n < 4; ++n) {
            // read a line, total 4 misses inside this loop
            r0 = A[i + n][i + 0];
            r1 = A[i + n][i + 1];
            r2 = A[i + n][i + 2];
            r3 = A[i + n][i + 3];
            r4 = A[i + n][i + 4];
            r5 = A[i + n][i + 5];
            r6 = A[i + n][i + 6];
            r7 = A[i + n][i + 7];

            // store in d, totoal 4 misses inside this loop
            B[i + n][i + 0] = r0;
            B[i + n][i + 1] = r1;
            B[i + n][i + 2] = r2;
            B[i + n][i + 3] = r3;
            B[i + n][i + 4] = r4;
            B[i + n][i + 5] = r5;
            B[i + n][i + 6] = r6;
            B[i + n][i + 7] = r7;

            // saved to the borrowed cache, total 4 misses in the first time
            B[n][56] = r4;
            B[n][57] = r5;
            B[n][58] = r6;
            B[n][59] = r7;
        }

        // in-place transpose the upper left, 0 misses,
        // since all element fit into the cache
        for (n = 0; n < 4; ++n) {
            for (m = n; m < 4; ++m) {
                r1 = B[i + n][i + m];
                B[i + n][i + m] = B[i + m][i + n];
                B[i + m][i + n] = r1;
            }
        }

        // copy lower 4 lines
        // total 8 misses
        for (n = 4; n < 8; ++n) {

            r0 = A[i + n][i + 0];
            r1 = A[i + n][i + 1];
            r2 = A[i + n][i + 2];
            r3 = A[i + n][i + 3];
            r4 = A[i + n][i + 4];
            r5 = A[i + n][i + 5];
            r6 = A[i + n][i + 6];
            r7 = A[i + n][i + 7];

            // store in d, totoal 4 misses inside this loop
            B[i + n][i + 0] = r0;
            B[i + n][i + 1] = r1;
            B[i + n][i + 2] = r2;
            B[i + n][i + 3] = r3;
            B[i + n][i + 4] = r4;
            B[i + n][i + 5] = r5;
            B[i + n][i + 6] = r6;
            B[i + n][i + 7] = r7;

            // saved to the borrowed cache, total 0 misses
            B[n - 4][60] = r0;
            B[n - 4][61] = r1;
            B[n - 4][62] = r2;
            B[n - 4][63] = r3;
        }

        // in-place transpose lower right
        // total 0 miss
        for (n = 4; n < 8; ++n) {
            for (m = n; m < 8; ++m) {
                r1 = B[i + n][i + m];
                B[i + n][i + m] = B[i + m][i + n];
                B[i + m][i + n] = r1;
            }
        }

        // in-place transpose two matrixex in the cache
        // total 0 misses
        for (n = 0; n < 4; ++n) {
            for (m = n; m < 4; ++m) {
                r1 = B[n][m + 56];
                B[n][m + 56] = B[m][n + 56];
                B[m][n + 56] = r1;

                r1 = B[n][m + 60];
                B[n][m + 60] = B[m][n + 60];
                B[m][n + 60] = r1;
            }
        }

        // copy the transposed matrix from
        // [0 ~ 3][56 ~ 63] into lower left of B
        // total 0 miss
        for (n = 0; n < 4; ++n) {
            for (m = 0; m < 4; ++m) {
                B[i + n + 4][i + m] = B[n][56 + m];
            }
        }

        // copy the transposed matrix from
        // [0 ~ 3][60 ~ 63] into upper right of B
        // total 4 misses
        for (n = 0; n < 4; ++n) {
            for (m = 0; m < 4; ++m) {
                B[i + n][i + m + 4] = B[n][60 + m];
            }
        }
    }

    // transpose the last diagonal block
    // total 24 misses
    {
        // copy upper 4 lines
        // total 8 misses
        for (n = 0; n < 4; ++n) {
            r0 = A[56 + n][56 + 0];
            r1 = A[56 + n][56 + 1];
            r2 = A[56 + n][56 + 2];
            r3 = A[56 + n][56 + 3];
            r4 = A[56 + n][56 + 4];
            r5 = A[56 + n][56 + 5];
            r6 = A[56 + n][56 + 6];
            r7 = A[56 + n][56 + 7];

            B[56 + n][56 + 0] = r0;
            B[56 + n][56 + 1] = r1;
            B[56 + n][56 + 2] = r2;
            B[56 + n][56 + 3] = r3;
            B[56 + n][56 + 4] = r4;
            B[56 + n][56 + 5] = r5;
            B[56 + n][56 + 6] = r6;
            B[56 + n][56 + 7] = r7;
        }

        // in-place transpose of upper left and upper right
        // total 0 misses
        for (n = 0; n < 4; ++n) {
            for (m = n; m < 4; ++m) {
                r1 = B[n + 56][m + 56];
                B[n + 56][m + 56] = B[m + 56][n + 56];
                B[m + 56][n + 56] = r1;

                r1 = B[n + 56][m + 60];
                B[n + 56][m + 60] = B[m + 56][n + 60];
                B[m + 56][n + 60] = r1;
            }
        }

        // copy lower 4 lines
        // total 8 misses
        for (n = 4; n < 8; ++n) {
            r0 = A[56 + n][56 + 0];
            r1 = A[56 + n][56 + 1];
            r2 = A[56 + n][56 + 2];
            r3 = A[56 + n][56 + 3];
            r4 = A[56 + n][56 + 4];
            r5 = A[56 + n][56 + 5];
            r6 = A[56 + n][56 + 6];
            r7 = A[56 + n][56 + 7];

            B[56 + n][56 + 0] = r0;
            B[56 + n][56 + 1] = r1;
            B[56 + n][56 + 2] = r2;
            B[56 + n][56 + 3] = r3;
            B[56 + n][56 + 4] = r4;
            B[56 + n][56 + 5] = r5;
            B[56 + n][56 + 6] = r6;
            B[56 + n][56 + 7] = r7;
        }

        // in-place transpose of lower left and lower right
        // total 0 miss
        for (n = 0; n < 4; ++n) {
            for (m = n; m < 4; ++m) {
                r1 = B[n + 60][m + 60];
                B[n + 60][m + 60] = B[m + 60][n + 60];
                B[m + 60][n + 60] = r1;

                r1 = B[n + 60][m + 56];
                B[n + 60][m + 56] = B[m + 60][n + 56];
                B[m + 60][n + 56] = r1;
            }
        }

        // exchange upper right and lower left
        // total 12 misses
        for (n = 0; n < 4; ++n) {
            // 1 miss
            r0 = B[56 + n][60 + 0];
            r1 = B[56 + n][60 + 1];
            r2 = B[56 + n][60 + 2];
            r3 = B[56 + n][60 + 3];

            // 1 miss
            r4 = B[60 + n][56 + 0];
            r5 = B[60 + n][56 + 1];
            r6 = B[60 + n][56 + 2];
            r7 = B[60 + n][56 + 3];

            // 0 miss
            B[60 + n][56 + 0] = r0;
            B[60 + n][56 + 1] = r1;
            B[60 + n][56 + 2] = r2;
            B[60 + n][56 + 3] = r3;

            // 1 miss
            B[56 + n][60 + 0] = r4;
            B[56 + n][60 + 1] = r5;
            B[56 + n][60 + 2] = r6;
            B[56 + n][60 + 3] = r7;
        }
    }

    // transpose the upper triangle block of A into
    // lower triangle of B
    // total misses = 28 blocks x 20 misses per block
    for (i = 0; i < 64 - 8; i += 8) {
        for (j = i + 8; j < 64; j += 8) {
            // left side
            for (n = 0; n < 8; ++n) {
                for (m = 0; m < 4; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }
            // right side
            for (n = 7; n >= 0; --n) {
                for (m = 4; m < 8; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }
        }
    }

    // transpose the lower triangle block of A into
    // upper triangle of B
    // totoal misses = 28 blocks x 20 misses per block
    for (i = 8; i < 64; i += 8) {
        for (j = 0; j < i; j += 8) {

            for (n = 0; n < 8; ++n) {
                for (m = 0; m < 4; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }

            for (n = 7; n >= 0; --n) {
                for (m = 4; m < 8; ++m) {
                    B[j+m][i+n] = A[i+n][j+m];
                }
            }
        }
    }
}

void transpose_matrix_67_61(int M, int N, int A[N][M], int B[M][N]) {
    ;
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */

void trans(int M, int N, int A[N][M], int B[M][N]);
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        transpose_matrix_32_32(M, N, A, B);
    }
    else if (M == 64 && N == 64) {
        transpose_matrix_64_64(M, N, A, B);
    } else {
        transpose_matrix_67_61(M, N, A, B);
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}


/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}