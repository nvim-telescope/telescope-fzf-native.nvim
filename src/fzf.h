#include "util.h"

int32_t index_at(int32_t index, int32_t max, bool forward);

typedef struct {
  int32_t start;
  int32_t end;
  int32_t score;
} result_t;

typedef struct {
  i16_t I16;
  i32_t I32;
} slab_t;

typedef enum {
  score_match = 16,
  score_gap_start = -3,
  score_gap_extention = -1,

  // We prefer matches at the beginning of a word, but the bonus should not be
  // too great to prevent the longer acronym matches from always winning over
  // shorter fuzzy matches. The bonus point here was specifically chosen that
  // the bonus is cancelled when the gap between the acronyms grows over
  // 8 characters, which is approximately the average length of the words found
  // in web2 dictionary and my file system.
  bonus_boundary = score_match / 2,

  // Although bonus point for non-word characters is non-contextual, we need it
  // for computing bonus points for consecutive chunks starting with a non-word
  // character.
  bonus_non_word = score_match / 2,

  // Edge-triggered bonus for matches in camelCase words.
  // Compared to word-boundary case, they don't accompany single-character gaps
  // (e.g. FooBar vs. foo-bar), so we deduct bonus point accordingly.
  bonus_camel_123 = bonus_boundary + score_gap_extention,

  // Minimum bonus point given to characters in consecutive chunks.
  // Note that bonus points for consecutive matches shouldn't have needed if we
  // used fixed match score as in the original algorithm.
  bonus_consecutive = -(score_gap_start + score_gap_extention),

  // The first character in the typed pattern usually has more significance
  // than the rest so it's important that it appears at special positions where
  // bonus points are given. e.g. "to-go" vs. "ongoing" on "og" or on "ogo".
  // The amount of the extra bonus should be limited so that the gap penalty is
  // still respected.
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

int32_t *pos_array(bool with_pos, int32_t len);
i16_t *alloc16(int32_t *offset, slab_t *slab, int32_t size);
i16_t *alloc16_no(int32_t offset, slab_t *slab, int32_t size);

i32_t *alloc32(int32_t *offset, slab_t *slab, int32_t size);
i32_t *alloc32_no(int32_t offset, slab_t *slab, int32_t size);

charClass char_class_of_ascii(rune ch);
charClass char_class_of_non_ascii(rune ch); // TODO(conni2461)

int16_t bonus_for(charClass prev_class, charClass class);
int16_t bonus_at(chars_t *input, int32_t idx);

rune normalie_rune(rune r); // TODO(conni2461)

int32_t try_skip(chars_t *input, bool case_sensitive, byte b, int32_t from);
bool is_ascii(rune *runes, int32_t size);
int32_t ascii_fuzzy_index(chars_t *input, rune *pattern, int32_t size,
                          bool case_sensitive);

result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                         chars_t *input, rune *pattern, bool with_pos,
                         slab_t *slab);

result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                         chars_t *input, rune *pattern, bool with_pos,
                         slab_t *slab);

// BACKWARDS
result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool withPos, slab_t *slab);

// HELPERS
result_t init_result(int32_t start, int32_t end, int32_t score);
int32_t calculate_score(bool case_sensitive, bool normalize, char *text,
                        char *pattern);
