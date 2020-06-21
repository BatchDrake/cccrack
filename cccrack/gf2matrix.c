/*
  gf2types.h: GF(2) matrix operations

  Copyright (C) 2019 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include "gf2types.h"

void
gf2_matrix_destroy(gf2_matrix_t *self)
{
  unsigned int i;

  if (self->row_data != NULL) {
    for (i = 0; i < self->rows; ++i)
      if (self->row_data[i] != NULL)
        free(self->row_data[i]);

    free(self->row_data);
  }

  free(self);
}

gf2_matrix_t *
gf2_matrix_new(unsigned int rows, unsigned int cols)
{
  gf2_matrix_t *self = NULL;
  unsigned int i;

  ALLOCATE(self, gf2_matrix_t);
  ALLOCATE_MANY(self->row_data, rows, uint64_t *);

  self->blocks = GF2_MATRIX_ROW_BLOCKS(cols);
  self->rows   = rows;
  self->cols   = cols;

  for (i = 0; i < rows; ++i)
    ALLOCATE_MANY(self->row_data[i], self->blocks, uint64_t);

  return self;

fail:
  if (self != NULL)
    gf2_matrix_destroy(self);

  return NULL;
}

gf2_matrix_t *
gf2_matrix_eye(unsigned int rows, unsigned int cols)
{
  gf2_matrix_t *self = NULL;
  unsigned int min, i;

  CONSTRUCT(self, gf2_matrix, rows, cols);

  min = MIN(rows, cols);

  for (i = 0; i < min; ++i)
    gf2_matrix_set(self, i, i, 1);

  return self;

fail:
  if (self != NULL)
    gf2_matrix_destroy(self);

  return NULL;
}

gf2_matrix_t *
gf2_matrix_transpose(const gf2_matrix_t *mat)
{
  gf2_matrix_t *self = NULL;
  unsigned int i, j;

  CONSTRUCT(self, gf2_matrix, mat->cols, mat->rows);

  for (j = 0; j < mat->rows; ++j)
    for (i = 0; i < mat->cols; ++i)
      gf2_matrix_set(self, i, j, gf2_matrix_get(mat, j, i));

  return self;

fail:
  if (self != NULL)
    gf2_matrix_destroy(self);

  return NULL;
}

void
gf2_matrix_swap_rows(gf2_matrix_t *self, unsigned int a, unsigned int b)
{
  uint64_t *prev;

  assert(a < self->rows);
  assert(b < self->rows);

  prev = self->row_data[a];
  self->row_data[a] = self->row_data[b];
  self->row_data[b] = prev;
}

void
gf2_matrix_add_rows(gf2_matrix_t *self, unsigned int a, unsigned int b)
{
  unsigned int i;

  for (i = 0; i < self->blocks; ++i)
    self->row_data[a][i] ^= self->row_data[b][i];
}

void
gf2_matrix_swap_cols(gf2_matrix_t *self, unsigned int a, unsigned int b)
{
  uint8_t prev;
  unsigned int i;

  assert(a < self->cols);
  assert(b < self->cols);

  for (i = 0; i < self->rows; ++i) {
    prev = gf2_matrix_get(self, i, a);
    gf2_matrix_set(self, i, a, gf2_matrix_get(self, i, b));
    gf2_matrix_set(self, i, b, prev);
  }
}

void
gf2_matrix_add_cols(gf2_matrix_t *self, unsigned int a, unsigned int b)
{
  uint8_t prev;
  unsigned int i;

  assert(a < self->cols);
  assert(b < self->cols);

  for (i = 0; i < self->rows; ++i)
    gf2_matrix_add(self, i, a, gf2_matrix_get(self, i, b));
}

void
gf2_matrix_debug(const gf2_matrix_t *self, BOOL transpose)
{
  unsigned int i, j;

  if (transpose) {
    for (i = 0; i < self->cols; ++i) {
      printf(" | ");
      for (j = 0; j < self->rows; ++j)
        printf("%d ", gf2_matrix_get(self, j, i));
      printf("|\n");
    }
  } else {
    for (j = 0; j < self->rows; ++j) {
      printf(" | ");
      for (i = 0; i < self->cols; ++i)
        printf("%d ", gf2_matrix_get(self, j, i));
      printf("|\n");
    }
  }



  putchar(10);
}

BOOL
gf2_matrix_gauss_jordan_cols(gf2_matrix_t *self, gf2_matrix_t **b)
{
  gf2_matrix_t *b_m = NULL;
  unsigned int i, j, rank;
  uint8_t pivot;

  assert(self->cols <= self->rows);

  if (b != NULL)
    TRY(b_m = gf2_matrix_eye(self->cols, self->cols));

  rank = 0;

  for (i = 0; i < self->cols; ++i) {
    /* Ensure this pivot is non null */
    if ((pivot = gf2_matrix_get(self, i, i)) == 0) {
      for (j = i + 1; j < self->rows; ++j) {
        if (gf2_matrix_get(self, j, i) != 0) {
          gf2_matrix_swap_rows(self, j, i);
          pivot = 1;
          break;
        }
      }
    }

    if (pivot != 0) {
      for (j = i + 1; j < self->cols; ++j) {
        if (gf2_matrix_get(self, i, j) != 0) {
          gf2_matrix_add_cols(self, j, i);
          if (b != NULL)
            gf2_matrix_add_rows(b_m, j, i);
        }
      }

      ++rank;
    }
  }

  self->rank= rank;

  if (b != NULL)
    *b = b_m;

  return TRUE;

fail:
  if (b_m != NULL)
    gf2_matrix_destroy(b_m);

  return FALSE;
}

uint8_t *
gf2_matrix_copy_row(const gf2_matrix_t *self, unsigned int row)
{
  uint8_t *rowdata;
  unsigned int i;

  ALLOCATE_MANY(rowdata, self->cols, uint8_t);

  for (i = 0; i < self->cols; ++i)
    rowdata[i] = gf2_matrix_get(self, row, i);

  return rowdata;

fail:
  return NULL;
}

uint8_t *
gf2_matrix_copy_col(const gf2_matrix_t *self, unsigned int col)
{
  uint8_t *coldata;
  unsigned int i;

  ALLOCATE_MANY(coldata, self->rows, uint8_t);

  for (i = 0; i < self->rows; ++i)
    coldata[i] = gf2_matrix_get(self, i, col);

  return coldata;

fail:
  return NULL;
}

BOOL
gf2_matrix_gauss_jordan_rows(gf2_matrix_t *self, gf2_matrix_t **b)
{
  gf2_matrix_t *b_m = NULL;
  unsigned int i, j, rank;
  uint8_t pivot;

  assert(self->rows <= self->cols);

  if (b != NULL)
    TRY(b_m = gf2_matrix_eye(self->rows, self->rows));

  rank = 0;

  for (i = 0; i < self->rows; ++i) {
    /* Ensure this pivot is non null */
    if ((pivot = gf2_matrix_get(self, i, i)) == 0) {
      for (j = i + 1; j < self->cols; ++j) {
        if (gf2_matrix_get(self, i, j) != 0) {
          gf2_matrix_swap_cols(self, i, j);
          pivot = 1;
          break;
        }
      }
    }

    if (pivot != 0) {
      for (j = i + 1; j < self->rows; ++j) {
        if (gf2_matrix_get(self, j, i) != 0) {
          gf2_matrix_add_rows(self, j, i);
          if (b != NULL)
            gf2_matrix_add_rows(b_m, j, i);
        }
      }

      ++rank;
    }
  }

  self->rank= rank;

  if (b != NULL)
    *b = b_m;

  return TRUE;

fail:
  if (b_m != NULL)
    gf2_matrix_destroy(b_m);

  return FALSE;
}
