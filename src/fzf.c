#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int index_at(int index, int max, bool forward) {
  if (forward) {
    return index;
  }
  return max - index - 1;
}

typedef struct {
  int start;
  int end;
  int score;
} result_t;

typedef struct {
  int16_t *I16;
  int32_t *I32;
} slab_t;

typedef char byte; // ADDITIONAL NEED TO FIGURE OUT THESE MISSING TYPES

//////////////////////////////////////////////////////////////////////////////
// ADDITIONAL
typedef struct {
  char *data; // byte or rune, think about this
  size_t size;
} string_t;

typedef struct {
  string_t slice;
  bool in_bytes;
  bool trim_length_known;
  u_int16_t trim_length;

  int32_t index;
} chars_t;

void *slice_int32(int32_t *input, int32_t size, int32_t from, int32_t to) {
  /* TODO(conni2461): We need more slices, macro time */
}

string_t *slice_of_chars(string_t *input, int32_t from, int32_t to) {
  string_t *ret;
  ret->size = to - from;
  /* TODO(conni2461): IF you change it to rune you have to have rune here */
  ret->data = malloc(ret->size * sizeof(char));
  for (int32_t i = from; i < to; i++) {
    ret->data[i] = input->data[i];
  }

  return ret;
}

string_t *slice_of_chars_left(string_t *input, int32_t from) {
  return slice_of_chars(input, from, input->size);
}

string_t *slice_of_chars_right(string_t *input, int32_t to) {
  return slice_of_chars(input, 0, to);
}

int32_t index_byte(string_t *string, byte b) {
  for (int i = 0; i < string->size; i++) {
    if (string->data[i] == b) {
      return i;
    }
  }
  return -1;
}
//////////////////////////////////////////////////////////////////////////////

#define scoreMatch 16
#define scoreGapStart -3
#define scoreGapExtention -1

// We prefer matches at the beginning of a word, but the bonus should not be
// too great to prevent the longer acronym matches from always winning over
// shorter fuzzy matches. The bonus point here was specifically chosen that
// the bonus is cancelled when the gap between the acronyms grows over
// 8 characters, which is approximately the average length of the words found
// in web2 dictionary and my file system.
#define bonusBoundary scoreMatch / 2

// Although bonus point for non-word characters is non-contextual, we need it
// for computing bonus points for consecutive chunks starting with a non-word
// character.
#define bonusNonWord scoreMatch / 2

// Edge-triggered bonus for matches in camelCase words.
// Compared to word-boundary case, they don't accompany single-character gaps
// (e.g. FooBar vs. foo-bar), so we deduct bonus point accordingly.
#define bonusCamel123 bonusBoundary + scoreGapExtention

// Minimum bonus point given to characters in consecutive chunks.
// Note that bonus points for consecutive matches shouldn't have needed if we
// used fixed match score as in the original algorithm.
#define bonusConsecutive -(scoreGapStart + scoreGapExtention)

// The first character in the typed pattern usually has more significance
// than the rest so it's important that it appears at special positions where
// bonus points are given. e.g. "to-go" vs. "ongoing" on "og" or on "ogo".
// The amount of the extra bonus should be limited so that the gap penalty is
// still respected.
#define bonusFirstCharMultiplier 2

typedef int charClass;
typedef int rune; // ADDITIONAL 32bit char

typedef enum {
  char_non_word = 0,
  char_lower,
  char_upper,
  char_letter,
  char_number
} char_types;

int *pos_array(bool with_pos, int len) {
  if (with_pos) {
    int *pos = malloc(len * sizeof(int));
    return pos;
  }
  return NULL;
}

/* TODO(conni2461): alloc16 line 161 - 167 */
/* TODO(conni2461): alloc32 line 169 - 175 */

charClass char_class_of_ascii(rune ch) {
  if (ch >= 'a' && ch <= 'z') {
    return char_lower;
  } else if (ch >= 'A' && ch <= 'Z') {
    return char_upper;
  } else if (ch >= '0' && ch <= '9') {
    return char_number;
  }
  return char_non_word;
}

charClass char_class_of_non_ascii(rune ch) {
  /* TODO(conni2461): char_class_of_non_ascii line 188 - 199 */
  return 0;
}

#define UNICODE_MAXASCII 0x7f

charClass char_class_of(rune ch) {
  if (ch <= UNICODE_MAXASCII) {
    return char_class_of_ascii(ch);
  }
  return char_class_of_non_ascii(ch);
}

/* TODO(conni2461): probably use explicit sizes int16_t, int32_t, ... */
int16_t bonus_for(charClass prev_class, charClass class) {
  if (prev_class == char_non_word && class != char_non_word) {
    return bonusBoundary;
  } else if ((prev_class == char_lower && class == char_upper) ||
             (prev_class != char_number && class == char_number)) {
    return bonusCamel123;
  } else if (class == char_non_word) {
    return bonusNonWord;
  }
  return 0;
}

int16_t bonus_at(chars_t *input, int idx) {
  if (idx == 0) {
    return bonusBoundary;
  }
  return bonus_for(char_class_of(input->slice.data[idx - 1]),
                   char_class_of(input->slice.data[idx]));
}

rune normalie_rune(rune r) {
  if (r < 0x00C0 || r > 0x2184) {
    return r;
  }
  /* rune n = normalized[r]; */
  /* if n > 0 { */
  /*   return n; */
  /* } */
  return r;
}

typedef result_t (*Algo)(bool case_sensitive, bool normalize, bool forward,
                         chars_t *input, rune *pattern, bool with_pos,
                         slab_t *slab);

int32_t try_skip(chars_t *input, bool case_sensitive, byte b, int32_t from) {
  // byteArray := input.Bytes()[from:]
  // basically a slice from till end
  string_t *byte_array = slice_of_chars_left(&input->slice, from);
  int32_t idx = index_byte(byte_array, b);
  if (idx == 0) {
    return from;
  }

  if (!case_sensitive && b >= 'a' && b <= 'z') {
    if (idx > 0) {
      /* TODO(conni2461): slice right */
      string_t *tmp = slice_of_chars_right(byte_array, idx);
      free(byte_array);
      byte_array = tmp;
    }
    int32_t uidx = index_byte(byte_array, b - 32);
    if (uidx >= 0) {
      idx = uidx;
    }
  }
  if (idx < 0) {
    return -1;
  }

  free(byte_array);
  return from + idx;
}

bool is_ascii(rune *runes, int32_t size) {
  for (int32_t i = 0; i < size; i++) {
    if (runes[i] >= 256) {
      return false;
    }
  }
  return true;
}

int32_t ascii_fuzzy_index(chars_t *input, rune *pattern, int32_t size,
                          bool case_sensitive) {
  // can't determine
  if (!input->in_bytes) {
    return 0;
  }

  if (!is_ascii(pattern, size)) {
    return -1;
  }

  int32_t first_idx = 0;
  int32_t idx = 0;
  for (int32_t pidx = 0; pidx < size; pidx++) {
    idx = try_skip(input, case_sensitive, pattern[pidx], idx);
    if (idx < 0) {
      return -1;
    }
    if (pidx == 0 && idx > 0) {
      first_idx = idx - 1;
    }
    idx++;
  }

  return first_idx;
}

/* TODO(conni2461): maybe i add debugv2 maybe not LUL */

/* TODO(conni2461): return capacitiy */
int32_t cap(void *t) {
  return 0;
}

result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        chars_t *input, rune *pattern, bool with_pos,
                        slab_t *slab);

result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                        chars_t *input, rune *pattern, bool with_pos,
                        slab_t *slab) {
  int32_t M = strlen(pattern);
  result_t ret;
  if (M == 0) {
    ret.start = 0;
    ret.end = 0;
    ret.score = 0;
    return ret; // TODO(conni2461): positions
  }
  int32_t N = input->slice.size;
  if (slab != NULL && N * M > cap(slab->I16)) {
    return fuzzy_match_v2(case_sensitive, normalize, forward, input, pattern,
                          with_pos, slab);
  }

  int32_t idx = ascii_fuzzy_index(input, pattern, M, case_sensitive);
  if (idx < 0) {
    ret.start = 0;
    ret.end = 0;
    ret.score = 0;
    return ret; // TODO(conni2461): positions
  }

  int16_t offset16 = 0;
  int32_t offset32 = 0;
  int16_t *H0; // alloc16(offset16, slab, N)
  int16_t *C0; // alloc16(offset16, slab, N)
  // Bonus point for each positions
  int16_t *B; // alloc16(offset16, slab, N)
  // The first occurrence of each character in the pattern
  int32_t *F; // alloc32(offset32, slab, M)
  // Rune array
  int32_t *T; // alloc32(offset32, slab, N)
  /* input.CopyRunes(T) */

  // Phase 2. Calculate bonus for each point
  int16_t max_score = 0;
  int32_t max_score_pos = 0;

  int32_t pidx = 0;
  int32_t last_idx = 0;

  rune pchar0 = pattern[0];
  rune pchar = pattern[0];
  int16_t prevH0 = 0;
  int32_t prev_class = char_non_word;
  bool in_gap = false;

  /* int32_t Tsub = T[idx:] */
  /* H0sub, C0sub, Bsub := H0[idx:][:len(Tsub)], C0[idx:][:len(Tsub)], B[idx:][:len(Tsub)] */


  return ret;
}
