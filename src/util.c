#include "util.h"

// Helpers for i16_t
i16_t *slice_i16(i16_t *input, int32_t from, int32_t to) {
  i16_t *ret = NULL;
  ret->size = to - from;
  ret->cap = to - from;
  ret->data = malloc(ret->size * sizeof(int16_t));
  for (int32_t i = from; i < to; i++) {
    ret->data[i] = input->data[i];
  }

  return ret;
}

i16_t *slice_i16_left(i16_t *input, int32_t from) {
  return slice_i16(input, from, input->size);
}

i16_t *slice_i16_right(i16_t *input, int32_t to) {
  return slice_i16(input, 0, to);
}

// Helpers for i32_t
i32_t *slice_i32(i32_t *input, int32_t from, int32_t to) {
  i32_t *ret = NULL;
  ret->size = to - from;
  ret->cap = to - from;
  ret->data = malloc(ret->size * sizeof(int16_t));
  for (int32_t i = from; i < to; i++) {
    ret->data[i] = input->data[i];
  }

  return ret;
}

i32_t *slice_i32_left(i32_t *input, int32_t from) {
  return slice_i32(input, from, input->size);
}

i32_t *slice_i32_right(i32_t *input, int32_t to) {
  return slice_i32(input, 0, to);
}

// helpers for string_t
string_t *slice_of_string(string_t *input, int32_t from, int32_t to) {
  string_t *ret = NULL;
  ret->size = to - from;
  /* TODO(conni2461): IF you change it to rune you have to have rune here */
  ret->data = malloc(ret->size * sizeof(char));
  for (int32_t i = from; i < to; i++) {
    ret->data[i] = input->data[i];
  }

  return ret;
}

string_t *slice_of_string_left(string_t *input, int32_t from) {
  return slice_of_string(input, from, input->size);
}

string_t *slice_of_string_right(string_t *input, int32_t to) {
  return slice_of_string(input, 0, to);
}

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
