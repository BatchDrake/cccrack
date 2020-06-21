/*
  symtag.c: Symbol tagger

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

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "symtag.h"

void
tagging_finalize(struct tagging *self)
{
  if (self->dict != NULL)
    free(self->dict);

  memset(self, 0, sizeof(struct tagging));
}

BOOL
tagging_copy(struct tagging *dest, const struct tagging *orig)
{
  struct tagging tagging = tagging_INITIALIZER;
  BOOL ok = FALSE;

  ALLOCATE_MANY(tagging.dict, orig->dict_len, uint8_t);
  memcpy(tagging.dict, orig->dict, orig->dict_len);

  tagging.dict_len = orig->dict_len;
  tagging.tagging_id = orig->tagging_id;
  tagging.bps = orig->bps;
  tagging.is_gray = orig->is_gray;

  tagging_finalize(dest);
  memcpy(dest, &tagging, sizeof(struct tagging));

  ok = TRUE;

fail:
  if (!ok)
    tagging_finalize(&tagging);

  return ok;
}

void
tagging_debug(const struct tagging *tagging)
{
  unsigned int width_bits, height_bits;
  uint64_t width, height;
  uint64_t i, j, k;
  unsigned int c;
  unsigned int left_width;
  char format[20];

  width_bits = tagging->bps / 2 + (tagging->bps & 1);
  height_bits = tagging->bps - width_bits;

  width  = 1ull << width_bits;
  height = 1ull << height_bits;

  snprintf(format, sizeof(format), " %%%dd", tagging->bps);

  left_width = (height_bits >> 2) + !!(height_bits & 3) + 1;

  printf("%*c", 5 + left_width, '+');
  for (i = 0; i < width; ++i)
    printf((const char *) format, i);
  putchar(10);

  printf("  ");

  c = left_width + 2;
  while (c-- > 0)
    putchar('-');
  putchar('+');

  c = (1 + tagging->bps) * width + 3;
  while (c-- > 0)
    putchar('-');

  putchar(10);

  snprintf(format, sizeof(format), " %%%dd ", left_width);

  for (j = 0; j < height; ++j) {
    printf("  ");
    printf((const char *) format, j * width);
    putchar('|');
    for (i = 0; i < width; ++i) {
      putchar(' ');
      for (k = 0; k < tagging->bps; ++k)
        putchar(
            '0' + ((tagging->dict[i + j * width] >> (tagging->bps - k - 1))
            & 1));
    }
    putchar(10);
  }

  printf("  ");

  c = left_width + 2;
   while (c-- > 0)
     putchar('-');
   putchar('+');

   c = (1 + tagging->bps) * width + 3;
   while (c-- > 0)
     putchar('-');

   putchar(10);

}

static inline unsigned int
popcount64(uint64_t b)
{
  b = (b & 0x5555555555555555ull) + (b >> 1 & 0x5555555555555555ull);
  b = (b & 0x3333333333333333ull) + (b >> 2 & 0x3333333333333333ull);
  b = b + (b >> 4) & 0x0F0F0F0F0F0F0F0Full;
  b = b + (b >> 8);
  b = b + (b >> 16);
  b = b + (b >> 32) & 0x0000007F;

  return (unsigned int) b;
}

void
tagging_compute_properties(struct tagging *self)
{
  unsigned int i, j;
  BOOL is_gray = TRUE;

  for (i = 0; is_gray && i < self->dict_len; ++i)
    if (i > 0)
      is_gray = popcount64(self->dict[i] ^ self->dict[i - 1]) == 1;

  self->is_gray = is_gray;
}

void
symtag_destroy(symtag_t *self)
{
  if (self->sym_data != NULL && self->sym_data != (void *) -1)
    munmap(self->sym_data, self->sym_len);

  if (self->tagging.dict != NULL)
    free(self->tagging.dict);

  if (self->bit_data != NULL)
    free(self->bit_data);

  free(self);
}

static symtag_t *
symtag_new(
    uint8_t *sym_data,
    size_t len,
    unsigned int bps,
    symtag_tagging_cb_t cb,
    void *private)
{
  symtag_t *self = NULL;

  ALLOCATE(self, symtag_t);

  self->sym_data = sym_data;
  self->sym_len  = len;
  self->bit_len  = len * bps;
  self->tagging.bps = bps;
  self->tagging.mask = (1 << bps) - 1;
  self->tagging.dict_len = 1 << bps;

  ALLOCATE_MANY(
      self->tagging.dict,
      self->tagging.dict_len,
      uint8_t);

  ALLOCATE_MANY(
      self->bit_data,
      self->bit_len,
      uint8_t);

  self->private = private;
  self->on_tagging = cb;

  return self;

fail:
  if (self != NULL)
    symtag_destroy(self);

  return NULL;
}

symtag_t *
symtag_new_from_file(
    const char *file,
    unsigned int bps,
    symtag_tagging_cb_t cb,
    void *private)
{
  symtag_t *self = NULL;
  uint8_t *sym_data = (uint8_t *) -1;
  unsigned int sym_len;
  unsigned int symcnt = 2;
  unsigned int i;
  unsigned int valid = 0;
  unsigned int chop_start = 0;
  unsigned int page_size;

  struct stat sbuf;
  int fd;

  if (stat(file, &sbuf) == -1) {
    ERROR("Cannot stat `%s': %s\n", file, strerror(errno));
    goto fail;
  }

  if ((fd = open(file, O_RDONLY)) == -1) {
    ERROR("Cannot open `%s': %s\n", file, strerror(errno));
    goto fail;
  }

  sym_len = sbuf.st_size;

  TRY(
      (sym_data = mmap(NULL, sym_len, PROT_READ, MAP_PRIVATE, fd, 0)) !=
          (uint8_t *) -1);


  close(fd);

  if (bps == 0) {
    bps = 1;
    for (i = 0; i < sym_len; ++i) {
      if (sym_data[i] < '0' || sym_data[i] >= '0' + 64)
        break;

      while ((sym_data[i] - '0') >= symcnt) {
        ++bps;
        symcnt <<= 1;
      }

      ++valid;
    }
  }

  if (valid == 0) {
    ERROR("This is not a valid symbol capture file\n");
    goto fail;
  } else if (valid < sym_len) {
    page_size = getpagesize();

    chop_start = ((valid + page_size - 1) / page_size) * page_size;

    if (chop_start < sym_len)
      munmap(sym_data + chop_start, sym_len - chop_start);

    sym_len = valid;
  }

  self = symtag_new(sym_data, sym_len, bps, cb, private);
  sym_data = (uint8_t *) -1;

  TRY(self != NULL);

  return self;

fail:
  if (sym_data != (uint8_t *) -1)
    munmap(sym_data, sym_len);

  if (self != NULL)
    symtag_destroy(self);

  return NULL;
}

static BOOL
symtag_tag_internal(symtag_t *self, unsigned int sym)
{
  unsigned int i, j;
  uint64_t bit;
  uint8_t *bit_data;

  /* Selecting bit */
  if (sym < self->tagging.dict_len) {
    for (i = 0; i < self->tagging.dict_len; ++i) {
      bit = 1ull << i;
      if ((self->sel_mask & bit) == 0) {
        self->sel_mask |= bit;
        self->tagging.dict[sym] = i;

        if (!symtag_tag_internal(self, sym + 1))
          goto fail;

        self->sel_mask &= ~bit;
      }
    }
  } else {
    bit_data = self->bit_data;
    for (i = 0; i < self->sym_len; ++i) {
      j = self->tagging.bps;
      do
        *bit_data++ =
            (self->tagging.dict[
                 (self->sym_data[i] - '0') & self->tagging.mask] >> --j) & 1;
      while (j != 0);
    }

    tagging_compute_properties(&self->tagging);

    /* All bits selected, call handler */
    TRY((self->on_tagging) (
        self->private,
        &self->tagging,
        self->bit_data,
        self->bit_len));

    ++self->tagging.tagging_id;
  }

  return TRUE;

fail:
  return FALSE;
}

BOOL
symtag_tag(symtag_t *self)
{
  self->tagging.tagging_id = 0;
  self->sel_mask = 0;

  return symtag_tag_internal(self, 0);
}
