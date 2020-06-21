/*
  defs.h: Various helper macros

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

#ifndef _LRPT_DEFS_H
#define _LRPT_DEFS_H

#include <stdint.h>
#include <util.h>
#include <stdlib.h>

#ifdef DEBUG
#  undef DEBUG
#endif

#ifdef ERROR
#  undef ERROR
#endif

#include <stdio.h>
#define DEBUG(fmt, arg...)                     \
  fprintf(                                      \
      stderr,                                   \
      "[i] " fmt,                               \
      ##arg)

#define ERROR(fmt, arg...)                     \
  fprintf(                                      \
      stderr,                                   \
      "[e] " fmt,                               \
      ##arg)

#define TRY_EXCEPT(expr, catch_stmts)        \
  if (!(expr)) {                                \
    catch_stmts;                                \
    goto fail;                                  \
  }

#define TRY(expr)                            \
    TRY_EXCEPT(                              \
        expr,                                   \
        ERROR(                                  \
            "%s:%d: exception in \"%s\"\n",     \
            __FILE__,                           \
            __LINE__,                           \
            STRINGIFY(expr)                     \
            ))

#define ALLOCATE_MANY(dest, n, type)         \
    TRY_EXCEPT(                              \
        dest = calloc(n, sizeof(type)),         \
        ERROR(                                  \
            "%s:%d: failed to allocate %d objects of type %s\n", \
            __FILE__,                           \
            __LINE__,                           \
            n,                                  \
            STRINGIFY(type)                     \
            ))

#define ALLOCATE(dest, type) ALLOCATE_MANY(dest, 1, type)

#define INIT(self, type, ...)          \
  TRY_EXCEPT(                                \
      JOIN(type, _init) (self, __VA_ARGS__),       \
      ERROR(                                    \
          "%s:%d: failed to call constructor of class \"%s\"\n", \
          __FILE__,                             \
          __LINE__,                             \
          STRINGIFY(type)))                     \


#define CONSTRUCT(dest, type, ...)     \
  TRY_EXCEPT(                                \
      dest = JOIN(type, _new) (__VA_ARGS__),       \
      ERROR(                                    \
          "%s:%d: failed to construct object of class %s\n", \
          __FILE__,                             \
          __LINE__,                             \
          STRINGIFY(type)))                     \

enum boolean {
  FALSE,
  TRUE
};

typedef enum boolean BOOL;

typedef uint8_t BIT;

#endif /* _LRPT_DEFS */

