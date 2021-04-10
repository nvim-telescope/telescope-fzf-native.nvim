#include <stdbool.h>
#include <stdlib.h>

// TODO(conni2461): WE NEED unicode support
typedef char rune; // ADDITIONAL 32bit char

typedef struct {
  int16_t *data;
  size_t size;
  size_t cap;
} i16_t;

typedef struct {
  int32_t *data;
  size_t size;
  size_t cap;
} i32_t;

typedef struct {
  char *data; // byte or rune, think about this
  size_t size;
} string_t;

typedef struct {
  string_t slice;
  bool in_bytes;
  bool trim_length_known;
  /* u_int16_t trim_length; */

  int32_t index;
} chars_t;

// Helpers for i16_t
i16_t *slice_i16(i16_t *input, int32_t from, int32_t to);
i16_t *slice_i16_left(i16_t *input, int32_t from);
i16_t *slice_i16_right(i16_t *input, int32_t to);

// helpers for i32_t
i32_t *slice_i32(i32_t *input, int32_t from, int32_t to);
i32_t *slice_i32_left(i32_t *input, int32_t from);
i32_t *slice_i32_right(i32_t *input, int32_t to);

// helpers for string_t
string_t *slice_of_string(string_t *input, int32_t from, int32_t to);
string_t *slice_of_string_left(string_t *input, int32_t from);
string_t *slice_of_string_right(string_t *input, int32_t to);
int32_t index_byte(string_t *string, char b);

// chars_t helpers
int32_t rune_len(rune *arr);
bool is_space(char ch);
int32_t leading_whitespaces(chars_t *chars);
int32_t trailing_whitespaces(chars_t *chars);
