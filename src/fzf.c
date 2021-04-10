#include "fzf.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

/* result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward, */
/*                         chars_t *input, rune *pattern, bool with_pos, */
/*                         slab_t *slab) { */
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
/*     return fuzzy_match_v2(case_sensitive, normalize, forward, input, pattern, */
/*                           with_pos, slab); */
/*   } */

/*   int32_t idx = ascii_fuzzy_index(input, pattern, M, case_sensitive); */
/*   if (idx < 0) { */
/*     ret.start = 0; */
/*     ret.end = 0; */
/*     ret.score = 0; */
/*     return ret; // TODO(conni2461): positions */
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
/*   /1* H0sub, C0sub, Bsub := H0[idx:][:len(Tsub)], C0[idx:][:len(Tsub)], B[idx:][:len(Tsub)] *1/ */
/*   /1* TODO(conni2461): 381 *1/ */

/*   return ret; */
/* } */
