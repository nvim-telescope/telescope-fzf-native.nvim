#include "fzf.h"

#include <assert.h>
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

position_t *pos_array(bool with_pos, int32_t len) {
  if (with_pos) {
    printf("pos_array should never be called\n");
    position_t *pos = malloc(sizeof(position_t));
    pos->size = 0;
    pos->cap = len;
    pos->data = malloc(len * sizeof(int32_t));
    return pos;
  }
  return NULL;
}

void append_pos(position_t *pos, int32_t value) {
  assert(pos->size < pos->cap);
  pos->data[pos->size] = value;
  pos->size += 1;
}

i16_t *alloc16(int32_t *offset, slab_t *slab, int32_t size, bool *allocated) {
  if (slab != NULL && slab->I16.cap > *offset + size) {
    i16_t *ret = malloc(sizeof(i16_t));
    *allocated = false;
    i16_slice_t slice = slice_i16(slab->I16.data, *offset, (*offset) + size);
    ret->data = slice.data;
    ret->size = slice.size;
    ret->cap = slice.size;
    offset = offset + size;
    return ret;
  }
  i16_t *ret = malloc(sizeof(i16_t));
  *allocated = true;
  ret->data = malloc(size * sizeof(int16_t));
  ret->size = size;
  ret->cap = size;
  return ret;
}

i16_t *alloc16_no(int32_t offset, slab_t *slab, int32_t size, bool *allocated) {
  return alloc16(&offset, slab, size, allocated);
}

i32_t *alloc32(int32_t *offset, slab_t *slab, int32_t size, bool *allocated) {
  if (slab != NULL && slab->I32.cap > *offset + size) {
    i32_t *ret = malloc(sizeof(i32_t));
    *allocated = false;
    i32_slice_t slice = slice_i32(slab->I32.data, *offset, (*offset) + size);
    ret->data = slice.data;
    ret->size = slice.size;
    ret->cap = slice.size;
    offset = offset + size;
    return ret;
  }
  i32_t *ret = malloc(sizeof(i32_t));
  *allocated = true;
  ret->data = malloc(size * sizeof(int32_t));
  ret->size = size;
  ret->cap = size;
  return ret;
}

i32_t *alloc32_no(int32_t offset, slab_t *slab, int32_t size, bool *allocated) {
  return alloc32(&offset, slab, size, allocated);
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
  str_slice_t slice = slice_str(input->slice.data, from, input->slice.size);
  string_t byte_array = {.data = slice.data, .size = slice.size};
  int32_t idx = index_byte(&byte_array, b);
  if (idx == 0) {
    return from;
  }

  if (!case_sensitive && b >= 'a' && b <= 'z') {
    if (idx > 0) {
      str_slice_t tmp = slice_str_right(byte_array.data, idx);
      byte_array.data = tmp.data;
      byte_array.size = tmp.size;
    }
    int32_t uidx = index_byte(&byte_array, b - 32);
    if (uidx >= 0) {
      idx = uidx;
    }
  }
  if (idx < 0) {
    return -1;
  }

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

result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                        chars_t *input, rune *pattern, bool with_pos,
                        slab_t *slab) {
  const int32_t M = rune_len(pattern);
  if (M == 0) {
    return (result_t){0, 0, 0, pos_array(with_pos, M)};
  }
  const int32_t N = input->slice.size;
  if (slab != NULL && N * M > slab->I16.cap) {
    return fuzzy_match_v1(case_sensitive, normalize, forward, input, pattern,
                          with_pos, slab);
  }

  int32_t idx = ascii_fuzzy_index(input, pattern, M, case_sensitive);
  if (idx < 0) {
    return (result_t){-1, -1, 0, NULL};
  }

  int32_t offset16 = 0;
  int32_t offset32 = 0;
  bool allocated_H0 = false;
  bool allocated_C0 = false;
  i16_t *H0 = alloc16(&offset16, slab, N, &allocated_H0);
  i16_t *C0 = alloc16(&offset16, slab, N, &allocated_C0);
  // Bonus point for each positions
  bool allocated_B = false;
  i16_t *B = alloc16(&offset16, slab, N, &allocated_B);
  // The first occurrence of each character in the pattern
  bool allocated_F = false;
  i32_t *F = alloc32(&offset32, slab, M, &allocated_F);
  // Rune array
  bool allocated_T = false;
  i32_t *T = alloc32_no(offset32, slab, N, &allocated_T);
  copy_runes(input, T); // input.CopyRunes(T)

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

  i32_slice_t Tsub = slice_i32(T->data, idx, T->size); // T[idx:];
  i16_slice_t H0sub =
      slice_i16_right(slice_i16(H0->data, idx, H0->size).data, Tsub.size);
  i16_slice_t C0sub =
      slice_i16_right(slice_i16(C0->data, idx, C0->size).data, Tsub.size);
  i16_slice_t Bsub =
      slice_i16_right(slice_i16(B->data, idx, B->size).data, Tsub.size);

  for (int32_t off = 0; off < Tsub.size; off++) {
    charClass class;
    char c = Tsub.data[off];
    if (c <= UNICODE_MAXASCII) {
      class = char_class_of_ascii(c);
      if (!case_sensitive && class == char_upper) {
        c += 32;
      }
    } else {
      class = char_class_of_non_ascii(c);
      if (!case_sensitive && class == char_upper) {
        /* TODO(conni2461): unicode support */
        c = tolower(c);
      }
      if (normalize) {
        c = normalie_rune(c);
      }
    }

    Tsub.data[off] = c;
    int16_t bonus = bonus_for(prev_class, class);
    Bsub.data[off] = bonus;
    prev_class = class;
    if (c == pchar) {
      if (pidx < M) {
        F->data[pidx] = (int32_t)(idx + off);
        pidx++;
        pchar = pattern[min32(pidx, M - 1)];
      }
      last_idx = idx + off;
    }

    if (c == pchar0) {
      int32_t score = score_match + bonus * bonus_first_char_multiplier;
      H0sub.data[off] = score;
      C0sub.data[off] = 1;
      if (M == 1 && ((forward && score > max_score) ||
                     (!forward && score >= max_score))) {
        max_score = score;
        max_score_pos = idx + off;
        if (forward && bonus == bonus_boundary) {
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
      in_gap = false;
    }
    prevH0 = H0sub.data[off];
  }
  if (pidx != M) {
    free_alloc(T, allocated_T);
    free_alloc(F, allocated_F);
    free_alloc(B, allocated_B);
    free_alloc(C0, allocated_C0);
    free_alloc(H0, allocated_H0);
    return (result_t){-1, -1, 0, NULL};
  }
  if (M == 1) {
    free_alloc(T, allocated_T);
    free_alloc(F, allocated_F);
    free_alloc(B, allocated_B);
    free_alloc(C0, allocated_C0);
    free_alloc(H0, allocated_H0);
    result_t res = {max_score_pos, max_score_pos + 1, max_score, NULL};
    if (!with_pos) {
      return res;
    }
    position_t *pos = pos_array(with_pos, 1);
    append_pos(pos, max_score_pos);
    res.pos = pos;
    return res;
  }

  int32_t f0 = F->data[0];
  int32_t width = last_idx - f0 + 1;
  bool allocated_H = false;
  i16_t *H = alloc16(&offset16, slab, width * M, &allocated_H);
  {
    i16_slice_t H0_tmp_slice = slice_i16(H0->data, f0, last_idx + 1);
    copy_into_i16(H, &H0_tmp_slice);
  }

  bool allocated_C = false;
  i16_t *C = alloc16_no(offset16, slab, width * M, &allocated_C);
  {
    i16_slice_t C0_tmp_slice = slice_i16(C0->data, f0, last_idx + 1);
    copy_into_i16(C, &C0_tmp_slice);
  }

  i32_slice_t Fsub = slice_i32(F->data, 1, F->size);
  str_slice_t Psub = slice_str_right(slice_str(pattern, 1, M).data, Fsub.size);
  for (int32_t off = 0; off < Fsub.size; off++) {
    int32_t f = Fsub.data[off];
    pchar = Psub.data[off];
    pidx = off + 1;
    int32_t row = pidx * width;
    in_gap = false;
    Tsub = slice_i32(T->data, f, last_idx + 1);
    Bsub = slice_i16_right(slice_i16(B->data, f, B->size).data, Tsub.size);
    i16_slice_t Csub = slice_i16_right(
        slice_i16(C->data, row + f - f0, C->size).data, Tsub.size);
    i16_slice_t Cdiag = slice_i16_right(
        slice_i16(C->data, row + f - f0 - 1 - width, C->size).data, Tsub.size);
    i16_slice_t Hsub = slice_i16_right(
        slice_i16(H->data, row + f - f0, H->size).data, Tsub.size);
    i16_slice_t Hdiag = slice_i16_right(
        slice_i16(H->data, row + f - f0 - 1 - width, H->size).data, Tsub.size);
    i16_slice_t Hleft = slice_i16_right(
        slice_i16(H->data, row + f - f0 - 1, H->size).data, Tsub.size);
    Hleft.data[0] = 0;
    for (int32_t j = 0; j < Tsub.size; j++) {
      char c = Tsub.data[j];
      int32_t col = j + f;
      int16_t s1 = 0;
      int16_t s2 = 0;
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
                             B->data[col - ((int32_t)consecutive) + 1]));
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
      if (pidx == M - 1 && ((forward && score > max_score) ||
                            (!forward && score >= max_score))) {
        max_score = score;
        max_score_pos = col;
      }
      Hsub.data[j] = score;
    }
  }

  // TODO(conni2461): DEBUG
  /* if DEBUG { */
  /*   debugV2(T, pattern, F, lastIdx, H, C) */
  /* } */

  position_t *pos = pos_array(with_pos, M);
  int32_t j = f0;
  if (with_pos) {
    int32_t i = M - 1;
    int32_t j = max_score_pos;
    bool prefer_match = true;
    for (;;) {
      int32_t I = i * width;
      int32_t j0 = j - f0;
      int16_t s = H->data[I + j0];

      int16_t s1 = 0;
      int16_t s2 = 0;
      if (i > 0 && j >= F->data[i]) {
        s1 = H->data[I - width + j0 - 1];
      }
      if (j > F->data[i]) {
        s2 = H->data[I + j0 - 1];
      }

      if (s > s1 && (s > s2 || (s == s2 && prefer_match))) {
        append_pos(pos, j);
        if (i == 0) {
          break;
        }
        i--;
      }
      prefer_match = C->data[I + j0] > 1 || (I + width + j0 + 1 < C->size &&
                                             C->data[I + width + j0 + 1] > 0);
      j--;
    }
  }

  free_alloc(H, allocated_H);
  free_alloc(C, allocated_C);
  free_alloc(T, allocated_T);
  free_alloc(F, allocated_F);
  free_alloc(B, allocated_B);
  free_alloc(C0, allocated_C0);
  free_alloc(H0, allocated_H0);
  return (result_t){j, max_score_pos + 1, (int32_t)max_score, pos};
}

score_pos_tuple_t calculate_score(bool case_sensitive, bool normalize,
                                  chars_t *text, rune *pattern, int32_t sidx,
                                  int32_t eidx, bool with_pos) {
  int32_t len_pattern = rune_len(pattern);

  int32_t pidx = 0;
  int32_t score = 0;
  bool in_gap = false;
  int32_t consecutive = 0;
  int16_t first_bonus = 0;
  position_t *pos = pos_array(with_pos, len_pattern);
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

result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        chars_t *text, rune *pattern, bool with_pos,
                        slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern, len_pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0, NULL};
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

    score_pos_tuple_t tuple = calculate_score(case_sensitive, normalize, text,
                                              pattern, sidx, eidx, with_pos);
    return (result_t){sidx, eidx, tuple.score, tuple.pos};
  }
  return (result_t){-1, -1, 0, NULL};
}

result_t exact_match_naive(bool case_sensitive, bool normalize, bool forward,
                           chars_t *text, rune *pattern, bool with_pos,
                           slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }

  int32_t len_runes = text->slice.size;
  if (len_runes < len_pattern) {
    return (result_t){-1, -1, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern, len_pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0, NULL};
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
                                    sidx, eidx, false)
                        .score;
    return (result_t){sidx, eidx, score, NULL};
  }
  return (result_t){-1, -1, 0, NULL};
}

result_t prefix_match(bool case_sensitive, bool normalize, bool forward,
                      chars_t *text, rune *pattern, bool with_pos,
                      slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }
  int32_t trimmed_len = 0;
  if (!is_space(pattern[0])) {
    trimmed_len = leading_whitespaces(text);
  }
  if (text->slice.size - trimmed_len < len_pattern) {
    return (result_t){-1, -1, 0, NULL};
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
      return (result_t){-1, -1, 0, NULL};
    }
  }
  int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                  trimmed_len, trimmed_len + len_pattern, false)
                      .score;
  return (result_t){trimmed_len, trimmed_len + len_pattern, score, NULL};
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
    return (result_t){trimmed_len, trimmed_len, 0, NULL};
  }
  int32_t diff = trimmed_len - len_pattern;
  if (diff < 0) {
    return (result_t){-1, -1, 0, NULL};
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
      return (result_t){-1, -1, 0, NULL};
    }
  }
  int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                  trimmed_len - len_pattern, trimmed_len, false)
                      .score;
  return (result_t){trimmed_len - len_pattern, trimmed_len, score, NULL};
}

result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                     chars_t *text, rune *pattern, bool withPos, slab_t *slab) {
  int32_t len_pattern = rune_len(pattern);
  if (len_pattern == 0) {
    return (result_t){-1, -1, 0, NULL};
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
    return (result_t){-1, -1, 0, NULL};
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
                          (bonus_first_char_multiplier - 1) * bonus_boundary,
                      NULL};
  }
  return (result_t){-1, -1, 0, NULL};
}

void append_set(term_set_t *set, term_t value) {
  if (set->cap == 0) {
    set->cap = 1;
    set->ptr = malloc(sizeof(term_set_t));
  } else if (set->size + 1 > set->cap) {
    // I want to keep this set as thight as possible. This function should not
    // be called that often because it only happens inside the prompt
    // determination, which happens only once for each prompt.
    set->cap += 1;
    set->ptr = realloc(set->ptr, sizeof(term_t) * set->cap);
    assert(set->ptr != NULL);
    printf("resizing\n");
  }
  set->ptr[set->size] = value;
  set->size += 1;
}

void append_sets(term_set_sets_t *set, term_set_t *value) {
  if (set->cap == 0) {
    set->cap = 1;
    set->ptr = malloc(sizeof(term_set_t));
  } else if (set->size + 1 > set->cap) {
    // Same reason as append_set. Need to think about this more
    set->cap += 1;
    set->ptr = realloc(set->ptr, sizeof(term_set_t) * set->cap);
    assert(set->ptr != NULL);
  }
  set->ptr[set->size] = *value;
  set->size += 1;
}

// TODO determine forward
/* forward := true */
/* for _, cri := range opts.Criteria[1:] { */
/*   if cri == byEnd { */
/*     forward = false */
/*     break */
/*   } */
/*   if cri == byBegin { */
/*     break */
/*   } */
/* } */

/* assumption (maybe i change that later)
 * - bool fuzzy always true
 * - always v2 alg
 * - bool extended always true (thats the whole point of this isn't it)
 *
 * TODOs:
 * - case (smart, ignore, respect) currently only ignore or respect
 */
term_set_sets_t *build_pattern_fun(bool case_sensitive, bool normalize,
                                   char *pattern) {
  int32_t len = strlen(pattern);
  char *as_string;
  as_string = trim_left(pattern, len, ' ', &len);
  while (has_suffix(as_string, len, " ", 1) &&
         !has_suffix(as_string, len, "\\ ", 2)) {
    as_string[len - 1] = 0;
    len--;
  }
  /* TODO(conni2461): We need to do more stuff here, e.g. we can't sort until we
   * have at least one not inverse */
  return parse_terms(case_sensitive, normalize, as_string);
}

term_set_sets_t *parse_terms(bool case_sensitive, bool normalize,
                             char *pattern) {
  pattern = str_replace(pattern, "\\ ", "\t");
  const char *delim = " ";
  char *ptr = strtok(pattern, delim);

  term_set_sets_t *sets = malloc(sizeof(term_set_sets_t));
  sets->size = 0;
  sets->cap = 0;
  term_set_t *set = malloc(sizeof(term_set_t));
  set->size = 0;
  set->cap = 0;

  bool switch_set = false;
  bool after_bar = false;
  while (ptr != NULL) {
    alg_types typ = term_fuzzy;
    bool inv = false;
    ptr = str_replace(ptr, "\t", " ");

    int32_t len = strlen(ptr);
    char *text = malloc(len * sizeof(char) + 1);
    strcpy(text, ptr);
    /* char *lower_text = str_tolower(text); */
    /* caseSensitive = caseMode == CaseRespect || */
    /*                   caseMode == CaseSmart &&text != lowerText; */
    /* normalizeTerm := normalize && */
    /*   lowerText == string(algo.NormalizeRunes([]rune(lowerText))); */
    if (!case_sensitive) {
      str_tolower(text);
    }
    if (set->size > 0 && !after_bar && strcmp(text, "|") == 0) {
      switch_set = false;
      after_bar = true;
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
      if (!inv) { // usually if fuzzy && but we assume always fuzzy
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
        append_sets(sets, set);
        set = malloc(sizeof(term_set_t));
        set->cap = 0;
        set->size = 0;
      }
      /* if normalizeTerm { */
      /*   textRunes = algo.NormalizeRunes(textRunes) */
      /* } */
      // TODO(conni2461):
      append_set(set, (term_t){.typ = typ,
                               .inv = inv,
                               .text = text, // DO I NEED TO COPY HERE?!
                               .case_sensitive = case_sensitive, // not correct
                               .normalize = normalize});         // not correct
      switch_set = true;
    }

    ptr = strtok(NULL, delim);
  }
  if (set->size > 0) {
    append_sets(sets, set);
  }
  return sets;
}

int32_t get_match_bad(bool case_sensitive, bool normalize, char *text,
                      char *pattern) {
  chars_t input = {.slice = {NULL},
                   .in_bytes = false,
                   .trim_length_known = false,
                   .index = 0};
  input.slice.data = text;
  input.slice.size = strlen(text);

  // 200KB * 32 = 12.8MB, 8KB * 32 = 256KB
  /* slab_t *slab = make_slab(100 * 1024, 2048); */
  slab_t *slab = NULL;

  term_set_sets_t *set = build_pattern_fun(case_sensitive, normalize, pattern);
  int32_t total_score = 0;
  for (int i = 0; i < set->size; i++) {
    term_set_t *term_set = &set->ptr[i];
    int32_t current_score = 0;
    for (int j = 0; j < term_set->size; j++) {
      term_t *term = &term_set->ptr[j];
      if (term->inv) {
        continue;
      }
      switch (term->typ) {
      case term_fuzzy:
        current_score += fuzzy_match_v2(term->case_sensitive, term->normalize,
                                        true, &input, term->text, false, slab)
                             .score;
        break;
      case term_exact:
        current_score +=
            exact_match_naive(term->case_sensitive, term->normalize, true,
                              &input, term->text, false, slab)
                .score;
        break;
      case term_prefix:
        current_score += prefix_match(term->case_sensitive, term->normalize,
                                      true, &input, term->text, false, slab)
                             .score;
        break;
      case term_suffix:
        current_score += suffix_match(term->case_sensitive, term->normalize,
                                      true, &input, term->text, false, slab)
                             .score;
        break;
      case term_equal:
        current_score += equal_match(term->case_sensitive, term->normalize,
                                     true, &input, term->text, false, slab)
                             .score;
        break;
      }
    }
    total_score += current_score;
  }
  return total_score;
}

slab_t *make_slab(int32_t size_16, int32_t size_32) {
  slab_t *slab = malloc(sizeof(slab_t));
  slab->I16.data = malloc(size_16 * sizeof(int16_t));
  slab->I16.cap = size_16;
  slab->I16.size = 0;

  slab->I32.data = malloc(size_16 * sizeof(int16_t));
  slab->I32.cap = size_16;
  slab->I32.size = 0;

  return slab;
}
