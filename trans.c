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
void transpose_32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64(int M, int N, int A[N][M], int B[M][N]);
void transpose_other(int M, int N, int A[N][M], int B[M][N]);
void transpose_standard(int M, int N, int A[N][M], int B[M][N], int c);

/*
 * transpose_submit - Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
	if(M == 32) {
		transpose_32(M, N, A, B);
	} else if(M == 64) {
		transpose_64(M, N, A, B);
	} else {
		transpose_other(M, N, A, B);
	}
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

char transpose_32_desc[] = "Transpose a 32x32 matrix";
char transpose_64_desc[] = "Transpose a 64x64 matrix";
char transpose_other_desc[] = "Transpose any matrix that isn't 32x32 or 64x64";

void transpose_32(int M, int N, int A[N][M], int B[M][N]) {
	transpose_standard(M, N, A, B, 8);
}

void transpose_64(int M, int N, int A[N][M], int B[M][N]) {
	transpose_standard(M, N, A, B, 4);
}

void transpose_standard(int M, int N, int A[N][M], int B[M][N], int change) {
	// Value and position of the diagonal, and temporary variables
	int val = 0, pos = 0, temp1, temp2;
	int row, col, bRow, bCol;

	// Iterate over the rows of the matrix
	for(col = 0; col < N; col += change) {

		// Iterate over the columns of the matrix
		for(row = 0; row < N; row += change) {

			temp1 = row + change;
 			temp2 = col + change;

			// Iterate over rows of the block
			for(bRow = row; bRow < temp1; bRow++) {

				// Iterate over the columns of the block
				for(bCol = col; bCol < temp2; bCol++) {

					// Transpose values if position is not on diagonal
					if (bRow != bCol) {
						B[bCol][bRow] = A[bRow][bCol];

					// Otherwise, save the value and position to prevent a miss
					} else {
						pos = bRow;
						val = A[bRow][bCol];
					}
				}

				// If on diagonal, move diagonal value on tranpose
				if (row == col) {
					B[pos][pos] = val;
				}
			}	
		}
	}
}

void transpose_other(int M, int N, int A[N][M], int B[M][N]){
	// Value and position of the diagonal, and temporary variables
	int val = 0, pos = 0, temp1, temp2;
	int row, col, bRow, bCol;
	int change = 16;

	// Iterate over the rows of the matrix
	for(col = 0; col < N; col += change) {

		// Iterate over the columns of the matrix
		for(row = 0; row < N; row += change) {

			temp1 = row + change;
 			temp2 = col + change;

			// Iterate over rows of the block
			for(bRow = row; (bRow < temp1) && (bRow < N); bRow++) {

				// Iterate over the columns of the block
				for(bCol = col; (bCol < temp2) && (bCol < M); bCol++) {

					// Transpose values if position is not on diagonal
					if (bRow != bCol) {
						B[bCol][bRow] = A[bRow][bCol];

					// Otherwise, save the value and position to prevent a miss
					} else {
						pos = bRow;
						val = A[bRow][bCol];
					}
				}

				// If on diagonal, move diagonal value on tranpose
				if (row == col) {
					B[pos][pos] = val;
				}
			}	
		}
	}
}


/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
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
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  registerTransFunction(trans, trans_desc);

  /* Register custom transpose functions */
  registerTransFunction(transpose_32, transpose_32_desc);
  registerTransFunction(transpose_64, transpose_64_desc);
  registerTransFunction(transpose_other, transpose_other_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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
