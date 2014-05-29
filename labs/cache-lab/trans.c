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
    int p, q;
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
        // Loop through upper triangle of A
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

        // Loop through lower triangle of A
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

        for (i = 0; i < 64; i += 8) {
            for (p = i; p < i + 8; p += 4) {

                q = i;
                for (n = 0; n < 4; ++n) {
                    for (m = 0; m < 4; ++m) {
                        if (n == m) continue;
                        B[q+m][p+n] = A[p+n][q+m];
                    }
                    B[q+n][p+n] = A[p+n][q+n];
                }

                q += 4;
                for (n = 3; n >= 0; --n) {
                    for (m = 3; m >= 0; --m) {
                        if (n == m) continue;
                        B[q+m][p+n] = A[p+n][q+m];
                    }
                    B[q+n][p+n] = A[p+n][q+n];
                }

            }
        }
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

