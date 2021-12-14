#include "fzf.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// TODO(conni2461): UNICODE HEADER
#define UNICODE_MAXASCII 0x7f

/* Helpers */
#define free_alloc(obj)                                                        \
  if (obj.allocated) {                                                         \
    free(obj.data);                                                            \
  }

#define gen_slice(name, type)                                                  \
  typedef struct {                                                             \
    type *data;                                                                \
    size_t size;                                                               \
  } name##_slice_t;                                                            \
  static name##_slice_t slice_##name(type *input, size_t from, size_t to) {    \
    return (name##_slice_t){.data = input + from, .size = to - from};          \
  }                                                                            \
  static name##_slice_t slice_##name##_right(type *input, size_t to) {         \
    return slice_##name(input, 0, to);                                         \
  }

#define gen_simple_slice(name, type)                                           \
  typedef struct {                                                             \
    type *data;                                                                \
    size_t size;                                                               \
  } name##_slice_t;                                                            \
  static name##_slice_t slice_##name(type *input, size_t from, size_t to) {    \
    return (name##_slice_t){.data = input + from, .size = to - from};          \
  }

gen_slice(i16, int16_t);
gen_simple_slice(i32, int32_t);
gen_slice(str, const char);
#undef gen_slice
#undef gen_simple_slice

/* TODO(conni2461): additional types (utf8) */
typedef int32_t char_class;
typedef char byte;

typedef enum {
  score_match = 16,
  score_gap_start = -3,
  score_gap_extention = -1,
  bonus_boundary = score_match / 2,
  bonus_non_word = score_match / 2,
  bonus_camel_123 = bonus_boundary + score_gap_extention,
  bonus_consecutive = -(score_gap_start + score_gap_extention),
  bonus_first_char_multiplier = 2,
} score_t;

typedef enum {
  char_non_word = 0,
  char_lower,
  char_upper,
  char_letter,
  char_number
} char_types;

typedef struct {
  const char *data;
  size_t size;
} fzf_string_t;

static int32_t index_byte(fzf_string_t *string, char b) {
  for (size_t i = 0; i < string->size; i++) {
    if (string->data[i] == b) {
      return (int32_t)i;
    }
  }
  return -1;
}

static size_t leading_whitespaces(fzf_string_t *str) {
  size_t whitespaces = 0;
  for (size_t i = 0; i < str->size; i++) {
    if (!isspace((unsigned char)str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

static size_t trailing_whitespaces(fzf_string_t *str) {
  size_t whitespaces = 0;
  for (size_t i = str->size - 1; i >= 0; i--) {
    if (!isspace((unsigned char)str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

static void copy_runes(fzf_string_t *src, fzf_i32_t *destination) {
  for (size_t i = 0; i < src->size; i++) {
    destination->data[i] = (int32_t)src->data[i];
  }
}

static void copy_into_i16(i16_slice_t *src, fzf_i16_t *dest) {
  for (size_t i = 0; i < src->size; i++) {
    dest->data[i] = src->data[i];
  }
}

// char* helpers
static char *trim_left(char *str, size_t *len, char trim) {
  for (size_t i = 0; i < *len; i++) {
    if (str[0] == trim) {
      (*len)--;
      str++;
    } else {
      break;
    }
  }
  return str;
}

static bool has_prefix(const char *str, const char *prefix, size_t prefix_len) {
  return strncmp(prefix, str, prefix_len) == 0;
}

static bool has_suffix(const char *str, size_t len, const char *suffix,
                       size_t suffix_len) {
  return len >= suffix_len &&
         strncmp(slice_str(str, len - suffix_len, len).data, suffix,
                 suffix_len) == 0;
}

// TODO(conni2461): REFACTOR
static char *str_replace(char *orig, char *rep, char *with) {
  char *result, *ins, *tmp;
  size_t len_rep, len_with, len_front, count;

  if (!orig || !rep) {
    return NULL;
  }
  len_rep = strlen(rep);
  if (len_rep == 0) {
    return NULL;
  }
  if (!with) {
    with = "";
  }
  len_with = strlen(with);

  ins = orig;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result =
      (char *)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

  if (!result) {
    return NULL;
  }

  while (count--) {
    ins = strstr(orig, rep);
    len_front = (size_t)(ins - orig);
    tmp = strncpy(tmp, orig, len_front) + len_front;
    tmp = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

// TODO(conni2461): REFACTOR
static char *str_tolower(char *str, size_t size) {
  char *lower_str = (char *)malloc((size + 1) * sizeof(char));
  for (size_t i = 0; i < size; i++) {
    lower_str[i] = (char)tolower(str[i]);
  }
  lower_str[size] = '\0';
  return lower_str;
}

static int16_t max16(int16_t a, int16_t b) {
  return (a > b) ? a : b;
}

static size_t min64u(size_t a, size_t b) {
  return (a < b) ? a : b;
}

static fzf_position_t *pos_array(bool with_pos, size_t len) {
  if (with_pos) {
    fzf_position_t *pos = (fzf_position_t *)malloc(sizeof(fzf_position_t));
    pos->size = 0;
    pos->cap = len;
    pos->data = (uint32_t *)malloc(len * sizeof(uint32_t));
    return pos;
  }
  return NULL;
}

static void resize_pos(fzf_position_t *pos, size_t add_len, size_t comp) {
  if (pos->size + comp > pos->cap) {
    pos->cap += add_len;
    pos->data = (uint32_t *)realloc(pos->data, sizeof(uint32_t) * pos->cap);
  }
}

static void append_pos(fzf_position_t *pos, size_t value) {
  resize_pos(pos, pos->cap, 1);
  pos->data[pos->size] = value;
  pos->size++;
}

static void concat_pos(fzf_position_t *left, fzf_position_t *right) {
  resize_pos(left, right->size, right->size);
  memcpy(left->data + left->size, right->data, right->size * sizeof(uint32_t));
  left->size += right->size;
}

static void insert_pos(fzf_position_t *pos, size_t start, size_t end) {
  resize_pos(pos, end - start, end - start);
  for (size_t k = start; k < end; k++) {
    pos->data[pos->size] = k;
    pos->size++;
  }
}

static fzf_i16_t alloc16(size_t *offset, fzf_slab_t *slab, size_t size) {
  if (slab != NULL && slab->I16.cap > *offset + size) {
    i16_slice_t slice = slice_i16(slab->I16.data, *offset, (*offset) + size);
    *offset = *offset + size;
    return (fzf_i16_t){.data = slice.data,
                       .size = slice.size,
                       .cap = slice.size,
                       .allocated = false};
  }
  int16_t *data = (int16_t *)malloc(size * sizeof(int16_t));
  return (fzf_i16_t){
      .data = data, .size = size, .cap = size, .allocated = true};
}

static fzf_i32_t alloc32(size_t *offset, fzf_slab_t *slab, size_t size) {
  if (slab != NULL && slab->I32.cap > *offset + size) {
    i32_slice_t slice = slice_i32(slab->I32.data, *offset, (*offset) + size);
    *offset = *offset + size;
    return (fzf_i32_t){.data = slice.data,
                       .size = slice.size,
                       .cap = slice.size,
                       .allocated = false};
  }
  int32_t *data = (int32_t *)malloc(size * sizeof(int32_t));
  return (fzf_i32_t){
      .data = data, .size = size, .cap = size, .allocated = true};
}

static char_class char_class_of_ascii(char ch) {
  if (ch >= 'a' && ch <= 'z') {
    return char_lower;
  } else if (ch >= 'A' && ch <= 'Z') {
    return char_upper;
  } else if (ch >= '0' && ch <= '9') {
    return char_number;
  }
  return char_non_word;
}

// static char_class char_class_of_non_ascii(char ch) {
//   return 0;
// }

static char_class char_class_of(char ch) {
  return char_class_of_ascii(ch);
  // if (ch <= 0x7f) {
  //   return char_class_of_ascii(ch);
  // }
  // return char_class_of_non_ascii(ch);
}

static int16_t bonus_for(char_class prev_class, char_class class) {
  if (prev_class == char_non_word && class != char_non_word) {
    return bonus_boundary;
  } else if ((prev_class == char_lower && class == char_upper) ||
             (prev_class != char_number && class == char_number)) {
    return bonus_camel_123;
  } else if (class == char_non_word) {
    return bonus_non_word;
  }
  return 0;
}

static int16_t bonus_at(fzf_string_t *input, size_t idx) {
  if (idx == 0) {
    return bonus_boundary;
  }
  return bonus_for(char_class_of(input->data[idx - 1]),
                   char_class_of(input->data[idx]));
}

/* TODO(conni2461): maybe just not do this */
static char normalize_rune(char r) {
  // TODO(conni2461)
  /* if (r < 0x00C0 || r > 0x2184) { */
  /*   return r; */
  /* } */
  /* rune n = normalized[r]; */
  /* if n > 0 { */
  /*   return n; */
  /* } */
  return r;
}

static int32_t try_skip(fzf_string_t *input, bool case_sensitive, byte b,
                        int32_t from) {
  str_slice_t slice = slice_str(input->data, (size_t)from, input->size);
  fzf_string_t byte_array = {.data = slice.data, .size = slice.size};
  int32_t idx = index_byte(&byte_array, b);
  if (idx == 0) {
    return from;
  }

  if (!case_sensitive && b >= 'a' && b <= 'z') {
    if (idx > 0) {
      str_slice_t tmp = slice_str_right(byte_array.data, (size_t)idx);
      byte_array.data = tmp.data;
      byte_array.size = tmp.size;
    }
    int32_t uidx = index_byte(&byte_array, b - (byte)32);
    if (uidx >= 0) {
      idx = uidx;
    }
  }
  if (idx < 0) {
    return -1;
  }

  return from + idx;
}

static bool is_ascii(const char *runes, size_t size) {
  // TODO(conni2461): future use
  /* for (size_t i = 0; i < size; i++) { */
  /*   if (runes[i] >= 256) { */
  /*     return false; */
  /*   } */
  /* } */
  return true;
}

static int32_t ascii_fuzzy_index(fzf_string_t *input, const char *pattern,
                                 size_t size, bool case_sensitive) {
  if (!is_ascii(pattern, size)) {
    return -1;
  }

  int32_t first_idx = 0, idx = 0;
  for (size_t pidx = 0; pidx < size; pidx++) {
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

typedef struct {
  int32_t score;
  fzf_position_t *pos;
} score_pos_tuple_t;

static score_pos_tuple_t fzf_calculate_score(bool case_sensitive,
                                             bool normalize, fzf_string_t *text,
                                             fzf_string_t *pattern, size_t sidx,
                                             size_t eidx, bool with_pos) {
  const size_t len_pattern = pattern->size;

  size_t pidx = 0;
  int32_t score = 0, consecutive = 0;
  bool in_gap = false;
  int16_t first_bonus = 0;
  fzf_position_t *pos = pos_array(with_pos, len_pattern);
  int32_t prev_class = char_non_word;
  if (sidx > 0) {
    prev_class = char_class_of(text->data[sidx - 1]);
  }
  for (size_t idx = sidx; idx < eidx; idx++) {
    char c = text->data[idx];
    int32_t class = char_class_of(c);
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c == pattern->data[pidx]) {
      if (with_pos) {
        append_pos(pos, idx);
      }
      score += score_match;
      int16_t bonus = bonus_for(prev_class, class);
      if (consecutive == 0) {
        first_bonus = bonus;
      } else {
        if (bonus == bonus_boundary) {
          first_bonus = bonus;
        }
        bonus = max16(max16(bonus, first_bonus), bonus_consecutive);
      }
      if (pidx == 0) {
        score += (int32_t)(bonus * bonus_first_char_multiplier);
      } else {
        score += (int32_t)bonus;
      }
      in_gap = false;
      consecutive++;
      pidx++;
    } else {
      if (in_gap) {
        score += score_gap_extention;
      } else {
        score += score_gap_start;
      }
      in_gap = true;
      consecutive = 0;
      first_bonus = 0;
    }
    prev_class = class;
  }
  return (score_pos_tuple_t){score, pos};
}

static fzf_result_t __fuzzy_match_v1(bool case_sensitive, bool normalize,
                                     fzf_string_t *text, fzf_string_t *pattern,
                                     bool with_pos, fzf_slab_t *slab) {
  const size_t len_pattern = pattern->size;
  const size_t len_runes = text->size;
  if (len_pattern == 0) {
    return (fzf_result_t){0, 0, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern->data, len_pattern, case_sensitive) < 0) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }

  int32_t pidx = 0;
  int32_t sidx = -1, eidx = -1;
  for (size_t idx = 0; idx < len_runes; idx++) {
    char c = text->data[idx];
    /* TODO(conni2461): Common pattern maybe a macro would be good here */
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c == pattern->data[pidx]) {
      if (sidx < 0) {
        sidx = (int32_t)idx;
      }
      pidx++;
      if (pidx == len_pattern) {
        eidx = (int32_t)idx + 1;
        break;
      }
    }
  }
  if (sidx >= 0 && eidx >= 0) {
    size_t start = (size_t)sidx, end = (size_t)eidx;
    pidx--;
    for (size_t idx = end - 1; idx >= start; idx--) {
      char c = text->data[idx];
      if (!case_sensitive) {
        /* TODO(conni2461): He does some unicode stuff here, investigate */
        c = (char)tolower(c);
      }
      if (c == pattern->data[pidx]) {
        pidx--;
        if (pidx < 0) {
          start = idx;
          break;
        }
      }
    }

    score_pos_tuple_t tuple = fzf_calculate_score(
        case_sensitive, normalize, text, pattern, start, end, with_pos);
    return (fzf_result_t){(int32_t)start, (int32_t)end, tuple.score, tuple.pos};
  }
  return (fzf_result_t){-1, -1, 0, NULL};
}

fzf_result_t fzf_fuzzy_match_v1(bool case_sensitive, bool normalize,
                                const char *input, const char *pattern,
                                bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __fuzzy_match_v1(case_sensitive, normalize, &input_wrap, &pattern_wrap,
                          with_pos, slab);
}

static fzf_result_t __fuzzy_match_v2(bool case_sensitive, bool normalize,
                                     fzf_string_t *input, fzf_string_t *pattern,
                                     bool with_pos, fzf_slab_t *slab) {
  const size_t M = pattern->size;
  const size_t N = input->size;
  if (M == 0) {
    return (fzf_result_t){0, 0, 0, pos_array(with_pos, M)};
  }
  if (slab != NULL && N * M > slab->I16.cap) {
    return __fuzzy_match_v1(case_sensitive, normalize, input, pattern, with_pos,
                            slab);
  }

  size_t idx;
  {
    int32_t tmp_idx =
        ascii_fuzzy_index(input, pattern->data, M, case_sensitive);
    if (tmp_idx < 0) {
      return (fzf_result_t){-1, -1, 0, NULL};
    }
    idx = (size_t)tmp_idx;
  }

  size_t offset16 = 0, offset32 = 0;
  fzf_i16_t H0 = alloc16(&offset16, slab, N);
  fzf_i16_t C0 = alloc16(&offset16, slab, N);
  // Bonus point for each positions
  fzf_i16_t B = alloc16(&offset16, slab, N);
  // The first occurrence of each character in the pattern
  fzf_i32_t F = alloc32(&offset32, slab, M);
  // Rune array
  fzf_i32_t T = alloc32(&offset32, slab, N);
  copy_runes(input, &T); // input.CopyRunes(T)

  // Phase 2. Calculate bonus for each point
  int16_t max_score = 0;
  size_t max_score_pos = 0;

  size_t pidx = 0, last_idx = 0;

  char pchar0 = pattern->data[0];
  char pchar = pattern->data[0];
  int16_t prevH0 = 0;
  int32_t prev_class = char_non_word;
  bool in_gap = false;

  i32_slice_t Tsub = slice_i32(T.data, idx, T.size); // T[idx:];
  i16_slice_t H0sub =
      slice_i16_right(slice_i16(H0.data, idx, H0.size).data, Tsub.size);
  i16_slice_t C0sub =
      slice_i16_right(slice_i16(C0.data, idx, C0.size).data, Tsub.size);
  i16_slice_t Bsub =
      slice_i16_right(slice_i16(B.data, idx, B.size).data, Tsub.size);

  for (size_t off = 0; off < Tsub.size; off++) {
    char_class class;
    char c = (char)Tsub.data[off];
    class = char_class_of_ascii(c);
    if (!case_sensitive && class == char_upper) {
      /* TODO(conni2461): unicode support */
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }

    Tsub.data[off] = c;
    int16_t bonus = bonus_for(prev_class, class);
    Bsub.data[off] = bonus;
    prev_class = class;
    if (c == pchar) {
      if (pidx < M) {
        F.data[pidx] = (int32_t)(idx + off);
        pidx++;
        pchar = pattern->data[min64u(pidx, M - 1)];
      }
      last_idx = idx + off;
    }

    if (c == pchar0) {
      int16_t score = score_match + bonus * bonus_first_char_multiplier;
      H0sub.data[off] = score;
      C0sub.data[off] = 1;
      if (M == 1 && (score > max_score)) {
        max_score = score;
        max_score_pos = idx + off;
        if (bonus == bonus_boundary) {
          break;
        }
      }
      in_gap = false;
    } else {
      if (in_gap) {
        H0sub.data[off] = max16(prevH0 + score_gap_extention, 0);
      } else {
        H0sub.data[off] = max16(prevH0 + score_gap_start, 0);
      }
      C0sub.data[off] = 0;
      in_gap = true;
    }
    prevH0 = H0sub.data[off];
  }
  if (pidx != M) {
    free_alloc(T);
    free_alloc(F);
    free_alloc(B);
    free_alloc(C0);
    free_alloc(H0);
    return (fzf_result_t){-1, -1, 0, NULL};
  }
  if (M == 1) {
    free_alloc(T);
    free_alloc(F);
    free_alloc(B);
    free_alloc(C0);
    free_alloc(H0);
    fzf_result_t res = {(int32_t)max_score_pos, (int32_t)max_score_pos + 1,
                        max_score, NULL};
    if (!with_pos) {
      return res;
    }
    fzf_position_t *pos = pos_array(with_pos, 1);
    append_pos(pos, max_score_pos);
    res.pos = pos;
    return res;
  }

  size_t f0 = (size_t)F.data[0];
  size_t width = last_idx - f0 + 1;
  fzf_i16_t H = alloc16(&offset16, slab, width * M);
  {
    i16_slice_t H0_tmp_slice = slice_i16(H0.data, f0, last_idx + 1);
    copy_into_i16(&H0_tmp_slice, &H);
  }

  fzf_i16_t C = alloc16(&offset16, slab, width * M);
  {
    i16_slice_t C0_tmp_slice = slice_i16(C0.data, f0, last_idx + 1);
    copy_into_i16(&C0_tmp_slice, &C);
  }

  i32_slice_t Fsub = slice_i32(F.data, 1, F.size);
  str_slice_t Psub =
      slice_str_right(slice_str(pattern->data, 1, M).data, Fsub.size);
  for (size_t off = 0; off < Fsub.size; off++) {
    size_t f = (size_t)Fsub.data[off];
    pchar = Psub.data[off];
    pidx = off + 1;
    size_t row = pidx * width;
    in_gap = false;
    Tsub = slice_i32(T.data, f, last_idx + 1);
    Bsub = slice_i16_right(slice_i16(B.data, f, B.size).data, Tsub.size);
    i16_slice_t Csub = slice_i16_right(
        slice_i16(C.data, row + f - f0, C.size).data, Tsub.size);
    i16_slice_t Cdiag = slice_i16_right(
        slice_i16(C.data, row + f - f0 - 1 - width, C.size).data, Tsub.size);
    i16_slice_t Hsub = slice_i16_right(
        slice_i16(H.data, row + f - f0, H.size).data, Tsub.size);
    i16_slice_t Hdiag = slice_i16_right(
        slice_i16(H.data, row + f - f0 - 1 - width, H.size).data, Tsub.size);
    i16_slice_t Hleft = slice_i16_right(
        slice_i16(H.data, row + f - f0 - 1, H.size).data, Tsub.size);
    Hleft.data[0] = 0;
    for (size_t j = 0; j < Tsub.size; j++) {
      char c = (char)Tsub.data[j];
      size_t col = j + f;
      int16_t s1 = 0, s2 = 0;
      int16_t consecutive = 0;

      if (in_gap) {
        s2 = Hleft.data[j] + score_gap_extention;
      } else {
        s2 = Hleft.data[j] + score_gap_start;
      }

      if (pchar == c) {
        s1 = Hdiag.data[j] + score_match;
        int16_t b = Bsub.data[j];
        consecutive = Cdiag.data[j] + 1;
        if (b == bonus_boundary) {
          consecutive = 1;
        } else if (consecutive > 1) {
          b = max16(b, max16(bonus_consecutive,
                             B.data[col - ((size_t)consecutive) + 1]));
        }
        if (s1 + b < s2) {
          s1 += Bsub.data[j];
          consecutive = 0;
        } else {
          s1 += b;
        }
      }
      Csub.data[j] = consecutive;
      in_gap = s1 < s2;
      int16_t score = max16(max16(s1, s2), 0);
      if (pidx == M - 1 && (score > max_score)) {
        max_score = score;
        max_score_pos = col;
      }
      Hsub.data[j] = score;
    }
  }

  fzf_position_t *pos = pos_array(with_pos, M);
  size_t j = max_score_pos;
  if (with_pos) {
    size_t i = M - 1;
    bool prefer_match = true;
    for (;;) {
      size_t I = i * width;
      size_t j0 = j - f0;
      int16_t s = H.data[I + j0];

      int16_t s1 = 0;
      int16_t s2 = 0;
      if (i > 0 && j >= F.data[i]) {
        s1 = H.data[I - width + j0 - 1];
      }
      if (j > F.data[i]) {
        s2 = H.data[I + j0 - 1];
      }

      if (s > s1 && (s > s2 || (s == s2 && prefer_match))) {
        append_pos(pos, j);
        if (i == 0) {
          break;
        }
        i--;
      }
      prefer_match = C.data[I + j0] > 1 || (I + width + j0 + 1 < C.size &&
                                            C.data[I + width + j0 + 1] > 0);
      j--;
    }
  }

  free_alloc(H);
  free_alloc(C);
  free_alloc(T);
  free_alloc(F);
  free_alloc(B);
  free_alloc(C0);
  free_alloc(H0);
  return (fzf_result_t){(int32_t)j, (int32_t)max_score_pos + 1,
                        (int32_t)max_score, pos};
}

fzf_result_t fzf_fuzzy_match_v2(bool case_sensitive, bool normalize,
                                const char *input, const char *pattern,
                                bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __fuzzy_match_v2(case_sensitive, normalize, &input_wrap, &pattern_wrap,
                          with_pos, slab);
}

static fzf_result_t __exact_match_naive(bool case_sensitive, bool normalize,
                                        fzf_string_t *text,
                                        fzf_string_t *pattern, bool with_pos,
                                        fzf_slab_t *slab) {
  const size_t len_pattern = pattern->size;
  const size_t len_runes = text->size;

  if (len_pattern == 0) {
    return (fzf_result_t){0, 0, 0, NULL};
  }
  if (len_runes < len_pattern) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern->data, len_pattern, case_sensitive) < 0) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }

  size_t pidx = 0;
  int32_t best_pos = -1;
  int16_t bonus = 0;
  int16_t best_bonus = -1;
  for (size_t idx = 0; idx < len_runes; idx++) {
    size_t idx_ = idx;
    char c = text->data[idx_];
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    size_t pidx_ = pidx;
    if (c == pattern->data[pidx_]) {
      if (pidx_ == 0) {
        bonus = bonus_at(text, idx_);
      }
      pidx++;
      if (pidx == len_pattern) {
        if (bonus > best_bonus) {
          best_pos = (int32_t)idx;
          best_bonus = bonus;
        }
        if (bonus == bonus_boundary) {
          break;
        }
        idx -= pidx - 1;
        pidx = 0;
        bonus = 0;
      }
    } else {
      idx -= pidx;
      pidx = 0;
      bonus = 0;
    }
  }
  if (best_pos >= 0) {
    size_t bp = (size_t)best_pos;
    size_t sidx = bp - len_pattern + 1;
    size_t eidx = bp + 1;
    int32_t score = fzf_calculate_score(case_sensitive, normalize, text,
                                        pattern, sidx, eidx, false)
                        .score;
    return (fzf_result_t){(int32_t)sidx, (int32_t)eidx, score, NULL};
  }
  return (fzf_result_t){-1, -1, 0, NULL};
}

fzf_result_t fzf_exact_match_naive(bool case_sensitive, bool normalize,
                                   const char *input, const char *pattern,
                                   bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __exact_match_naive(case_sensitive, normalize, &input_wrap,
                             &pattern_wrap, with_pos, slab);
}

static fzf_result_t __prefix_match(bool case_sensitive, bool normalize,
                                   fzf_string_t *text, fzf_string_t *pattern,
                                   bool with_pos, fzf_slab_t *slab) {
  const size_t len_pattern = pattern->size;
  if (len_pattern == 0) {
    return (fzf_result_t){0, 0, 0, NULL};
  }
  size_t trimmed_len = 0;
  /* TODO(conni2461): i feel this is wrong */
  if (!isspace((unsigned char)pattern->data[0])) {
    trimmed_len = leading_whitespaces(text);
  }
  if (text->size - trimmed_len < len_pattern) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }
  for (size_t i = 0; i < len_pattern; i++) {
    char c = text->data[trimmed_len + i];
    if (!case_sensitive) {
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c != pattern->data[i]) {
      return (fzf_result_t){-1, -1, 0, NULL};
    }
  }
  size_t start = trimmed_len;
  size_t end = trimmed_len + len_pattern;
  int32_t score = fzf_calculate_score(case_sensitive, normalize, text, pattern,
                                      start, end, false)
                      .score;
  return (fzf_result_t){(int32_t)start, (int32_t)end, score, NULL};
}

fzf_result_t fzf_prefix_match(bool case_sensitive, bool normalize,
                              const char *input, const char *pattern,
                              bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __prefix_match(case_sensitive, normalize, &input_wrap, &pattern_wrap,
                        with_pos, slab);
}

static fzf_result_t __suffix_match(bool case_sensitive, bool normalize,
                                   fzf_string_t *text, fzf_string_t *pattern,
                                   bool with_pos, fzf_slab_t *slab) {
  const size_t len_runes = text->size;
  size_t trimmed_len = len_runes;
  const size_t len_pattern = pattern->size;
  /* TODO(conni2461): i feel this is wrong */
  if (len_pattern == 0 ||
      !isspace((unsigned char)pattern->data[len_pattern - 1])) {
    trimmed_len -= trailing_whitespaces(text);
  }
  if (len_pattern == 0) {
    return (fzf_result_t){(int32_t)trimmed_len, (int32_t)trimmed_len, 0, NULL};
  }
  size_t diff = trimmed_len - len_pattern;
  if (diff < 0) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }

  for (size_t idx = 0; idx < len_pattern; idx++) {
    char c = text->data[idx + diff];
    if (!case_sensitive) {
      c = (char)tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c != pattern->data[idx]) {
      return (fzf_result_t){-1, -1, 0, NULL};
    }
  }
  size_t start = trimmed_len - len_pattern;
  size_t end = trimmed_len;
  int32_t score = fzf_calculate_score(case_sensitive, normalize, text, pattern,
                                      start, end, false)
                      .score;
  return (fzf_result_t){(int32_t)start, (int32_t)end, score, NULL};
}

fzf_result_t fzf_suffix_match(bool case_sensitive, bool normalize,
                              const char *input, const char *pattern,
                              bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __suffix_match(case_sensitive, normalize, &input_wrap, &pattern_wrap,
                        with_pos, slab);
}

static fzf_result_t __equal_match(bool case_sensitive, bool normalize,
                                  fzf_string_t *text, fzf_string_t *pattern,
                                  bool withPos, fzf_slab_t *slab) {
  const size_t len_pattern = pattern->size;
  if (len_pattern == 0) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }

  size_t trimmed_len = leading_whitespaces(text);
  size_t trimmed_end_len = trailing_whitespaces(text);

  if ((text->size - trimmed_len - trimmed_end_len) != len_pattern) {
    return (fzf_result_t){-1, -1, 0, NULL};
  }

  bool match = true;
  if (normalize) {
    // TODO(conni2461): to rune
    for (size_t idx = 0; idx < len_pattern; idx++) {
      char pchar = pattern->data[idx];
      char c = text->data[trimmed_len + idx];
      if (!case_sensitive) {
        c = (char)tolower(c);
      }
      if (normalize_rune(c) != normalize_rune(pchar)) {
        match = false;
        break;
      }
    }
  } else {
    // TODO(conni2461): to rune
    for (size_t idx = 0; idx < len_pattern; idx++) {
      char pchar = pattern->data[idx];
      char c = text->data[trimmed_len + idx];
      if (!case_sensitive) {
        c = (char)tolower(c);
      }
      if (c != pchar) {
        match = false;
        break;
      }
    }
  }
  if (match) {
    return (fzf_result_t){
        (int32_t)trimmed_len, ((int32_t)trimmed_len + (int32_t)len_pattern),
        (score_match + bonus_boundary) * (int32_t)len_pattern +
            (bonus_first_char_multiplier - 1) * bonus_boundary,
        NULL};
  }
  return (fzf_result_t){-1, -1, 0, NULL};
}

fzf_result_t fzf_equal_match(bool case_sensitive, bool normalize,
                             const char *input, const char *pattern,
                             bool with_pos, fzf_slab_t *slab) {
  fzf_string_t input_wrap = {.data = input, .size = strlen(input)};
  fzf_string_t pattern_wrap = {.data = pattern, .size = strlen(pattern)};
  return __equal_match(case_sensitive, normalize, &input_wrap, &pattern_wrap,
                       with_pos, slab);
}

static void append_set(fzf_term_set_t *set, fzf_term_t value) {
  if (set->cap == 0) {
    set->cap = 1;
    set->ptr = (fzf_term_t *)malloc(sizeof(fzf_term_t));
  } else if (set->size + 1 > set->cap) {
    set->cap *= 2;
    set->ptr = realloc(set->ptr, sizeof(fzf_term_t) * set->cap);
  }
  set->ptr[set->size] = value;
  set->size++;
}

static void append_pattern(fzf_pattern_t *pattern, fzf_term_set_t *value) {
  if (pattern->cap == 0) {
    pattern->cap = 1;
    pattern->ptr = (fzf_term_set_t **)malloc(sizeof(fzf_term_set_t *));
  } else if (pattern->size + 1 > pattern->cap) {
    pattern->cap *= 2;
    pattern->ptr =
        realloc(pattern->ptr, sizeof(fzf_term_set_t *) * pattern->cap);
  }
  pattern->ptr[pattern->size] = value;
  pattern->size++;
}

static fzf_result_t fzf_call_alg(fzf_term_t *term, bool normalize,
                                 fzf_string_t *input, bool with_pos,
                                 fzf_slab_t *slab) {
  switch (term->typ) {
  case term_fuzzy:
    return __fuzzy_match_v2(term->case_sensitive, normalize, input,
                            (fzf_string_t *)term->text, with_pos, slab);
  case term_exact:
    return __exact_match_naive(term->case_sensitive, normalize, input,
                               (fzf_string_t *)term->text, with_pos, slab);
  case term_prefix:
    return __prefix_match(term->case_sensitive, normalize, input,
                          (fzf_string_t *)term->text, with_pos, slab);
  case term_suffix:
    return __suffix_match(term->case_sensitive, normalize, input,
                          (fzf_string_t *)term->text, with_pos, slab);
  case term_equal:
    return __equal_match(term->case_sensitive, normalize, input,
                         (fzf_string_t *)term->text, with_pos, slab);
  }
  return __fuzzy_match_v2(term->case_sensitive, normalize, input,
                          (fzf_string_t *)term->text, with_pos, slab);
}

// TODO(conni2461): REFACTOR
/* assumption (maybe i change that later)
 * - always v2 alg
 * - bool extended always true (thats the whole point of this isn't it)
 */
DLLEXPORT fzf_pattern_t *fzf_parse_pattern(fzf_case_types case_mode, bool normalize,
                                 char *pattern, bool fuzzy) {
  size_t pat_len = strlen(pattern);
  pattern = trim_left(pattern, &pat_len, ' ');
  while (has_suffix(pattern, pat_len, " ", 1) &&
         !has_suffix(pattern, pat_len, "\\ ", 2)) {
    pattern[pat_len - 1] = 0;
    pat_len--;
  }

  char *pattern_copy = str_replace(pattern, "\\ ", "\t");

  const char *delim = " ";
  char *ptr = strtok(pattern_copy, delim);

  fzf_pattern_t *pat_obj = (fzf_pattern_t *)malloc(sizeof(fzf_pattern_t));
  memset(pat_obj, 0, sizeof(*pat_obj));
  fzf_term_set_t *set = (fzf_term_set_t *)malloc(sizeof(fzf_term_set_t));
  memset(set, 0, sizeof(*set));

  bool switch_set = false;
  bool after_bar = false;
  while (ptr != NULL) {
    fzf_alg_types typ = term_fuzzy;
    bool inv = false;
    char *text = str_replace(ptr, "\t", " ");
    size_t len = strlen(text);
    char *og_str = text;
    char *lower_text = str_tolower(text, len);
    bool case_sensitive = case_mode == case_respect ||
                          (case_mode == case_smart && strcmp(text, lower_text));
    if (!case_sensitive) {
      free(text);
      text = lower_text;
      og_str = lower_text;
    } else {
      free(lower_text);
    }
    if (!fuzzy) {
      typ = term_exact;
    }
    if (set->size > 0 && !after_bar && strcmp(text, "|") == 0) {
      switch_set = false;
      after_bar = true;
      ptr = strtok(NULL, delim);
      free(og_str);
      continue;
    }
    after_bar = false;
    if (has_prefix(text, "!", 1)) {
      inv = true;
      typ = term_exact;
      text++;
      len--;
    }

    if (strcmp(text, "$") != 0 && has_suffix(text, len, "$", 1)) {
      typ = term_suffix;
      text[len - 1] = 0;
      len--;
    }

    if (has_prefix(text, "'", 1)) {
      if (fuzzy && !inv) {
        typ = term_exact;
        text++;
        len--;
      } else {
        typ = term_fuzzy;
        text++;
        len--;
      }
    } else if (has_prefix(text, "^", 1)) {
      if (typ == term_suffix) {
        typ = term_equal;
      } else {
        typ = term_prefix;
      }
      text++;
      len--;
    }

    if (len > 0) {
      if (switch_set) {
        append_pattern(pat_obj, set);
        set = (fzf_term_set_t *)malloc(sizeof(fzf_term_set_t));
        set->cap = 0;
        set->size = 0;
      }
      fzf_string_t *text_ptr = (fzf_string_t *)malloc(sizeof(fzf_string_t));
      text_ptr->data = text;
      text_ptr->size = len;
      append_set(set, (fzf_term_t){.typ = typ,
                                   .inv = inv,
                                   .ptr = og_str,
                                   .text = text_ptr,
                                   .case_sensitive = case_sensitive});
      switch_set = true;
    } else {
      free(og_str);
    }

    ptr = strtok(NULL, delim);
  }
  if (set->size > 0) {
    append_pattern(pat_obj, set);
  }
  bool only = true;
  for (size_t i = 0; i < pat_obj->size; i++) {
    fzf_term_set_t *term_set = pat_obj->ptr[i];
    if (term_set->size > 1) {
      only = false;
      break;
    }
    if (term_set->ptr[0].inv == false) {
      only = false;
      break;
    }
  }
  pat_obj->only_inv = only;
  free(pattern_copy);
  return pat_obj;
}

DLLEXPORT void fzf_free_pattern(fzf_pattern_t *pattern) {
  for (size_t i = 0; i < pattern->size; i++) {
    fzf_term_set_t *term_set = pattern->ptr[i];
    for (size_t j = 0; j < term_set->size; j++) {
      fzf_term_t *term = &term_set->ptr[j];
      free(term->ptr);
      free(term->text);
    }
    free(term_set->ptr);
    free(term_set);
  }
  free(pattern->ptr);
  free(pattern);
}

DLLEXPORT int32_t fzf_get_score(const char *text, fzf_pattern_t *pattern,
                      fzf_slab_t *slab) {
  fzf_string_t input = {.data = text, .size = strlen(text)};

  if (pattern->only_inv) {
    int final = 0;
    for (size_t i = 0; i < pattern->size; i++) {
      fzf_term_set_t *term_set = pattern->ptr[i];
      fzf_term_t *term = &term_set->ptr[0];
      final += fzf_call_alg(term, false, &input, false, slab).score;
    }
    return (final > 0) ? 0 : 1;
  }

  int32_t total_score = 0;
  for (size_t i = 0; i < pattern->size; i++) {
    fzf_term_set_t *term_set = pattern->ptr[i];
    int32_t current_score = 0;
    bool matched = false;
    for (size_t j = 0; j < term_set->size; j++) {
      fzf_term_t *term = &term_set->ptr[j];
      fzf_result_t res = fzf_call_alg(term, false, &input, false, slab);
      if (res.start >= 0) {
        if (term->inv) {
          continue;
        }
        current_score = res.score;
        matched = true;
      } else if (term->inv) {
        current_score = 0;
        matched = true;
      }
    }
    if (matched) {
      total_score += current_score;
    } else {
      total_score = 0;
      break;
    }
  }

  return total_score;
}

DLLEXPORT fzf_position_t *fzf_get_positions(const char *text, fzf_pattern_t *pattern,
                                  fzf_slab_t *slab) {
  fzf_string_t input = {.data = text, .size = strlen(text)};

  fzf_position_t *all_pos = pos_array(true, 1);

  for (size_t i = 0; i < pattern->size; i++) {
    fzf_term_set_t *term_set = pattern->ptr[i];
    fzf_result_t current_res = (fzf_result_t){0, 0, 0, NULL};
    bool matched = false;
    for (size_t j = 0; j < term_set->size; j++) {
      fzf_term_t *term = &term_set->ptr[j];
      fzf_result_t res = fzf_call_alg(term, false, &input, true, slab);
      if (res.start >= 0) {
        if (term->inv) {
          fzf_free_positions(res.pos);
          continue;
        }
        current_res = res;
        matched = true;
      } else if (term->inv) {
        matched = true;
      }
    }
    if (matched) {
      if (current_res.pos) {
        concat_pos(all_pos, current_res.pos);
        fzf_free_positions(current_res.pos);
      } else {
        int32_t diff = (current_res.end - current_res.start);
        if (diff > 0) {
          insert_pos(all_pos, (size_t)current_res.start,
                     (size_t)current_res.end);
        }
      }
    } else {
      free(all_pos->data);
      memset(all_pos, 0, sizeof(*all_pos));
      break;
    }
  }
  return all_pos;
}

DLLEXPORT void fzf_free_positions(fzf_position_t *pos) {
  if (pos) {
    if (pos->data) {
      free(pos->data);
    }
    free(pos);
  }
}

DLLEXPORT fzf_slab_t *fzf_make_slab(size_t size_16, size_t size_32) {
  fzf_slab_t *slab = (fzf_slab_t *)malloc(sizeof(fzf_slab_t));
  memset(slab, 0, sizeof(*slab));

  slab->I16.data = (int16_t *)malloc(size_16 * sizeof(int16_t));
  memset(slab->I16.data, 0, size_16 * sizeof(*slab->I16.data));
  slab->I16.cap = size_16;
  slab->I16.size = 0;
  slab->I16.allocated = true;

  slab->I32.data = (int32_t *)malloc(size_32 * sizeof(int32_t));
  memset(slab->I32.data, 0, size_32 * sizeof(*slab->I32.data));
  slab->I32.cap = size_32;
  slab->I32.size = 0;
  slab->I32.allocated = true;

  return slab;
}

DLLEXPORT fzf_slab_t *fzf_make_default_slab(void) {
  return fzf_make_slab(100 * 1024, 2048);
}

DLLEXPORT void fzf_free_slab(fzf_slab_t *slab) {
  if (slab) {
    free(slab->I16.data);
    free(slab->I32.data);
    free(slab);
  }
}
