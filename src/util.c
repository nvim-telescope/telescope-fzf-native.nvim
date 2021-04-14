#include "util.h"

#include <ctype.h>
#include <string.h>

#define slice_impl(name, type)                                                 \
  name##_slice_t slice_##name(type *input, int32_t from, int32_t to) {         \
    return (name##_slice_t){.data = input + from, .size = to - from};          \
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

int32_t leading_whitespaces(string_t *str) {
  int32_t whitespaces = 0;
  for (int32_t i = 0; i < str->size; i++) {
    if (!is_space(str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

int32_t trailing_whitespaces(string_t *str) {
  int32_t whitespaces = 0;
  for (int32_t i = str->size - 1; i >= 0; i--) {
    if (!is_space(str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

void copy_runes(string_t *str, i32_t *destination) {
  for (int i = 0; i < destination->size; i++) {
    destination->data[i] = str->data[i];
  }
}

void copy_into_i16(i16_t *dest, i16_slice_t *src) {
  for (int i = 0; i < src->size; i++) {
    dest->data[i] = src->data[i];
  }
}

// char* helpers
char *trim_left(char *str, int32_t len, char trim, int32_t *new_len) {
  *new_len = len;
  for (int32_t i = 0; i < len; i++) {
    if (str[0] == trim) {
      (*new_len)--;
      str++;
    } else {
      break;
    }
  }
  return str;
}

bool has_prefix(char *str, char *prefix, int32_t prefix_len) {
  return strncmp(prefix, str, prefix_len) == 0;
}

bool has_suffix(char *str, int32_t len, char *suffix, int32_t suffix_len) {
  return len >= suffix_len &&
         strcmp(slice_str(str, len - suffix_len, len).data, suffix) == 0;
}

char *str_replace(char *orig, char *rep, char *with) {
  char *result;
  char *ins;
  char *tmp;
  int len_rep;
  int len_with;
  int len_front;
  int count;

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

  tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

  if (!result) {
    return NULL;
  }

  while (count--) {
    ins = strstr(orig, rep);
    len_front = ins - orig;
    tmp = strncpy(tmp, orig, len_front) + len_front;
    tmp = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

void str_tolower(char *str) {
  for (int i = 0; str[i]; i++) {
    str[i] = tolower(str[i]);
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
