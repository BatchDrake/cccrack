/*
  cccrack.c: Crack convolutional codes

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

#include "cccrack.h"

#include <string.h>
#include <math.h>

static void
cccrack_rankdef_destroy(cccrack_rankdef_t *self)
{
  tagging_finalize(&self->tagging);

  unsigned int i;

  for (i = 0; i < self->h_count; ++i)
    if (self->h_list[i] != NULL)
      free(self->h_list[i]);

  if (self->h_list != NULL)
    free(self->h_list);

  for (i = 0; i < self->h_poly_count; ++i)
    if (self->h_poly_list[i] != NULL)
      free(self->h_poly_list[i]);

  if (self->h_poly_list != NULL)
    free(self->h_poly_list);

  for (i = 0; i < self->g_count; ++i)
    if (self->g_list[i] != NULL)
      free(self->g_list[i]);

  if (self->g_list != NULL)
    free(self->g_list);

  for (i = 0; i < self->g_poly_count; ++i)
    if (self->g_poly_list[i] != NULL)
      free(self->g_poly_list[i]);

  if (self->g_poly_list != NULL)
    free(self->g_poly_list);

  free(self);
}

void
cccrack_rankdef_debug(const cccrack_rankdef_t *self)
{
  unsigned int i, j, k;

  printf("RANK DEFFICIENCY INFO (tagging ID: %d)\n", self->tagging.tagging_id);
  tagging_debug(&self->tagging);
  printf(
      "  Estimated code parameters: %d/%d (K=%d)\n",
      self->k,
      self->n,
      self->K);
  printf(
      "  Tagging is Gray: \033[1;3%dm%s\033[0m\n",
      self->tagging.is_gray ? 2 : 1,
      self->tagging.is_gray ? "YES" : "NO");

  printf("  Number of parity outputs: %d\n", self->h_count);

  for (i = 0; i < self->h_poly_count; ++i) {
    printf("    H[%d] = ", i + 1);
    for (j = 0; j < self->n; ++j)
      printf("%3lld ", self->h_poly_list[i][j]);
    printf(" | OCT:");
    for (j = 0; j < self->n; ++j)
      printf("%3llo ", self->h_poly_list[i][j]);

    putchar(10);
  }

  putchar(10);

  printf("  Number of generator polynomials: %d\n", self->g_count);

  printf("\033[1m");

  for (i = 0; i < self->g_poly_count; ++i) {
    printf("    G[%d] = ", i + 1);
    for (j = 0; j < self->n; ++j)
      printf("%3lld ", self->g_poly_list[i][j]);
    printf(" | OCT:");
    for (j = 0; j < self->n; ++j)
      printf("%3llo ", self->g_poly_list[i][j]);
    printf(" | BIN:");
    for (j = 0; j < self->n; ++j) {
      for (k = 0; k < self->K; ++k)
        printf("%d", self->g_list[i][j +  self->n * k]);
      putchar(32);
    }
    putchar(10);
  }

  printf("\033[0m");

  putchar(10);
}

static BOOL
cccrack_rankdef_populate(
    cccrack_rankdef_t *self,
    const gf2_matrix_t *R,
    const gf2_matrix_t *B)

{
  unsigned int i, l;
  uint8_t *row = NULL;

  l = gf2_matrix_get_cols(R);

  self->n_a = l;

  for (i = 0; i < l; ++i)
    if (gf2_matrix_col_is_null(R, i)) {
      TRY(row = gf2_matrix_copy_row(B, i));
      TRY(PTR_LIST_APPEND_CHECK(self->h, row) != -1);
      row = NULL;
    }

  return TRUE;

fail:
  if (row != NULL)
    free(row);

  return FALSE;
}

static cccrack_rankdef_t *
cccrack_rankdef_new(const struct tagging *tagging)
{
  cccrack_rankdef_t *self = NULL;

  ALLOCATE(self, cccrack_rankdef_t);

  TRY(tagging_copy(&self->tagging, tagging));

  return self;

fail:
  if (self != NULL)
    cccrack_rankdef_destroy(self);

  return NULL;
}

cccrack_rankdef_t *
cccrack_rankdef_dup(const cccrack_rankdef_t *orig)
{
  cccrack_rankdef_t *self = NULL;
  unsigned int i;
  uint8_t *bits = NULL;
  uint64_t *polys = NULL;

  CONSTRUCT(self, cccrack_rankdef, &orig->tagging);

  self->n_a = orig->n_a;
  self->n   = orig->n;
  self->k   = orig->k;
  self->K   = orig->K;
  self->muT = orig->muT;

  self->likely = orig->likely;

  for (i = 0; i < orig->h_count; ++i) {
    ALLOCATE_MANY(bits, orig->n_a, uint8_t);
    memcpy(bits, orig->h_list[i], orig->n_a);
    TRY(PTR_LIST_APPEND_CHECK(self->h, bits) != -1);
    bits = NULL;
  }

  for (i = 0; i < orig->g_count; ++i) {
    ALLOCATE_MANY(bits, orig->n * orig->K, uint8_t);
    memcpy(bits, orig->g_list[i], orig->n * orig->K);
    TRY(PTR_LIST_APPEND_CHECK(self->g, bits) != -1);
    bits = NULL;
  }

  for (i = 0; i < orig->h_poly_count; ++i) {
    ALLOCATE_MANY(polys, orig->n, uint64_t);
    memcpy(polys, orig->h_poly_list[i], orig->n);
    TRY(PTR_LIST_APPEND_CHECK(self->h_poly, polys) != -1);
    polys = NULL;
  }

  for (i = 0; i < orig->g_poly_count; ++i) {
    ALLOCATE_MANY(polys, orig->n, uint64_t);
    memcpy(polys, orig->g_poly_list[i], orig->n);
    TRY(PTR_LIST_APPEND_CHECK(self->g_poly, polys) != -1);
    polys = NULL;
  }

  return self;

fail:
  if (polys != NULL)
    free(polys);

  if (bits != NULL)
    free(bits);

  if (self != NULL)
    cccrack_rankdef_destroy(self);

  return NULL;
}

static cccrack_rankdef_t *
cccrack_rankdef_from_matrices(
    const struct tagging *tagging,
    const gf2_matrix_t *R,
    const gf2_matrix_t *B)
{
  cccrack_rankdef_t *self = NULL;

  CONSTRUCT(self, cccrack_rankdef, tagging);

  TRY(cccrack_rankdef_populate(self, R, B));

  return self;

fail:
  if (self != NULL)
    cccrack_rankdef_destroy(self);

  return NULL;
}

static void
cccrack_rankdef_set_second_defficiency(
    cccrack_rankdef_t *self,
    unsigned int l)
{
  self->n = l - self->n_a;
}

static BOOL
cccrack_rankdef_extract_duals(cccrack_rankdef_t *self)
{
  unsigned int i, j, k;
  uint64_t poly;
  uint64_t *list = NULL;

  for (i = 0; i < self->h_count; ++i) {
    ALLOCATE_MANY(list, self->n, uint64_t);

    for (j = 0; j < self->n; ++j) {
      poly = 0;
      for (k = 0; k <= self->muT; ++k)
        poly |= self->h_list[i][self->n * k + j] << k;

      list[j] = poly;
    }

    TRY(PTR_LIST_APPEND_CHECK(self->h_poly, list) != -1);
    list = NULL;
  }

  return TRUE;

fail:
  if (list != NULL)
    free(list);

  return FALSE;
}

static BOOL
cccrack_rankdef_compute_generators(cccrack_rankdef_t *self)
{
  int p;
  unsigned int i, j, k;
  unsigned int rowcnt = 0;
  unsigned int equations;
  unsigned int unknowns, rows;
  unsigned int count = 0;
  unsigned int d = 0;
  uint64_t poly;
  uint64_t *list = NULL;
  uint8_t *row = NULL;

  gf2_matrix_t *A = NULL;
  gf2_matrix_t *B = NULL;

  BOOL ok = FALSE;

  unknowns  = self->n * self->K;
  equations = self->K + self->muT; /* TODO: Add more equations */

  rows = MAX(unknowns, self->h_count * equations);

  CONSTRUCT(A, gf2_matrix, rows, unknowns);

  /* This system seems overdetermined but is not */

  for (d = 0; d < self->h_count; ++d) {
    for (i = 0; i < equations; ++i) {
      for (j = 0; j < unknowns; ++j) {
        /*
         * - k is not mapped: We get multiple solutions for g, each of them
         *   is a polynomial for one of the inputs.
         *
         * - p = (i - equations / 2) * n + j
         */

        /* Compute shift of this vector */
        p = (i - equations / 2) * self->n + j;

        if (p >= 0 && p < unknowns)
          gf2_matrix_set(A, rowcnt, p, self->h_list[d][j]);
      }

      ++rowcnt;
    }
  }

  TRY(gf2_matrix_gauss_jordan_cols(A, &B));

  for (i = 0; i < unknowns; ++i)
    if (gf2_matrix_col_is_null(A, i)) {
      TRY(row = gf2_matrix_copy_row(B, i));
      ALLOCATE_MANY(list, self->n, uint64_t);

      for (j = 0; j < self->n; ++j) {
        poly = 0;
        for (k = 0; k <= self->muT; ++k)
          poly |= row[self->n * k + j] << (self->K - k - 1);

        list[j] = poly;
      }

      TRY(PTR_LIST_APPEND_CHECK(self->g_poly, list) != -1);
      list = NULL;

      TRY(PTR_LIST_APPEND_CHECK(self->g, row) != -1);
      row = NULL;
    }

  self->likely = self->g_poly_count == self->k;

  ok = TRUE;

fail:
  if (row != NULL)
    free(row);

  if (list != NULL)
    free(list);

  if (A != NULL)
    gf2_matrix_destroy(A);

  if (B != NULL)
    gf2_matrix_destroy(B);

  return ok;
}

void
cccrack_destroy(cccrack_t *self)
{
  unsigned int i;

  if (self->symtag != NULL)
    symtag_destroy(self->symtag);

  for (i = 0; i < self->rankdef_count; ++i)
    if (self->rankdef_list[i] != NULL)
      cccrack_rankdef_destroy(self->rankdef_list[i]);

  if (self->rankdef_list != NULL)
    free(self->rankdef_list);

  free(self);
}

static BOOL
cccrack_push_rankdef(cccrack_t *self, cccrack_rankdef_t *def)
{
  TRY(PTR_LIST_APPEND_CHECK(self->rankdef, def) != -1);

  return TRUE;

fail:
  return FALSE;
}

static BOOL
cccrack_eval_candidate(
    cccrack_t *self,
    const cccrack_rankdef_t *template)
{
  cccrack_rankdef_t *dup = NULL;
  BOOL should_save;
  BOOL ok = FALSE;

  TRY(dup = cccrack_rankdef_dup(template));
  TRY(cccrack_rankdef_extract_duals(dup));
  TRY(cccrack_rankdef_compute_generators(dup));

  should_save =
      (self->params.all || cccrack_rankdef_is_likely(dup));

  if (should_save) {
    TRY(cccrack_push_rankdef(self, dup));
  } else {
    cccrack_rankdef_destroy(dup);
  }

  dup = NULL;

  ok = TRUE;

fail:
  if (dup != NULL)
    cccrack_rankdef_destroy(dup);

  return ok;
}

static BOOL
cccrack_enumerate_configs(cccrack_t *self, cccrack_rankdef_t *template)
{
  unsigned int k, n, n_a, n_k;
  unsigned int z;
  BOOL ok = FALSE;

  /*
   * Now we have n_a and n. We are ready to enumerate all
   * possible values for k and muT
   */

  n   = template->n;
  n_a = template->n_a;

  for (k = 1; k < n; ++k) {
    n_k = n - k;
    for (z = 1; z <= n_k; ++z) {
      template->muT = template->n_a  - (template->n_a * k) / n - z;
      template->k = k;
      template->K = template->muT / template->k + 1;

      /* This is something interesting. Study case when K = 1 */
      if (template->K > 1)
        TRY(cccrack_eval_candidate(self, template));
    }
  }

  ok = TRUE;

fail:
  return ok;
}

static BOOL
cccrack_helper_save_tagging(
    const char *path,
    const uint8_t *bits,
    size_t len)
{
  FILE *fp = NULL;
  BOOL ok = FALSE;

  TRY(fp = fopen(path, "wb"));

  while (len-- > 0)
    fputc('0' + *bits++, fp);

  ok = TRUE;

fail:
  if (fp != NULL)
    fclose(fp);

  return ok;
}

static BOOL
cccrack_on_tagging(
    void *private,
    const struct tagging *tagging,
    const uint8_t *bits,
    size_t len)
{
  unsigned int width;
  unsigned int height;
  unsigned int l, i, j, p;
  gf2_matrix_t *R = NULL;
  gf2_matrix_t *B = NULL;
  cccrack_rankdef_t *rankdef = NULL;
  cccrack_t *self = (cccrack_t *) private;
  BOOL have_n;
  BOOL have_kays;
  BOOL ok = FALSE;

  if (self->params.tagging != -1 && self->params.tagging != tagging->tagging_id)
    return TRUE;

  if (self->params.dumpfile != NULL)
    TRY(cccrack_helper_save_tagging(self->params.dumpfile, bits, len));

  if (!self->params.no_gray && !tagging->is_gray)
    return TRUE;

  width = floor(sqrt(len));

  if (width > CCCRACK_MAX_WIDTH)
    width = CCCRACK_MAX_WIDTH;

  /* TODO: Repeat for several regions */
  for (l = 2; !ok && l < width; ++l) {
    height = len / l;
    if (height > width * CCCRACK_MAX_REL_HEIGHT)
      height = width * CCCRACK_MAX_REL_HEIGHT;

    /* Construct received code matrix */
    CONSTRUCT(R, gf2_matrix, height, l);

    p = 0;

    for (i = 0; i < height; ++i)
      for (j = 0; j < l; ++j)
        gf2_matrix_set(R, i, j, bits[p++]);

    TRY(gf2_matrix_gauss_jordan_cols(R, &B));

    if (gf2_matrix_get_rank(R) < j) {
      have_n = FALSE;

      if (rankdef == NULL) {
        TRY(rankdef = cccrack_rankdef_from_matrices(tagging, R, B));

        if (self->params.n > 0) {
          rankdef->n = self->params.n;
          have_n = TRUE;
        }
      } else {
        /* No n found. This is the second iter */
        cccrack_rankdef_set_second_defficiency(rankdef, l);
        have_n = TRUE;
      }

      if (have_n) {
        /* We have guessed n now. How about k and K? */
        have_kays = self->params.k > 0 && self->params.K > 0;

        if (have_kays) {
          rankdef->k = self->params.k;
          rankdef->K = self->params.K;
          rankdef->muT = rankdef->k * (rankdef->K - 1);

          TRY(cccrack_eval_candidate(self, rankdef));
        } else {
          /* No k, K provided. Test them all */
          TRY(cccrack_enumerate_configs(self, rankdef));
        }

        ok = TRUE;
      }
    }

    gf2_matrix_destroy(R);
    R = NULL;

    gf2_matrix_destroy(B);
    B = NULL;
  }

  ok = TRUE;

fail:
  if (rankdef != NULL)
    cccrack_rankdef_destroy(rankdef);

  if (B != NULL)
    gf2_matrix_destroy(B);

  if (R != NULL)
    gf2_matrix_destroy(R);

  return ok;
}

BOOL
cccrack_run(cccrack_t *self)
{
  BOOL ok = FALSE;

  TRY(symtag_tag(self->symtag));

  ok = TRUE;

fail:
  return ok;
}

cccrack_t *
cccrack_new(const char *path, const struct cccrack_params *params)
{
  const struct cccrack_params defparams = cccrack_params_INITIALIZER;
  cccrack_t *self = NULL;

  if (params == NULL)
    params = &defparams;

  ALLOCATE(self, cccrack_t);

  self->params = *params;

  TRY(self->symtag = symtag_new_from_file(
      path,
      self->params.bps,
      cccrack_on_tagging,
      self));

  return self;

fail:
  if (self != NULL)
    cccrack_destroy(self);

  return NULL;
}
