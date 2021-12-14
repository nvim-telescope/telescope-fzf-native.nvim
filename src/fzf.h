#ifndef _FZF_H_
#define _FZF_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
    #define DLLEXPORT __declspec(dllexport)
#else
    #define DLLEXPORT
#endif

typedef struct {
  int16_t *data;
  size_t size;
  size_t cap;
  bool allocated;
} fzf_i16_t;

typedef struct {
  int32_t *data;
  size_t size;
  size_t cap;
  bool allocated;
} fzf_i32_t;

typedef struct {
  uint32_t *data;
  size_t size;
  size_t cap;
} fzf_position_t;

typedef struct {
  int32_t start;
  int32_t end;
  int32_t score;

  fzf_position_t *pos;
} fzf_result_t;

typedef struct {
  fzf_i16_t I16;
  fzf_i32_t I32;
} fzf_slab_t;

typedef enum {
  term_fuzzy = 0,
  term_exact,
  term_prefix,
  term_suffix,
  term_equal
} fzf_alg_types;

typedef enum { case_smart = 0, case_ignore, case_respect } fzf_case_types;

typedef struct {
  fzf_alg_types typ;
  bool inv;
  char *ptr;
  void *text;
  bool case_sensitive;
} fzf_term_t;

typedef struct {
  fzf_term_t *ptr;
  size_t size;
  size_t cap;
} fzf_term_set_t;

typedef struct {
  fzf_term_set_t **ptr;
  size_t size;
  size_t cap;
  bool only_inv;
} fzf_pattern_t;

fzf_result_t fzf_fuzzy_match_v1(bool case_sensitive, bool normalize,
                                const char *text, const char *pattern,
                                bool with_pos, fzf_slab_t *slab);
fzf_result_t fzf_fuzzy_match_v2(bool case_sensitive, bool normalize,
                                const char *text, const char *pattern,
                                bool with_pos, fzf_slab_t *slab);
fzf_result_t fzf_exact_match_naive(bool case_sensitive, bool normalize,
                                   const char *text, const char *pattern,
                                   bool with_pos, fzf_slab_t *slab);
fzf_result_t fzf_prefix_match(bool case_sensitive, bool normalize,
                              const char *text, const char *pattern,
                              bool with_pos, fzf_slab_t *slab);
fzf_result_t fzf_suffix_match(bool case_sensitive, bool normalize,
                              const char *text, const char *pattern,
                              bool with_pos, fzf_slab_t *slab);
fzf_result_t fzf_equal_match(bool case_sensitive, bool normalize,
                             const char *text, const char *pattern,
                             bool with_pos, fzf_slab_t *slab);

/* interface */
DLLEXPORT fzf_pattern_t *fzf_parse_pattern(fzf_case_types case_mode, bool normalize,
                                 char *pattern, bool fuzzy);
DLLEXPORT void fzf_free_pattern(fzf_pattern_t *pattern);

DLLEXPORT int32_t fzf_get_score(const char *text, fzf_pattern_t *pattern,
                      fzf_slab_t *slab);
DLLEXPORT fzf_position_t *fzf_get_positions(const char *text, fzf_pattern_t *pattern,
                                  fzf_slab_t *slab);
DLLEXPORT void fzf_free_positions(fzf_position_t *pos);

DLLEXPORT fzf_slab_t *fzf_make_slab(size_t size_16, size_t size_32);
DLLEXPORT fzf_slab_t *fzf_make_default_slab(void);
DLLEXPORT void fzf_free_slab(fzf_slab_t *slab);

#endif // _fzf_H_
