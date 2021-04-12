#ifndef _FZF_H_
#define _FZF_H_

#include "util.h"

int32_t index_at(int32_t index, int32_t max, bool forward);

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

typedef int32_t charClass;
typedef char byte; // ADDITIONAL NEED TO FIGURE OUT THESE MISSING TYPES

typedef enum {
  char_non_word = 0,
  char_lower,
  char_upper,
  char_letter,
  char_number
} char_types;

position_t *pos_array(bool with_pos, int32_t len);
void append_pos(position_t *pos, int32_t value);

i16_t alloc16(int32_t *offset, slab_t *slab, int32_t size, bool *allocated);
i16_t alloc16_no(int32_t offset, slab_t *slab, int32_t size, bool *allocated);

i32_t alloc32(int32_t *offset, slab_t *slab, int32_t size, bool *allocated);
i32_t alloc32_no(int32_t offset, slab_t *slab, int32_t size, bool *allocated);

charClass char_class_of_ascii(rune ch);
charClass char_class_of_non_ascii(rune ch); // TODO(conni2461)

int16_t bonus_for(charClass prev_class, charClass class);
int16_t bonus_at(chars_t *input, int32_t idx);

rune normalie_rune(rune r); // TODO(conni2461)

int32_t try_skip(chars_t *input, bool case_sensitive, byte b, int32_t from);
bool is_ascii(rune *runes, int32_t size);
int32_t ascii_fuzzy_index(chars_t *input, rune *pattern, int32_t size,
                          bool case_sensitive);

result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                        chars_t *text, rune *pattern, bool with_pos,
                        slab_t *slab);
score_pos_tuple_t calculate_score(bool case_sensitive, bool normalize,
                                  chars_t *text, rune *pattern, int32_t sidx,
                                  int32_t eidx, bool with_pos);
result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        chars_t *text, rune *pattern, bool with_pos,
                        slab_t *slab);
result_t exact_match_naive(bool case_sensitive, bool normalize, bool forward,
                           chars_t *text, rune *pattern, bool with_pos,
                           slab_t *slab);
result_t prefix_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool with_pos,
                      slab_t *slab);
result_t suffix_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool with_pos,
                      slab_t *slab);
result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                     chars_t *text, rune *pattern, bool with_pos, slab_t *slab);

// HELPERS
#define free_alloc(obj, b)                                                     \
  if (b) {                                                                     \
    free(obj.data);                                                            \
  }

typedef enum {
  term_fuzzy = 0,
  term_exact,
  term_prefix,
  term_suffix,
  term_equal
} alg_types;

typedef struct {
  alg_types typ;
  bool inv;
  char *og_str;
  char *text;
  bool case_sensitive;
  bool normalize;
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
} term_set_sets_t;

void append_set(term_set_t *set, term_t value);
void append_sets(term_set_sets_t *set, term_set_t *value);

// TODO(conni2461): Return pattern. pattern has even more required information
term_set_sets_t *build_pattern_fun(bool case_sensitive, bool normalize,
                                   char *pattern);
term_set_sets_t *parse_terms(bool case_sensitive, bool normalize,
                             char *pattern);
void free_sets(term_set_sets_t *sets);

int32_t get_match(char *text, term_set_sets_t *sets, slab_t *slab);
int32_t get_match_bad(bool case_sensitive, bool normalize, char *text,
                      char *pattern);

slab_t *make_slab(int32_t size_16, int32_t size_32);
void free_slab(slab_t *slab);

#endif // _fzf_H_
