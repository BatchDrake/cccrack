/*
  gf2types.h: GF(2) types

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

#ifndef _GF2_GF2TYPES_H
#define _GF2_GF2TYPES_H

#include <defs.h>
#include <assert.h>

struct gf2_matrix {
  unsigned int rows, cols;
  unsigned int blocks;
  unsigned int rank;
  uint64_t **row_data;
};

typedef struct gf2_matrix gf2_matrix_t;

#define GF2_MATRIX_ROW_BLOCK(n) ((n) >> 6)
#define GF2_MATRIX_ROW_BLOCKS(n) GF2_MATRIX_ROW_BLOCK((n) + 63)
#define GF2_MATRIX_ROW_SHIFT(n) (((n) & 63))

static inline void
gf2_matrix_set(
    gf2_matrix_t *self,
    unsigned int row,
    unsigned int col,
    uint8_t bit)
{
  unsigned int block = GF2_MATRIX_ROW_BLOCK(col);
  unsigned int off = GF2_MATRIX_ROW_SHIFT(col);
  uint64_t mask = ~(1ull << off);

  assert(row < self->rows);
  assert(col < self->cols);

  bit &= 1;

  self->row_data[row][block] =
      (mask & self->row_data[row][block]) | (bit << off);
}

static inline void
gf2_matrix_add(
    gf2_matrix_t *self,
    unsigned int row,
    unsigned int col,
    uint8_t bit)
{
  unsigned int block = GF2_MATRIX_ROW_BLOCK(col);
  unsigned int off = GF2_MATRIX_ROW_SHIFT(col);

  assert(row < self->rows);
  assert(col < self->cols);

  self->row_data[row][block] ^= bit << off;
}

static inline uint8_t
gf2_matrix_get(
    const gf2_matrix_t *self,
    unsigned int row,
    unsigned int col)
{
  unsigned int block = GF2_MATRIX_ROW_BLOCK(col);
  unsigned int off = GF2_MATRIX_ROW_SHIFT(col);

  assert(row < self->rows);
  assert(col < self->cols);

  return (self->row_data[row][block] >> off) & 1;
}

static inline unsigned int
gf2_matrix_get_cols(const gf2_matrix_t *self)
{
  return self->cols;
}

static inline unsigned int
gf2_matrix_get_rows(const gf2_matrix_t *self)
{
  return self->rows;
}

static inline unsigned int
gf2_matrix_get_rank(const gf2_matrix_t *self)
{
  return self->rank;
}

static inline BOOL
gf2_matrix_col_is_null(const gf2_matrix_t *self, unsigned int col)
{
  unsigned int i;
  unsigned int block = GF2_MATRIX_ROW_BLOCK(col);
  unsigned int off = GF2_MATRIX_ROW_SHIFT(col);
  uint64_t mask = 1ull << off;

  assert(col < self->cols);

  for (i = 0; i < self->rows; ++i)
    if ((self->row_data[i][block] & mask) != 0)
      return FALSE;

  return TRUE;
}

static inline BOOL
gf2_matrix_row_is_null(const gf2_matrix_t *self, unsigned int row)
{
  unsigned int i;

  assert(row < self->rows);

  for (i = 0; i < self->blocks; ++i)
    if (self->row_data[row][i] != 0)
      return FALSE;

  return TRUE;
}

void gf2_matrix_destroy(gf2_matrix_t *self);
gf2_matrix_t *gf2_matrix_new(unsigned int rows, unsigned int cols);
gf2_matrix_t *gf2_matrix_eye(unsigned int rows, unsigned int cols);
gf2_matrix_t *gf2_matrix_transpose(const gf2_matrix_t *mat);
void gf2_matrix_swap_rows(gf2_matrix_t *self, unsigned int a, unsigned int b);
void gf2_matrix_add_rows(gf2_matrix_t *self, unsigned int a, unsigned int b);
void gf2_matrix_swap_cols(gf2_matrix_t *self, unsigned int a, unsigned int b);
void gf2_matrix_add_cols(gf2_matrix_t *self, unsigned int a, unsigned int b);
void gf2_matrix_debug(const gf2_matrix_t *self, BOOL transpose);
uint8_t *gf2_matrix_copy_row(const gf2_matrix_t *self, unsigned int row);
uint8_t *gf2_matrix_copy_col(const gf2_matrix_t *self, unsigned int col);
BOOL gf2_matrix_gauss_jordan_rows(gf2_matrix_t *self, gf2_matrix_t **b);
BOOL gf2_matrix_gauss_jordan_cols(gf2_matrix_t *self, gf2_matrix_t **b);

#endif /* _GF2_GF2TYPES_H */
