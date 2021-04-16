#ifndef _FZF_H_
#define _FZF_H_

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  int16_t *data;
  size_t size;
  size_t cap;
  bool allocated;
} i16_t;

typedef struct {
  int32_t *data;
  size_t size;
  size_t cap;
  bool allocated;
} i32_t;

typedef struct {
  char *data;
  size_t size;
} string_t;

typedef struct {
  int32_t *data;
  int32_t size;
  int32_t cap;
} position_t;

typedef struct {
  int32_t start;
  int32_t end;
  int32_t score;

  position_t *pos;
} result_t;

typedef struct {
  int32_t score;
  position_t *pos;
} score_pos_tuple_t;

typedef struct {
  i16_t I16;
  i32_t I32;
} slab_t;

typedef enum {
  score_match = 16,
  score_gap_start = -3,
  score_gap_extention = -1,
  bonus_boundary = score_match / 2,
  bonus_non_word = score_match / 2,
  bonus_camel_123 = bonus_boundary + score_gap_extention,
  bonus_consecutive = -(score_gap_start + score_gap_extention),
  bonus_first_char_multiplier = 2,
} score_types;

typedef int32_t char_class;
typedef char byte; // ADDITIONAL NEED TO FIGURE OUT THESE MISSING TYPES

typedef enum {
  char_non_word = 0,
  char_lower,
  char_upper,
  char_letter,
  char_number
} char_types;

typedef enum {
  term_fuzzy = 0,
  term_exact,
  term_prefix,
  term_suffix,
  term_equal
} alg_types;

typedef enum { case_smart = 0, case_ignore, case_respect } case_types;

typedef result_t (*algorithm_t)(bool, bool, bool, string_t *, string_t *, bool,
                                slab_t *);

typedef struct {
  alg_types typ;
  algorithm_t alg;
  bool inv;
  char *ptr;
  string_t text;
  bool case_sensitive;
} term_t;

typedef struct {
  term_t *ptr;
  int32_t size;
  int32_t cap;
} term_set_t;

typedef struct {
  term_set_t **ptr;
  int32_t size;
  int32_t cap;
} pattern_t;

/* Algorithms */
result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                        string_t *text, string_t *pattern, bool with_pos,
                        slab_t *slab);
score_pos_tuple_t calculate_score(bool case_sensitive, bool normalize,
                                  string_t *text, string_t *pattern,
                                  int32_t sidx, int32_t eidx, bool with_pos);
result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        string_t *text, string_t *pattern, bool with_pos,
                        slab_t *slab);
result_t exact_match_naive(bool case_sensitive, bool normalize, bool forward,
                           string_t *text, string_t *pattern, bool with_pos,
                           slab_t *slab);
result_t prefix_match(bool case_sensitive, bool normalize, bool forward,
                      string_t *text, string_t *pattern, bool with_pos,
                      slab_t *slab);
result_t suffix_match(bool case_sensitive, bool normalize, bool forward,
                      string_t *text, string_t *pattern, bool with_pos,
                      slab_t *slab);
result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                     string_t *text, string_t *pattern, bool with_pos,
                     slab_t *slab);

/* interface */
pattern_t *parse_pattern(case_types case_mode, bool normalize, char *pattern);
void free_pattern(pattern_t *pattern);

position_t get_positions(char *text, pattern_t *pattern, slab_t *slab);
int32_t get_score(char *text, pattern_t *pattern, slab_t *slab);

slab_t *make_slab(int32_t size_16, int32_t size_32);
void free_slab(slab_t *slab);

/* UTILS */
// Helpers for slice
#define slice_def(name, type)                                                  \
  typedef struct {                                                             \
    type *data;                                                                \
    int32_t size;                                                              \
  } name##_slice_t;                                                            \
                                                                               \
  name##_slice_t slice_##name(type *input, int32_t from, int32_t to);          \
  name##_slice_t slice_##name##_right(type *input, int32_t to);
slice_def(i16, int16_t);
slice_def(i32, int32_t);
slice_def(str, char);
#undef slice_def

#endif // _fzf_H_
