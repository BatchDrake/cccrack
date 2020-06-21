/*
 * cccrack.h: headers, prototypes and declarations for cccrack
 * Creation date: Mon Feb 25 15:01:52 2019
 */

#ifndef _MAIN_INCLUDE_H
#define _MAIN_INCLUDE_H

#include <config.h> /* General compile-time configuration parameters */
#include <util.h> /* From util: Common utility library */
#include "gf2types.h"

#include "symtag.h"

#define CCCRACK_MAX_WIDTH      100
#define CCCRACK_MAX_REL_HEIGHT 5

struct cccrack_rankdef {
  struct tagging tagging;
  PTR_LIST(uint8_t, h); /* Dual elements. There should be n-k of these */
  PTR_LIST(uint64_t, h_poly); /* Polynomials */

  PTR_LIST(uint8_t, g); /* Generator elements, one for each k */
  PTR_LIST(uint64_t, g_poly); /* Polynomial form */

  unsigned int n_a;
  unsigned int n;
  unsigned int k;
  unsigned int K;
  unsigned int muT;

  BOOL likely;
};

typedef struct cccrack_rankdef cccrack_rankdef_t;

void cccrack_rankdef_debug(const cccrack_rankdef_t *self);

static inline BOOL
cccrack_rankdef_is_likely(const cccrack_rankdef_t *self)
{
  return self->likely;
}

static inline BOOL
cccrack_rankdef_is_gray(const cccrack_rankdef_t *self)
{
  return self->tagging.is_gray;
}

struct cccrack_params {
  unsigned int bps;
  int tagging;
  const char *dumpfile;
  unsigned int k, n, K;
  BOOL no_gray;
  BOOL all;
};

#define cccrack_params_INITIALIZER      \
{                                        \
  0, /* bps */                           \
  -1, /* tagging */                      \
  NULL, /* dumpfile */                   \
  0, 0, 0, /* k, n, K */                 \
  FALSE, /* no_gray */                   \
  FALSE, /* all */                       \
}

struct cccrack {
  struct cccrack_params params;
  symtag_t *symtag;

  PTR_LIST(cccrack_rankdef_t, rankdef); /* Equals to the number of taggins */
};

typedef struct cccrack cccrack_t;

static inline uint64_t
cccrack_get_tagging_count(const cccrack_t *self)
{
  return symtag_get_tagging_count(self->symtag);
}

static inline unsigned int
cccrack_get_candidate_count(const cccrack_t *self)
{
  return self->rankdef_count;
}

static inline const cccrack_rankdef_t *
cccrack_get_candidate(const cccrack_t *self, unsigned int i)
{
  return self->rankdef_list[i];
}

void cccrack_destroy(cccrack_t *self);

BOOL cccrack_run(cccrack_t *self);

cccrack_t *cccrack_new(const char *path, const struct cccrack_params *);

#endif /* _MAIN_INCLUDE_H */
