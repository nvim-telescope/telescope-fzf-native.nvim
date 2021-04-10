#include "util.h"

#define slice_impl(name, type)                                                 \
  name##_slice_t slice_##name(type *input, int32_t from, int32_t to) {         \
    return (name##_slice_t){                                                   \
        .data = input + from, .from = from, .to = to, .size = to - from};      \
  }                                                                            \
  name##_slice_t slice_##name##_right(type *input, int32_t to) {               \
    return slice_##name(input, 0, to);                                         \
  }

slice_impl(i16, int16_t);
slice_impl(i32, int32_t);
slice_impl(str, char);

int32_t index_byte(string_t *string, char b) {
  for (int i = 0; i < string->size; i++) {
    if (string->data[i] == b) {
      return i;
    }
  }
  return -1;
}

// char_t helpers
int32_t rune_len(rune *arr) {
  int32_t len = 0;
  while (arr[len] != '\0') {
    len++;
  }
  return len;
}

bool is_space(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n';
}

int32_t leading_whitespaces(chars_t *chars) {
  int32_t whitespaces = 0;
  for (int32_t i = 0; i < chars->slice.size; i++) {
    if (!is_space(chars->slice.data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

int32_t trailing_whitespaces(chars_t *chars) {
  int32_t whitespaces = 0;
  for (int32_t i = chars->slice.size - 1; i >= 0; i--) {
    if (!is_space(chars->slice.data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

void copy_runes(chars_t *chars, i32_t *destination) {
  for (int i = 0; i < destination->size; i++) {
    destination->data[i] = chars->slice.data[i];
  }
}

void copy_into_i16(i16_t *dest, i16_slice_t *src) {
  for (int i = 0; i < src->size; i++) {
    dest->data[i] = src->data[i];
  }
}

// min + max
int16_t min16(int16_t a, int16_t b) {
  if (a < b)
    return a;
  return b;
}

int16_t max16(int16_t a, int16_t b) {
  if (a > b)
    return a;
  return b;
}

int32_t min32(int32_t a, int32_t b) {
  if (a < b)
    return a;
  return b;
}

int32_t max32(int32_t a, int32_t b) {
  if (a > b)
    return a;
  return b;
}
