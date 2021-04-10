#include "fzf.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

// TODO(conni2461): UNICODE HEADER
#define UNICODE_MAXASCII 0x7f

int32_t index_at(int32_t index, int32_t max, bool forward) {
  if (forward) {
    return index;
  }
  return max - index - 1;
}

int32_t *pos_array(bool with_pos, int32_t len) {
  if (with_pos) {
    int32_t *pos = malloc(len * sizeof(int32_t));
    return pos;
  }
  return NULL;
}

i16_t *alloc16(int32_t *offset, slab_t *slab, int32_t size) {
  if (slab != NULL && slab->I16.cap > *offset + size) {
    i16_t *slice = slice_i16(&slab->I16, *offset, (*offset) + size);
    offset = offset + size;
    return slice;
  }
  i16_t *ret = NULL;
  ret->data = malloc(size * sizeof(int16_t));
  ret->size = size;
  ret->cap = size;
  return ret;
}

i16_t *alloc16_no(int32_t offset, slab_t *slab, int32_t size) {
  return alloc16(&offset, slab, size);
}

i32_t *alloc32(int32_t *offset, slab_t *slab, int32_t size) {
  if (slab != NULL && slab->I32.cap > *offset + size) {
    i32_t *slice = slice_i32(&slab->I32, *offset, (*offset) + size);
    offset = offset + size;
    return slice;
  }
  i32_t *ret = NULL;
  ret->data = malloc(size * sizeof(int32_t));
  ret->size = size;
  ret->cap = size;
  return ret;
}

i32_t *alloc32_no(int32_t offset, slab_t *slab, int32_t size) {
  return alloc32(&offset, slab, size);
}

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

charClass char_class_of(rune ch) {
  if (ch <= UNICODE_MAXASCII) {
    return char_class_of_ascii(ch);
  }
  return char_class_of_non_ascii(ch);
}

int16_t bonus_for(charClass prev_class, charClass class) {
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

int16_t bonus_at(chars_t *input, int32_t idx) {
  if (idx == 0) {
    return bonus_boundary;
  }
  return bonus_for(char_class_of(input->slice.data[idx - 1]),
                   char_class_of(input->slice.data[idx]));
}

rune normalie_rune(rune r) {
  if (r < 0x00C0 || r > 0x2184) {
    return r;
  }
  // TODO(conni2461)
  /* rune n = normalized[r]; */
  /* if n > 0 { */
  /*   return n; */
  /* } */
  return r;
}

int32_t try_skip(chars_t *input, bool case_sensitive, byte b, int32_t from) {
  // byteArray := input.Bytes()[from:]
  // basically a slice from till end
  string_t *byte_array = slice_of_string_left(&input->slice, from);
  int32_t idx = index_byte(byte_array, b);
  if (idx == 0) {
    return from;
  }

  if (!case_sensitive && b >= 'a' && b <= 'z') {
    if (idx > 0) {
      string_t *tmp = slice_of_string_right(byte_array, idx);
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

/* TODO(conni2461): maybe i add debugv2 maybe not */

/* result_t *fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
 */
/*                          chars_t *input, rune *pattern, bool with_pos, */
/*                          slab_t *slab) { */
/*   int32_t M = strlen(pattern); */
/*   result_t ret; */
/*   if (M == 0) { */
/*     ret.start = 0; */
/*     ret.end = 0; */
/*     ret.score = 0; */
/*     return ret; // TODO(conni2461): positions */
/*   } */
/*   int32_t N = input->slice.size; */
/*   if (slab != NULL && N * M > slab->I16.cap) { */
/*     return fuzzy_match_v2(case_sensitive, normalize, forward, input, pattern,
 */
/*                           with_pos, slab); */
/*   } */

/*   int32_t idx = ascii_fuzzy_index(input, pattern, M, case_sensitive); */
/*   if (idx < 0) { */
/*     return init_result(0, 0, 0); */
/*   } */

/*   int16_t offset16 = 0; */
/*   int32_t offset32 = 0; */
/*   int16_t *H0; // alloc16(offset16, slab, N) */
/*   int16_t *C0; // alloc16(offset16, slab, N) */
/*   // Bonus point for each positions */
/*   int16_t *B; // alloc16(offset16, slab, N) */
/*   // The first occurrence of each character in the pattern */
/*   int32_t *F; // alloc32(offset32, slab, M) */
/*   // Rune array */
/*   int32_t *T; // alloc32(offset32, slab, N) */
/*   /1* input.CopyRunes(T) *1/ */

/*   // Phase 2. Calculate bonus for each point */
/*   int16_t max_score = 0; */
/*   int32_t max_score_pos = 0; */

/*   int32_t pidx = 0; */
/*   int32_t last_idx = 0; */

/*   rune pchar0 = pattern[0]; */
/*   rune pchar = pattern[0]; */
/*   int16_t prevH0 = 0; */
/*   int32_t prev_class = char_non_word; */
/*   bool in_gap = false; */

/*   /1* int32_t Tsub = T[idx:] *1/ */
/*   /1* H0sub, C0sub, Bsub := H0[idx:][:len(Tsub)], C0[idx:][:len(Tsub)], */
/*    * B[idx:][:len(Tsub)] *1/ */
/*   /1* TODO(conni2461): 381 *1/ */

/*   return ret; */
/* } */

// LETS GO BACKWARDS
int32_t calculate_score(bool case_sensitive, bool normalize, chars_t *text,
                        rune *pattern, int32_t sidx, int32_t eidx,
                        bool with_pos) {
  /* int32_t len_pattern = rune_len(pattern); */

  int32_t pidx = 0;
  int32_t score = 0;
  bool in_gap = false;
  int32_t consecutive = 0;
  int16_t first_bonus = 0;
  /* int32_t *pos = pos_array(with_pos, len_pattern); */
  int32_t prev_class = char_non_word;
  if (sidx > 0) {
    prev_class = char_class_of(text->slice.data[sidx - 1]);
  }
  for (int32_t idx = sidx; idx < eidx; idx++) {
    char c = text->slice.data[idx];
    int32_t class = char_class_of(c);
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = tolower(c);
    }
    if (normalize) {
      c = normalie_rune(c);
    }
    if (c == pattern[pidx]) {
      /* if (with_pos) { */
      /* TODO(conni2461): append(pos, idx) */
      /* } */
      score += score_match;
      int16_t bonus = bonus_for(prev_class, class);
      if (consecutive == 0) {
        first_bonus = bonus;
      } else {
        if (bonus == bonus_boundary) {
          first_bonus = bonus;
        }
        bonus = fmax(fmax(bonus, first_bonus), bonus_consecutive);
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
  return score;
}

result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        chars_t *text, rune *pattern, bool with_pos,
                        slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0};
  }
  if (ascii_fuzzy_index(text, pattern, len_pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0};
  }

  int32_t pidx = 0;
  int32_t sidx = -1;
  int32_t eidx = -1;

  int32_t len_runes = text->slice.size;

  for (int32_t idx = 0; idx < len_runes; idx++) {
    char c = text->slice.data[index_at(idx, len_runes, forward)];
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = tolower(c);
    }
    if (normalize) {
      c = normalie_rune(c);
    }
    char r = pattern[index_at(pidx, len_pattern, forward)];
    if (c == r) {
      if (sidx < 0) {
        sidx = idx;
      }
      /* TODO(conni2461): is this the same as `if pidx++; pidx == lenPattern` */
      pidx++;
      if (pidx == len_pattern) {
        eidx = idx + 1;
        break;
      }
    }
  }
  if (sidx >= 0 && eidx >= 0) {
    pidx--;
    for (int32_t idx = eidx - 1; idx >= sidx; idx--) {
      char c = text->slice.data[index_at(idx, len_runes, forward)];
      if (!case_sensitive) {
        /* TODO(conni2461): He does some unicode stuff here, investigate */
        c = tolower(c);
      }
      char r = pattern[index_at(pidx, len_pattern, forward)];
      if (c == r) {
        /* TODO(conni2461): is this the same as `if pidx++; pidx == lenPattern`
         */
        pidx--;
        if (pidx < 0) {
          sidx = idx;
          break;
        }
      }
    }
    if (!forward) {
      sidx = len_runes - eidx;
      eidx = len_runes - sidx;
    }

    // TODO(conni2461): positions?!?!?
    int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                    sidx, eidx, with_pos);
    return (result_t){sidx, eidx, score};
  }
  return (result_t){-1, -1, 0};
}

result_t exact_match_naive(bool case_sensitive, bool normalize, bool forward,
                           chars_t *text, rune *pattern, bool with_pos,
                           slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0};
  }

  int32_t len_runes = text->slice.size;
  if (len_runes < len_pattern) {
    return (result_t){-1, -1, 0};
  }
  if (ascii_fuzzy_index(text, pattern, len_pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0};
  }

  int32_t pidx = 0;
  int32_t best_pos = -1;
  int16_t bonus = 0;
  int16_t best_bonus = -1;
  for (int32_t idx = 0; idx < len_runes; idx++) {
    int32_t idx_ = index_at(idx, len_runes, forward);
    char c = text->slice.data[idx_];
    if (!case_sensitive) {
      /* TODO(conni2461): He does some unicode stuff here, investigate */
      c = tolower(c);
    }
    if (normalize) {
      c = normalie_rune(c);
    }
    int32_t pidx_ = index_at(pidx, len_pattern, forward);
    char r = pattern[pidx_];
    if (r == c) {
      if (pidx_ == 0) {
        bonus = bonus_at(text, idx_);
      }
      pidx++;
      if (pidx == len_pattern) {
        if (bonus > best_bonus) {
          best_pos = idx;
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
    int32_t sidx;
    int32_t eidx;
    if (forward) {
      sidx = best_pos - len_pattern + 1;
      eidx = best_pos + 1;
    } else {
      sidx = len_runes - (best_pos + 1);
      eidx = len_runes - (best_pos - len_pattern + 1);
    }
    int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                    sidx, eidx, false);
    return (result_t){sidx, eidx, score};
  }
  return (result_t){-1, -1, 0};
}

result_t prefix_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool with_pos,
                      slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0};
  }
  int32_t trimmed_len = 0;
  if (!is_space(pattern[0])) {
    trimmed_len = leading_whitespaces(text);
  }
  if (text->slice.size - trimmed_len < len_pattern) {
    return (result_t){-1, -1, 0};
  }
  for (int32_t idx = 0; idx < len_pattern; idx++) {
    rune r = pattern[idx];
    char c = text->slice.data[trimmed_len + idx];
    if (!case_sensitive) {
      c = tolower(c);
    }
    if (normalize) {
      c = normalie_rune(c);
    }
    if (c != r) {
      return (result_t){-1, -1, 0};
    }
  }
  int32_t score =
      calculate_score(case_sensitive, normalize, text, pattern, trimmed_len,
                      trimmed_len + len_pattern, false);
  return (result_t){trimmed_len, trimmed_len + len_pattern, score};
}
result_t suffix_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool with_pos,
                      slab_t *slab) {
  int32_t len_runes = text->slice.size;
  int32_t trimmed_len = len_runes;
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0 || !is_space(pattern[len_pattern] - 1)) {
    trimmed_len -= trailing_whitespaces(text);
  }
  if (len_pattern == 0) {
    return (result_t){trimmed_len, trimmed_len, 0};
  }
  int32_t diff = trimmed_len - len_pattern;
  if (diff < 0) {
    return (result_t){-1, -1, 0};
  }

  for (int32_t idx = 0; idx < len_pattern; idx++) {
    rune r = pattern[idx];
    char c = text->slice.data[idx + diff];
    if (!case_sensitive) {
      c = tolower(c);
    }
    if (normalize) {
      c = normalie_rune(c);
    }
    if (c != r) {
      return (result_t){-1, -1, 0};
    }
  }
  int32_t score =
      calculate_score(case_sensitive, normalize, text, pattern,
                      trimmed_len - len_pattern, trimmed_len, false);
  return (result_t){trimmed_len - len_pattern, trimmed_len, score};
}

result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                     chars_t *text, rune *pattern, bool withPos, slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){-1, -1, 0};
  }

  int32_t trimmed_len = 0;
  if (!is_space(pattern[0])) {
    trimmed_len = leading_whitespaces(text);
  }

  int32_t trimmed_end_len = 0;
  if (!is_space(pattern[len_pattern - 1])) {
    trimmed_end_len = trailing_whitespaces(text);
  }

  if ((text->slice.size - trimmed_len - trimmed_end_len) != len_pattern) {
    return (result_t){-1, -1, 0};
  }

  bool match = true;
  if (normalize) {
    // TODO(conni2461): to rune
    char *runes = text->slice.data;
    for (int32_t idx = 0; idx < len_pattern; idx++) {
      rune pchar = pattern[idx];
      rune c = runes[trimmed_len + idx];
      if (!case_sensitive) {
        c = tolower(c);
      }
      if (normalie_rune(c) != normalie_rune(pchar)) {
        match = false;
        break;
      }
    }
  } else {
    // TODO(conni2461): to rune
    char *runes = text->slice.data;

    for (int32_t idx = 0; idx < len_pattern; idx++) {
      rune pchar = pattern[idx];
      rune c = runes[trimmed_len + idx];
      if (!case_sensitive) {
        c = tolower(c);
      }
      if (c != pchar) {
        match = false;
        break;
      }
    }
  }
  if (match) {
    return (result_t){trimmed_len, trimmed_len + len_pattern,
                      (score_match + bonus_boundary) * len_pattern +
                          (bonus_first_char_multiplier - 1) * bonus_boundary};
  }
  return (result_t){-1, -1, 0};
}

// TODO(conni2461): tmp for testing, we need unit tests
// Or maybe that is the function that will combine everything above and return
// the score. We need a function like that for telescope/lua
int32_t get_match(bool case_sensitive, bool normalize, char *text,
                  char *pattern) {
  chars_t asdf = {.slice = {NULL},
                  .in_bytes = false,
                  .trim_length_known = false,
                  .index = 0};
  asdf.slice.data = text;
  asdf.slice.size = strlen(text);

  return fuzzy_match_v1(case_sensitive, normalize, false, &asdf, pattern, false,
                        NULL)
      .score;
}
