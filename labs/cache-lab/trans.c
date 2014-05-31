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

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int n, m;
    // if (M != 32 && N != 32) return;

    if (M == 32 && N == 32) {
        // Loop through upper triangle of A
        for (i = 0; i < N-8; i += 8) {
            for (j = i + 8; j < M; j += 8) {
                for (n = 0; n < 8; ++n) {
                    for (m = 0; m < 8; ++m) {
                        B[j+m][i+n] = A[i+n][j+m];
                    }
                }
            }
        }

        // Loop through lower triangle of A
        for (i = 8; i < N; i += 8) {
            for (j = 0; j < i; j += 8) {
                for (n = 0; n < 8; ++n) {
                    for (m = 0; m < 8; ++m) {
                        B[j+m][i+n] = A[i+n][j+m];
                    }
                }
            }
        }

        // Loop through the diangle of A
        for (i = 0; i < N; i += 8) {
            for (n = 0; n < 8; ++n) {
                for (m = 0; m < 8; ++m) {
                    if (n == m) continue;
                    B[i+m][i+n] = A[i+n][i+m];
                }
                B[i+n][i+n] = A[i+n][i+n];
            }
        }
    }
    if (M == 64 && N == 64) {
        transpose_matrix_64_64(M, N, A, B);
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

/*
 * trans.c - Matrix transpose B = A^T
 *
 * Wei Chen, weichen1@andrew.cmu.edu
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
// #include <stdio.h>
// #include "cachelab.h"

// /***********
//  * helpers
//  * ********/
// int local_min(int a, int b) {
//     return a < b ? a : b;
// }


// int is_transpose(int M, int N, int A[N][M], int B[M][N]);

// void transpose_dia_block(int M, int N, int A[N][M], int B[M][N], int sizea, int sizeb){
//     int ii, jj, i, j;
//     int tmp;

//     for(ii = 0; ii < N; ii += sizea) {
//         for(jj = 0; jj < M; jj += sizeb) {
//             for(i = ii; i < local_min(ii + sizea, N); ++i) {
//                 for(j = jj; j < local_min(jj + sizeb, M); ++j) {
//                     if (i - ii != j - jj) {
//                         B[j][i] = A[i][j];
//                     } else {
//                         tmp = A[i][j];
//                     }
//                 }
//                 for(j = jj; j < local_min(jj + sizeb, M); ++j) {
//                     if (i - ii == j - jj) {
//                         B[j][i] = tmp;
//                     }
//                 }
//             }
//         }
//     }
// }

// char transpose_32_32_desc[] = "Transpose big sub matrix, and smaller matrix";
// void transpose_32_32(int M, int N, int A[N][M], int B[M][N]){
//     int ii, jj, i, j;
//     int a1, a2, a3, a4, a5, a6, a7, a8;

//     for(ii = 0; ii < N; ii += 8) {
//         for(jj = 0; jj < M; jj += 8) {
//             if ( ii != jj ) {
//                 // up 4 lines
//                 for (i = 0; i < 4; ++i) {
//                     a1 = A[ii + i][jj];
//                     a2 = A[ii + i][jj+1];
//                     a3 = A[ii + i][jj+2];
//                     a4 = A[ii + i][jj+3];
//                     a5 = A[ii + i][jj+4];
//                     a6 = A[ii + i][jj+5];
//                     a7 = A[ii + i][jj+6];
//                     a8 = A[ii + i][jj+7];

//                     B[jj + i][ii] = a1;
//                     B[jj + i][ii+1] = a2;
//                     B[jj + i][ii+2] = a3;
//                     B[jj + i][ii+3] = a4;
//                     B[jj + i][ii+4] = a5;
//                     B[jj + i][ii+5] = a6;
//                     B[jj + i][ii+6] = a7;
//                     B[jj + i][ii+7] = a8;
//                 }
//                 for (i = jj; i < jj + 4; ++i) {
//                     for (j = ii + i - jj; j < ii + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii][ii + i - jj];
//                         B[jj + j - ii][ii + i - jj] = a1;
//                     }
//                 } // transpose upper left
//                 // down 4 lines
//                 for (i = 0; i < 4; ++i) {
//                     a1 = B[jj + i][ii + 4];
//                     a2 = B[jj + i][ii + 5];
//                     a3 = B[jj + i][ii + 6];
//                     a4 = B[jj + i][ii + 7];

//                     a5 = A[ii + 4 + i][jj];
//                     a6 = A[ii + 4 + i][jj+1];
//                     a7 = A[ii + 4 + i][jj+2];
//                     a8 = A[ii + 4 + i][jj+3]; // = 4 miss

//                     B[jj + i][ii + 4] = a5;
//                     B[jj + i][ii + 5] = a6;
//                     B[jj + i][ii + 6] = a7;
//                     B[jj + i][ii + 7] = a8; // = 0 miss

//                     B[jj + 4 + i][ii] = a1;
//                     B[jj + 4 + i][ii+1] = a2;
//                     B[jj + 4 + i][ii+2] = a3;
//                     B[jj + 4 + i][ii+3] = a4; // = 4 miss

//                     a1 = A[ii + 4 + i][jj + 4];
//                     a2 = A[ii + 4 + i][jj + 5];
//                     a3 = A[ii + 4 + i][jj + 6];
//                     a4 = A[ii + 4 + i][jj + 7]; // = 0 miss

//                     B[jj + 4 + i][ii + 4] = a1;
//                     B[jj + 4 + i][ii + 5] = a2;
//                     B[jj + 4 + i][ii + 6] = a3;
//                     B[jj + 4 + i][ii + 7] = a4; // = 0 miss
//                 }

//                 for (i = jj + 4; i < jj + 8; ++i) {
//                     for (j = ii + i - jj; j < ii + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii][ii + i - jj];
//                         B[jj + j - ii][ii + i - jj] = a1;
//                     }
//                 } // transpose down right, = 0 miss

//                 for (i = jj + 4; i < jj + 8; ++i) {
//                     for (j = ii + i - jj - 4; j < ii + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + 4 + j - ii][ii + i - jj - 4];
//                         B[jj + 4 + j - ii][ii + i - jj - 4] = a1;
//                     }
//                 } // down left, = 0 miss

//                 for (i = jj; i < jj + 4; ++i) {
//                     for (j = ii + 4 + i - jj; j < ii + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii - 4][ii + 4 + i - jj];
//                         B[jj + j - ii - 4][ii + 4 + i - jj] = a1;
//                     }
//                 } // up right, = 4 miss
//             } else if (ii == jj) {
//                 // up 4 lines, = 8
//                 for (i = ii; i < ii + 4; ++i) {
//                     a1 = A[i][jj];
//                     a2 = A[i][jj+1];
//                     a3 = A[i][jj+2];
//                     a4 = A[i][jj+3];
//                     a5 = A[i][jj+4];
//                     a6 = A[i][jj+5];
//                     a7 = A[i][jj+6];
//                     a8 = A[i][jj+7];

//                     B[i][jj] = a1;
//                     B[i][jj+1] = a2;
//                     B[i][jj+2] = a3;
//                     B[i][jj+3] = a4;
//                     B[i][jj+4] = a5;
//                     B[i][jj+5] = a6;
//                     B[i][jj+6] = a7;
//                     B[i][jj+7] = a8;
//                 }
//                 for (i = ii; i < ii + 4; ++i) {
//                     for (j = i; j < jj + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j][i];
//                         B[j][i] = a1;
//                     }
//                 }

//                 for (i = ii; i < ii + 4; ++i) {
//                     for (j = i + 4; j < jj + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j - 4][i + 4];
//                         B[j - 4][i + 4] = a1;
//                     }
//                 }

//                 // down 4 lines, = 8
//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     a1 = A[i][jj];
//                     a2 = A[i][jj+1];
//                     a3 = A[i][jj+2];
//                     a4 = A[i][jj+3];
//                     a5 = A[i][jj+4];
//                     a6 = A[i][jj+5];
//                     a7 = A[i][jj+6];
//                     a8 = A[i][jj+7];

//                     B[i][jj] = a1;
//                     B[i][jj+1] = a2;
//                     B[i][jj+2] = a3;
//                     B[i][jj+3] = a4;
//                     B[i][jj+4] = a5;
//                     B[i][jj+5] = a6;
//                     B[i][jj+6] = a7;
//                     B[i][jj+7] = a8;
//                 }

//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     for (j = i; j < jj + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j][i];
//                         B[j][i] = a1;
//                     }
//                 }

//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     for (j = jj + i - ii - 4; j < jj + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j + 4][i - 4];
//                         B[j + 4][i - 4] = a1;
//                     }
//                 }

//                 // exchange, = 8
//                 for (i = 0; i < 4; ++i) {
//                     a5 = B[ii + 4 + i][jj];
//                     a6 = B[ii + 4 + i][jj+1];
//                     a7 = B[ii + 4 + i][jj+2];
//                     a8 = B[ii + 4 + i][jj+3];
//                     a1 = B[ii + i][jj+4];
//                     a2 = B[ii + i][jj+5];
//                     a3 = B[ii + i][jj+6];
//                     a4 = B[ii + i][jj+7];

//                     B[ii + i][jj+4] = a5;
//                     B[ii + i][jj+5] = a6;
//                     B[ii + i][jj+6] = a7;
//                     B[ii + i][jj+7] = a8;
//                     B[ii + 4 + i][jj] = a1;
//                     B[ii + 4 + i][jj+1] = a2;
//                     B[ii + 4 + i][jj+2] = a3;
//                     B[ii + 4 + i][jj+3] = a4;
//                 }
//             }
//         }
//     }

// }


// char transpose_64_64_desc[] = "Transpose big sub matrix, and smaller matrix extension 5";
// void transpose_64_64(int M, int N, int A[N][M], int B[M][N]){
//     int ii, jj, i, j;
//     int a1, a2, a3, a4, a5, a6, a7, a8;

//     // we borrow (1, 57) -- (4, 64) as a higher level cache
//     // diagonal
//     for(ii = 0; ii < N; ii += 8) {
//         for(jj = 0; jj < M; jj += 8) {
//             if (ii == jj && ii != 56) {
//                     // up 4 lines, = 8
//                     for (i = ii; i < ii + 4; ++i) {
//                         a1 = A[i][jj];
//                         a2 = A[i][jj+1];
//                         a3 = A[i][jj+2];
//                         a4 = A[i][jj+3];
//                         a5 = A[i][jj+4];
//                         a6 = A[i][jj+5];
//                         a7 = A[i][jj+6];
//                         a8 = A[i][jj+7];

//                         B[i][jj] = a1;
//                         B[i][jj+1] = a2;
//                         B[i][jj+2] = a3;
//                         B[i][jj+3] = a4;
//                         B[i][jj+4] = a5;
//                         B[i][jj+5] = a6;
//                         B[i][jj+6] = a7;
//                         B[i][jj+7] = a8;

//                         // save to cache, first time = 4 miss, else = 0 miss.
//                         B[i - ii][56] = a5;
//                         B[i - ii][57] = a6;
//                         B[i - ii][58] = a7;
//                         B[i - ii][59] = a8;
//                     }
//                     // up left
//                     for (i = ii; i < ii + 4; ++i) {
//                         for (j = i; j < jj + 4; ++j) {
//                             a1 = B[i][j];
//                             B[i][j] = B[j][i];
//                             B[j][i] = a1;
//                         }
//                     }

//                     // down 4 lines, = 8
//                     for (i = ii + 4; i < ii + 8; ++i) {
//                         a1 = A[i][jj];
//                         a2 = A[i][jj+1];
//                         a3 = A[i][jj+2];
//                         a4 = A[i][jj+3];
//                         a5 = A[i][jj+4];
//                         a6 = A[i][jj+5];
//                         a7 = A[i][jj+6];
//                         a8 = A[i][jj+7];

//                         B[i][jj] = a1;
//                         B[i][jj+1] = a2;
//                         B[i][jj+2] = a3;
//                         B[i][jj+3] = a4;
//                         B[i][jj+4] = a5;
//                         B[i][jj+5] = a6;
//                         B[i][jj+6] = a7;
//                         B[i][jj+7] = a8;

//                         // save to cache, first = 4 miss, else = 0 miss
//                         B[i - ii - 4][60] = a1;
//                         B[i - ii - 4][61] = a2;
//                         B[i - ii - 4][62] = a3;
//                         B[i - ii - 4][63] = a4;
//                     }

//                     // down right
//                     for (i = ii + 4; i < ii + 8; ++i) {
//                         for (j = i; j < jj + 8; ++j) {
//                             a1 = B[i][j];
//                             B[i][j] = B[j][i];
//                             B[j][i] = a1;
//                         }
//                     }

//                     // transpose 2 matrces in cache
//                     for (i = 0; i < 4; ++i) {
//                         for (j = 56 + i; j < 60; ++j) {
//                             a1 = B[i][j];
//                             B[i][j] = B[j - 56][i + 56];
//                             B[j - 56][i + 56] = a1;
//                         }
//                     }

//                     for (i = 0; i < 4; ++i) {
//                         for (j = 60 + i; j < 64; ++j) {
//                             a1 = B[i][j];
//                             B[i][j] = B[j - 60][i + 60];
//                             B[j - 60][i + 60] = a1;
//                         }
//                     }

//                     // update to array B.
//                     // left ---> down left
//                     for (i = ii + 4; i < ii + 8; ++i) {
//                         for (j = jj; j < jj + 4; ++j) {
//                             B[i][j] = B[i - ii - 4][56 + j - jj];
//                         }
//                     }
//                     // right --> up right
//                     for (i = ii; i < ii + 4; ++i) {
//                         for (j = jj + 4; j < jj + 8; ++j) {
//                             B[i][j] = B[i - ii][60 + j - jj - 4];
//                         }
//                     }
//             }
//         }
//     }

//     for(ii = 0; ii < N; ii += 8) {
//         for(jj = 0; jj < M; jj += 8) {
//             if ( ii != jj ) {
//                 // for *up right* and *down left*
//                 // up 4 lines
//                 for (i = 0; i < 4; ++i) {
//                     a1 = A[ii + i][jj];
//                     a2 = A[ii + i][jj+1];
//                     a3 = A[ii + i][jj+2];
//                     a4 = A[ii + i][jj+3];
//                     a5 = A[ii + i][jj+4];
//                     a6 = A[ii + i][jj+5];
//                     a7 = A[ii + i][jj+6];
//                     a8 = A[ii + i][jj+7];

//                     B[jj + i][ii] = a1;
//                     B[jj + i][ii+1] = a2;
//                     B[jj + i][ii+2] = a3;
//                     B[jj + i][ii+3] = a4;
//                     B[jj + i][ii+4] = a5;
//                     B[jj + i][ii+5] = a6;
//                     B[jj + i][ii+6] = a7;
//                     B[jj + i][ii+7] = a8;
//                 }
//                 for (i = jj; i < jj + 4; ++i) {
//                     for (j = ii + i - jj; j < ii + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii][ii + i - jj];
//                         B[jj + j - ii][ii + i - jj] = a1;
//                     }
//                 } // transpose upper left
//                 // down 4 lines
//                 for (i = 0; i < 4; ++i) {
//                     a1 = B[jj + i][ii + 4];
//                     a2 = B[jj + i][ii + 5];
//                     a3 = B[jj + i][ii + 6];
//                     a4 = B[jj + i][ii + 7];

//                     a5 = A[ii + 4 + i][jj];
//                     a6 = A[ii + 4 + i][jj+1];
//                     a7 = A[ii + 4 + i][jj+2];
//                     a8 = A[ii + 4 + i][jj+3]; // = 4 miss

//                     B[jj + i][ii + 4] = a5;
//                     B[jj + i][ii + 5] = a6;
//                     B[jj + i][ii + 6] = a7;
//                     B[jj + i][ii + 7] = a8; // = 0 miss

//                     B[jj + 4 + i][ii] = a1;
//                     B[jj + 4 + i][ii+1] = a2;
//                     B[jj + 4 + i][ii+2] = a3;
//                     B[jj + 4 + i][ii+3] = a4; // = 4 miss

//                     a1 = A[ii + 4 + i][jj + 4];
//                     a2 = A[ii + 4 + i][jj + 5];
//                     a3 = A[ii + 4 + i][jj + 6];
//                     a4 = A[ii + 4 + i][jj + 7]; // = 0 miss

//                     B[jj + 4 + i][ii + 4] = a1;
//                     B[jj + 4 + i][ii + 5] = a2;
//                     B[jj + 4 + i][ii + 6] = a3;
//                     B[jj + 4 + i][ii + 7] = a4; // = 0 miss
//                 }

//                 for (i = jj + 4; i < jj + 8; ++i) {
//                     for (j = ii + i - jj; j < ii + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii][ii + i - jj];
//                         B[jj + j - ii][ii + i - jj] = a1;
//                     }
//                 } // transpose down right, = 0 miss

//                 for (i = jj + 4; i < jj + 8; ++i) {
//                     for (j = ii + i - jj - 4; j < ii + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + 4 + j - ii][ii + i - jj - 4];
//                         B[jj + 4 + j - ii][ii + i - jj - 4] = a1;
//                     }
//                 } // down left, = 0 miss

//                 for (i = jj; i < jj + 4; ++i) {
//                     for (j = ii + 4 + i - jj; j < ii + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[jj + j - ii - 4][ii + 4 + i - jj];
//                         B[jj + j - ii - 4][ii + 4 + i - jj] = a1;
//                     }
//                 } // up right, = 4 miss
//             } else if (ii == jj && ii == 56) {
//                 // up 4 lines, = 8
//                 for (i = ii; i < ii + 4; ++i) {
//                     a1 = A[i][jj];
//                     a2 = A[i][jj+1];
//                     a3 = A[i][jj+2];
//                     a4 = A[i][jj+3];
//                     a5 = A[i][jj+4];
//                     a6 = A[i][jj+5];
//                     a7 = A[i][jj+6];
//                     a8 = A[i][jj+7];

//                     B[i][jj] = a1;
//                     B[i][jj+1] = a2;
//                     B[i][jj+2] = a3;
//                     B[i][jj+3] = a4;
//                     B[i][jj+4] = a5;
//                     B[i][jj+5] = a6;
//                     B[i][jj+6] = a7;
//                     B[i][jj+7] = a8;
//                 }
//                 for (i = ii; i < ii + 4; ++i) {
//                     for (j = i; j < jj + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j][i];
//                         B[j][i] = a1;
//                     }
//                 }

//                 for (i = ii; i < ii + 4; ++i) {
//                     for (j = i + 4; j < jj + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j - 4][i + 4];
//                         B[j - 4][i + 4] = a1;
//                     }
//                 }

//                 // down 4 lines, = 8
//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     a1 = A[i][jj];
//                     a2 = A[i][jj+1];
//                     a3 = A[i][jj+2];
//                     a4 = A[i][jj+3];
//                     a5 = A[i][jj+4];
//                     a6 = A[i][jj+5];
//                     a7 = A[i][jj+6];
//                     a8 = A[i][jj+7];

//                     B[i][jj] = a1;
//                     B[i][jj+1] = a2;
//                     B[i][jj+2] = a3;
//                     B[i][jj+3] = a4;
//                     B[i][jj+4] = a5;
//                     B[i][jj+5] = a6;
//                     B[i][jj+6] = a7;
//                     B[i][jj+7] = a8;
//                 }

//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     for (j = i; j < jj + 8; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j][i];
//                         B[j][i] = a1;
//                     }
//                 }

//                 for (i = ii + 4; i < ii + 8; ++i) {
//                     for (j = jj + i - ii - 4; j < jj + 4; ++j) {
//                         a1 = B[i][j];
//                         B[i][j] = B[j + 4][i - 4];
//                         B[j + 4][i - 4] = a1;
//                     }
//                 }

//                 // exchange, = 8
//                 for (i = 0; i < 4; ++i) {
//                     a5 = B[ii + 4 + i][jj];
//                     a6 = B[ii + 4 + i][jj+1];
//                     a7 = B[ii + 4 + i][jj+2];
//                     a8 = B[ii + 4 + i][jj+3];
//                     a1 = B[ii + i][jj+4];
//                     a2 = B[ii + i][jj+5];
//                     a3 = B[ii + i][jj+6];
//                     a4 = B[ii + i][jj+7];

//                     B[ii + i][jj+4] = a5;
//                     B[ii + i][jj+5] = a6;
//                     B[ii + i][jj+6] = a7;
//                     B[ii + i][jj+7] = a8;
//                     B[ii + 4 + i][jj] = a1;
//                     B[ii + 4 + i][jj+1] = a2;
//                     B[ii + 4 + i][jj+2] = a3;
//                     B[ii + 4 + i][jj+3] = a4;
//                 }
//             }
//         }
//     }

// }

// /*
//  * transpose_submit - This is the solution transpose function that you
//  *     will be graded on for Part B of the assignment. Do not change
//  *     the description string "Transpose submission", as the driver
//  *     searches for that string to identify the transpose function to
//  *     be graded. The REQUIRES and ENSURES from 15-122 are included
//  *     for your convenience. They can be removed if you like.
//  */
// char transpose_submit_desc[] = "Transpose submission";
// void transpose_submit(int M, int N, int A[N][M], int B[M][N])
// {

//     if (M == 32 && N == 32) {
//         transpose_32_32(M, N, A, B);
//     } else if (M == 64 && N == 64) {
//         transpose_64_64(M, N, A, B);
//     } else {
//         transpose_dia_block(M, N, A, B, 16, 4);
//     }

// }

// /*
//  * You can define additional transpose functions below. We've defined
//  * a simple one below to help you get started.
//  */

// /*
//  * trans - A simple baseline transpose function, not optimized for the cache.
//  */
// char trans_desc[] = "Simple row-wise scan transpose";
// void trans(int M, int N, int A[N][M], int B[M][N])
// {
//     int i, j, tmp;


//     for (i = 0; i < N; i++) {
//         for (j = 0; j < M; j++) {
//             tmp = A[i][j];
//             B[j][i] = tmp;
//         }
//     }

// }

// /*
//  * registerFunctions - This function registers your transpose
//  *     functions with the driver.  At runtime, the driver will
//  *     evaluate each of the registered functions and summarize their
//  *     performance. This is a handy way to experiment with different
//  *     transpose strategies.
//  */
// void registerFunctions()
// {
//     /* Register your solution function */
//     registerTransFunction(transpose_submit, transpose_submit_desc);

//     /* Register any additional transpose functions */
//     registerTransFunction(transpose_64_64, transpose_64_64_desc);
// }

// /*
//  * is_transpose - This helper function checks if B is the transpose of
//  *     A. You can check the correctness of your transpose by calling
//  *     it before returning from the transpose function.
//  */
// int is_transpose(int M, int N, int A[N][M], int B[M][N])
// {
//     int i, j;

//     for (i = 0; i < N; i++) {
//         for (j = 0; j < M; ++j) {
//             if (A[i][j] != B[j][i]) {
//                 return 0;
//             }
//         }
//     }
//     return 1;
// }
