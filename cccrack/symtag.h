/*
  symtag.h: Symbol tagger

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

#ifndef _SYMTAG_H
#define _SYMTAG_H

#include <stdint.h>
#include <defs.h>

struct tagging {
  uint8_t *dict;
  size_t dict_len;

  unsigned int tagging_id;
  unsigned int bps;
  unsigned int mask;

  BOOL is_gray;
};

void tagging_debug(const struct tagging *tagging);

#define tagging_INITIALIZER    \
{                               \
  NULL, /* dict */              \
  0, /* dict_len */             \
  0, /* tagging_id */           \
  0, /* bps */                  \
  FALSE, /* is_gray */          \
}

void tagging_finalize(struct tagging *self);
BOOL tagging_copy(struct tagging *dest, const struct tagging *orig);

typedef BOOL (*symtag_tagging_cb_t) (
    void *private,
    const struct tagging *tagging,
    const uint8_t *bits,
    size_t len);

struct symtag {
  uint8_t *sym_data;
  uint8_t *bit_data;
  struct tagging tagging;
  size_t sym_len;
  size_t bit_len;

  uint64_t sel_mask;

  void *private;
  symtag_tagging_cb_t on_tagging;
};

typedef struct symtag symtag_t;

static inline uint64_t
symtag_get_tagging_count(const symtag_t *self)
{
  uint64_t result = 1;
  uint64_t val = self->tagging.dict_len;

  while (val > 1)
    result *= val--;

  return result;
}

void symtag_destroy(symtag_t *self);

symtag_t *symtag_new_from_file(
    const char *file,
    unsigned int bps,
    symtag_tagging_cb_t cb,
    void *private);

BOOL symtag_tag(symtag_t *self);

#endif /* _SYMTAG_H */
