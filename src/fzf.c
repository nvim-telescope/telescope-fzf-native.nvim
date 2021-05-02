#include "fzf.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

// TODO(conni2461): UNICODE HEADER
#define UNICODE_MAXASCII 0x7f

typedef unsigned char char_u;

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

gen_slice(i16, int16_t);
gen_slice(i32, int32_t);
#undef gen_slice
#undef gen_simple_slice

/* TODO(conni2461): additional types (utf8) */
typedef int16_t char_class;

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

static int32_t index_byte(i32_slice_t *slice, int32_t cp) {
  for (size_t i = 0; i < slice->size; i++) {
    if (slice->data[i] == cp) {
      return (int32_t)i;
    }
  }
  return -1;
}

static size_t leading_whitespaces(utf8str_t *str) {
  size_t whitespaces = 0;
  for (size_t i = 0; i < str->len; i++) {
    if (!isspace(str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

static size_t trailing_whitespaces(utf8str_t *str) {
  size_t whitespaces = 0;
  for (size_t i = str->len - 1; i >= 0; i--) {
    if (!isspace(str->data[i])) {
      break;
    }
    whitespaces++;
  }
  return whitespaces;
}

static void copy_runes(const utf8str_t *src, i32_t *destination) {
  for (size_t i = 0; i < src->len; i++) {
    destination->data[i] = src->data[i];
  }
}

static void copy_into_i16(const i16_slice_t *src, i16_t *dest) {
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

static bool has_prefix(const char *str, char *prefix, size_t prefix_len) {
  return strncmp(prefix, str, prefix_len) == 0;
}

static bool has_suffix(const char *str, size_t len, char *suffix,
                       size_t suffix_len) {
  return len >= suffix_len &&
         strncmp(str + (len - suffix_len), suffix, suffix_len) == 0;
}

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

static char *str_tolower(char *str, size_t size) {
  char *lower_str = (char *)malloc((size + 1) * sizeof(char));
  for (size_t i = 0; i < size; i++) {
    lower_str[i] = (char)tolower(str[i]);
  }
  lower_str[size] = '\0';
  return lower_str;
}

static int32_t utf8_tolower(int32_t cp) {
  if (((0x0041 <= cp) && (0x005a >= cp)) ||
      ((0x00c0 <= cp) && (0x00d6 >= cp)) ||
      ((0x00d8 <= cp) && (0x00de >= cp)) ||
      ((0x0391 <= cp) && (0x03a1 >= cp)) ||
      ((0x03a3 <= cp) && (0x03ab >= cp)) ||
      ((0x0410 <= cp) && (0x042f >= cp))) {
    cp += 32;
  } else if ((0x0400 <= cp) && (0x040f >= cp)) {
    cp += 80;
  } else if (((0x0100 <= cp) && (0x012f >= cp)) ||
             ((0x0132 <= cp) && (0x0137 >= cp)) ||
             ((0x014a <= cp) && (0x0177 >= cp)) ||
             ((0x0182 <= cp) && (0x0185 >= cp)) ||
             ((0x01a0 <= cp) && (0x01a5 >= cp)) ||
             ((0x01de <= cp) && (0x01ef >= cp)) ||
             ((0x01f8 <= cp) && (0x021f >= cp)) ||
             ((0x0222 <= cp) && (0x0233 >= cp)) ||
             ((0x0246 <= cp) && (0x024f >= cp)) ||
             ((0x03d8 <= cp) && (0x03ef >= cp)) ||
             ((0x0460 <= cp) && (0x0481 >= cp)) ||
             ((0x048a <= cp) && (0x04ff >= cp))) {
    cp |= 0x1;
  } else if (((0x0139 <= cp) && (0x0148 >= cp)) ||
             ((0x0179 <= cp) && (0x017e >= cp)) ||
             ((0x01af <= cp) && (0x01b0 >= cp)) ||
             ((0x01b3 <= cp) && (0x01b6 >= cp)) ||
             ((0x01cd <= cp) && (0x01dc >= cp))) {
    cp += 1;
    cp &= ~0x1;
  } else {
    switch (cp) {
    case 0x0178: return 0x00ff;
    case 0x0243: return 0x0180;
    case 0x018e: return 0x01dd;
    case 0x023d: return 0x019a;
    case 0x0220: return 0x019e;
    case 0x01b7: return 0x0292;
    case 0x01c4: return 0x01c6;
    case 0x01c7: return 0x01c9;
    case 0x01ca: return 0x01cc;
    case 0x01f1: return 0x01f3;
    case 0x01f7: return 0x01bf;
    case 0x0187: return 0x0188;
    case 0x018b: return 0x018c;
    case 0x0191: return 0x0192;
    case 0x0198: return 0x0199;
    case 0x01a7: return 0x01a8;
    case 0x01ac: return 0x01ad;
    case 0x01af: return 0x01b0;
    case 0x01b8: return 0x01b9;
    case 0x01bc: return 0x01bd;
    case 0x01f4: return 0x01f5;
    case 0x023b: return 0x023c;
    case 0x0241: return 0x0242;
    case 0x03fd: return 0x037b;
    case 0x03fe: return 0x037c;
    case 0x03ff: return 0x037d;
    case 0x037f: return 0x03f3;
    case 0x0386: return 0x03ac;
    case 0x0388: return 0x03ad;
    case 0x0389: return 0x03ae;
    case 0x038a: return 0x03af;
    case 0x038c: return 0x03cc;
    case 0x038e: return 0x03cd;
    case 0x038f: return 0x03ce;
    case 0x0370: return 0x0371;
    case 0x0372: return 0x0373;
    case 0x0376: return 0x0377;
    case 0x03f4: return 0x03b8;
    case 0x03cf: return 0x03d7;
    case 0x03f9: return 0x03f2;
    case 0x03f7: return 0x03f8;
    case 0x03fa: return 0x03fb;
    default: break;
    }
  }

  return cp;
}

int32_t utf8_toupper(int32_t cp) {
  if (((0x0061 <= cp) && (0x007a >= cp)) ||
      ((0x00e0 <= cp) && (0x00f6 >= cp)) ||
      ((0x00f8 <= cp) && (0x00fe >= cp)) ||
      ((0x03b1 <= cp) && (0x03c1 >= cp)) ||
      ((0x03c3 <= cp) && (0x03cb >= cp)) ||
      ((0x0430 <= cp) && (0x044f >= cp))) {
    cp -= 32;
  } else if ((0x0450 <= cp) && (0x045f >= cp)) {
    cp -= 80;
  } else if (((0x0100 <= cp) && (0x012f >= cp)) ||
             ((0x0132 <= cp) && (0x0137 >= cp)) ||
             ((0x014a <= cp) && (0x0177 >= cp)) ||
             ((0x0182 <= cp) && (0x0185 >= cp)) ||
             ((0x01a0 <= cp) && (0x01a5 >= cp)) ||
             ((0x01de <= cp) && (0x01ef >= cp)) ||
             ((0x01f8 <= cp) && (0x021f >= cp)) ||
             ((0x0222 <= cp) && (0x0233 >= cp)) ||
             ((0x0246 <= cp) && (0x024f >= cp)) ||
             ((0x03d8 <= cp) && (0x03ef >= cp)) ||
             ((0x0460 <= cp) && (0x0481 >= cp)) ||
             ((0x048a <= cp) && (0x04ff >= cp))) {
    cp &= ~0x1;
  } else if (((0x0139 <= cp) && (0x0148 >= cp)) ||
             ((0x0179 <= cp) && (0x017e >= cp)) ||
             ((0x01af <= cp) && (0x01b0 >= cp)) ||
             ((0x01b3 <= cp) && (0x01b6 >= cp)) ||
             ((0x01cd <= cp) && (0x01dc >= cp))) {
    cp -= 1;
    cp |= 0x1;
  } else {
    switch (cp) {
    default: break;
    case 0x00ff: return 0x0178;
    case 0x0180: return 0x0243;
    case 0x01dd: return 0x018e;
    case 0x019a: return 0x023d;
    case 0x019e: return 0x0220;
    case 0x0292: return 0x01b7;
    case 0x01c6: return 0x01c4;
    case 0x01c9: return 0x01c7;
    case 0x01cc: return 0x01ca;
    case 0x01f3: return 0x01f1;
    case 0x01bf: return 0x01f7;
    case 0x0188: return 0x0187;
    case 0x018c: return 0x018b;
    case 0x0192: return 0x0191;
    case 0x0199: return 0x0198;
    case 0x01a8: return 0x01a7;
    case 0x01ad: return 0x01ac;
    case 0x01b0: return 0x01af;
    case 0x01b9: return 0x01b8;
    case 0x01bd: return 0x01bc;
    case 0x01f5: return 0x01f4;
    case 0x023c: return 0x023b;
    case 0x0242: return 0x0241;
    case 0x037b: return 0x03fd;
    case 0x037c: return 0x03fe;
    case 0x037d: return 0x03ff;
    case 0x03f3: return 0x037f;
    case 0x03ac: return 0x0386;
    case 0x03ad: return 0x0388;
    case 0x03ae: return 0x0389;
    case 0x03af: return 0x038a;
    case 0x03cc: return 0x038c;
    case 0x03cd: return 0x038e;
    case 0x03ce: return 0x038f;
    case 0x0371: return 0x0370;
    case 0x0373: return 0x0372;
    case 0x0377: return 0x0376;
    case 0x03d1: return 0x0398;
    case 0x03d7: return 0x03cf;
    case 0x03f2: return 0x03f9;
    case 0x03f8: return 0x03f7;
    case 0x03fb: return 0x03fa;
    }
  }

  return cp;
}

static bool utf8_islower(int32_t chr) {
  return chr != utf8_toupper(chr);
}

static bool utf8_isupper(int32_t chr) {
  return chr != utf8_toupper(chr);
}

static size_t utf8_len(const void *str) {
  const char_u *s = (const char_u *)str;
  const char_u *t = s;
  size_t length = 0;

  while ((size_t)(s - t) < SIZE_MAX && '\0' != *s) {
    if (0xf0 == (0xf8 & *s)) {
      // 4-byte utf8 code point (began with 0b11110xxx)
      s += 4;
    } else if (0xe0 == (0xf0 & *s)) {
      // 3-byte utf8 code point (began with 0b1110xxxx)
      s += 3;
    } else if (0xc0 == (0xe0 & *s)) {
      // 2-byte utf8 code point (began with 0b110xxxxx)
      s += 2;
    } else {
      // 1-byte ascii (began with 0b0xxxxxxx)
      s += 1;
    }
    length++;
  }

  if ((size_t)(s - t) > SIZE_MAX) {
    length--;
  }
  return length;
}

utf8str_t into_utf8(const char *str) {
  const char_u *s = (const char_u *)str;
  const char_u *t = s;
  size_t len = utf8_len(s), i = 0;
  int32_t *res = (int32_t *)malloc(len * sizeof(int32_t));

  while ((size_t)(s - t) < SIZE_MAX && '\0' != *s) {
    if (0xf0 == (0xf8 & *s)) {
      // 4-byte utf8 code point (began with 0b11110xxx)
      res[i] = ((0x07 & s[0]) << 18) | ((0x3f & s[1]) << 12) |
               ((0x3f & s[2]) << 6) | (0x3f & s[3]);
      s += 4;
    } else if (0xe0 == (0xf0 & *s)) {
      // 3-byte utf8 code point (began with 0b1110xxxx)
      res[i] = ((0x0f & s[0]) << 12) | ((0x3f & s[1]) << 6) | (0x3f & s[2]);
      s += 3;
    } else if (0xc0 == (0xe0 & *s)) {
      // 2-byte utf8 code point (began with 0b110xxxxx)
      res[i] = ((0x1f & s[0]) << 6) | (0x3f & s[1]);
      s += 2;
    } else {
      // 1-byte ascii (began with 0b0xxxxxxx)
      res[i] = s[0];
      s += 1;
    }
    i++;
  }

  return (utf8str_t){.data = res, .len = len};
}

void free_utfstr(utf8str_t *str) {
  free(str->data);
}

static int32_t normalize(int32_t cp) {
  switch (cp) {
  case 0x00E1: return 'a';
  case 0x0103: return 'a';
  case 0x01CE: return 'a';
  case 0x00E2: return 'a';
  case 0x00E4: return 'a';
  case 0x0227: return 'a';
  case 0x1EA1: return 'a';
  case 0x0201: return 'a';
  case 0x00E0: return 'a';
  case 0x1EA3: return 'a';
  case 0x0203: return 'a';
  case 0x0101: return 'a';
  case 0x0105: return 'a';
  case 0x1E9A: return 'a';
  case 0x00E5: return 'a';
  case 0x1E01: return 'a';
  case 0x00E3: return 'a';
  case 0x0363: return 'a';
  case 0x0250: return 'a';
  case 0x1E03: return 'b';
  case 0x1E05: return 'b';
  case 0x0253: return 'b';
  case 0x1E07: return 'b';
  case 0x0180: return 'b';
  case 0x0183: return 'b';
  case 0x0107: return 'c';
  case 0x010D: return 'c';
  case 0x00E7: return 'c';
  case 0x0109: return 'c';
  case 0x0255: return 'c';
  case 0x010B: return 'c';
  case 0x0188: return 'c';
  case 0x023C: return 'c';
  case 0x0368: return 'c';
  case 0x0297: return 'c';
  case 0x2184: return 'c';
  case 0x010F: return 'd';
  case 0x1E11: return 'd';
  case 0x1E13: return 'd';
  case 0x0221: return 'd';
  case 0x1E0B: return 'd';
  case 0x1E0D: return 'd';
  case 0x0257: return 'd';
  case 0x1E0F: return 'd';
  case 0x0111: return 'd';
  case 0x0256: return 'd';
  case 0x018C: return 'd';
  case 0x0369: return 'd';
  case 0x00E9: return 'e';
  case 0x0115: return 'e';
  case 0x011B: return 'e';
  case 0x0229: return 'e';
  case 0x1E19: return 'e';
  case 0x00EA: return 'e';
  case 0x00EB: return 'e';
  case 0x0117: return 'e';
  case 0x1EB9: return 'e';
  case 0x0205: return 'e';
  case 0x00E8: return 'e';
  case 0x1EBB: return 'e';
  case 0x025D: return 'e';
  case 0x0207: return 'e';
  case 0x0113: return 'e';
  case 0x0119: return 'e';
  case 0x0247: return 'e';
  case 0x1E1B: return 'e';
  case 0x1EBD: return 'e';
  case 0x0364: return 'e';
  case 0x029A: return 'e';
  case 0x025E: return 'e';
  case 0x025B: return 'e';
  case 0x0258: return 'e';
  case 0x025C: return 'e';
  case 0x01DD: return 'e';
  case 0x1D08: return 'e';
  case 0x1E1F: return 'f';
  case 0x0192: return 'f';
  case 0x01F5: return 'g';
  case 0x011F: return 'g';
  case 0x01E7: return 'g';
  case 0x0123: return 'g';
  case 0x011D: return 'g';
  case 0x0121: return 'g';
  case 0x0260: return 'g';
  case 0x1E21: return 'g';
  case 0x01E5: return 'g';
  case 0x0261: return 'g';
  case 0x1E2B: return 'h';
  case 0x021F: return 'h';
  case 0x1E29: return 'h';
  case 0x0125: return 'h';
  case 0x1E27: return 'h';
  case 0x1E23: return 'h';
  case 0x1E25: return 'h';
  case 0x02AE: return 'h';
  case 0x0266: return 'h';
  case 0x1E96: return 'h';
  case 0x0127: return 'h';
  case 0x036A: return 'h';
  case 0x0265: return 'h';
  case 0x2095: return 'h';
  case 0x00ED: return 'i';
  case 0x012D: return 'i';
  case 0x01D0: return 'i';
  case 0x00EE: return 'i';
  case 0x00EF: return 'i';
  case 0x1ECB: return 'i';
  case 0x0209: return 'i';
  case 0x00EC: return 'i';
  case 0x1EC9: return 'i';
  case 0x020B: return 'i';
  case 0x012B: return 'i';
  case 0x012F: return 'i';
  case 0x0268: return 'i';
  case 0x1E2D: return 'i';
  case 0x0129: return 'i';
  case 0x0365: return 'i';
  case 0x0131: return 'i';
  case 0x1D09: return 'i';
  case 0x1D62: return 'i';
  case 0x2071: return 'i';
  case 0x01F0: return 'j';
  case 0x0135: return 'j';
  case 0x029D: return 'j';
  case 0x0249: return 'j';
  case 0x025F: return 'j';
  case 0x0237: return 'j';
  case 0x1E31: return 'k';
  case 0x01E9: return 'k';
  case 0x0137: return 'k';
  case 0x1E33: return 'k';
  case 0x0199: return 'k';
  case 0x1E35: return 'k';
  case 0x029E: return 'k';
  case 0x2096: return 'k';
  case 0x013A: return 'l';
  case 0x019A: return 'l';
  case 0x026C: return 'l';
  case 0x013E: return 'l';
  case 0x013C: return 'l';
  case 0x1E3D: return 'l';
  case 0x0234: return 'l';
  case 0x1E37: return 'l';
  case 0x1E3B: return 'l';
  case 0x0140: return 'l';
  case 0x026B: return 'l';
  case 0x026D: return 'l';
  case 0x0142: return 'l';
  case 0x2097: return 'l';
  case 0x1E3F: return 'm';
  case 0x1E41: return 'm';
  case 0x1E43: return 'm';
  case 0x0271: return 'm';
  case 0x0270: return 'm';
  case 0x036B: return 'm';
  case 0x1D1F: return 'm';
  case 0x026F: return 'm';
  case 0x2098: return 'm';
  case 0x0144: return 'n';
  case 0x0148: return 'n';
  case 0x0146: return 'n';
  case 0x1E4B: return 'n';
  case 0x0235: return 'n';
  case 0x1E45: return 'n';
  case 0x1E47: return 'n';
  case 0x01F9: return 'n';
  case 0x0272: return 'n';
  case 0x1E49: return 'n';
  case 0x019E: return 'n';
  case 0x0273: return 'n';
  case 0x00F1: return 'n';
  case 0x2099: return 'n';
  case 0x00F3: return 'o';
  case 0x014F: return 'o';
  case 0x01D2: return 'o';
  case 0x00F4: return 'o';
  case 0x00F6: return 'o';
  case 0x022F: return 'o';
  case 0x1ECD: return 'o';
  case 0x0151: return 'o';
  case 0x020D: return 'o';
  case 0x00F2: return 'o';
  case 0x1ECF: return 'o';
  case 0x01A1: return 'o';
  case 0x020F: return 'o';
  case 0x014D: return 'o';
  case 0x01EB: return 'o';
  case 0x00F8: return 'o';
  case 0x1D13: return 'o';
  case 0x00F5: return 'o';
  case 0x0366: return 'o';
  case 0x0275: return 'o';
  case 0x1D17: return 'o';
  case 0x0254: return 'o';
  case 0x1D11: return 'o';
  case 0x1D12: return 'o';
  case 0x1D16: return 'o';
  case 0x1E55: return 'p';
  case 0x1E57: return 'p';
  case 0x01A5: return 'p';
  case 0x209A: return 'p';
  case 0x024B: return 'q';
  case 0x02A0: return 'q';
  case 0x0155: return 'r';
  case 0x0159: return 'r';
  case 0x0157: return 'r';
  case 0x1E59: return 'r';
  case 0x1E5B: return 'r';
  case 0x0211: return 'r';
  case 0x027E: return 'r';
  case 0x027F: return 'r';
  case 0x027B: return 'r';
  case 0x0213: return 'r';
  case 0x1E5F: return 'r';
  case 0x027C: return 'r';
  case 0x027A: return 'r';
  case 0x024D: return 'r';
  case 0x027D: return 'r';
  case 0x036C: return 'r';
  case 0x0279: return 'r';
  case 0x1D63: return 'r';
  case 0x015B: return 's';
  case 0x0161: return 's';
  case 0x015F: return 's';
  case 0x015D: return 's';
  case 0x0219: return 's';
  case 0x1E61: return 's';
  case 0x1E9B: return 's';
  case 0x1E63: return 's';
  case 0x0282: return 's';
  case 0x023F: return 's';
  case 0x017F: return 's';
  case 0x00DF: return 's';
  case 0x209B: return 's';
  case 0x0165: return 't';
  case 0x0163: return 't';
  case 0x1E71: return 't';
  case 0x021B: return 't';
  case 0x0236: return 't';
  case 0x1E97: return 't';
  case 0x1E6B: return 't';
  case 0x1E6D: return 't';
  case 0x01AD: return 't';
  case 0x1E6F: return 't';
  case 0x01AB: return 't';
  case 0x0288: return 't';
  case 0x0167: return 't';
  case 0x036D: return 't';
  case 0x0287: return 't';
  case 0x209C: return 't';
  case 0x0289: return 'u';
  case 0x00FA: return 'u';
  case 0x016D: return 'u';
  case 0x01D4: return 'u';
  case 0x1E77: return 'u';
  case 0x00FB: return 'u';
  case 0x1E73: return 'u';
  case 0x00FC: return 'u';
  case 0x1EE5: return 'u';
  case 0x0171: return 'u';
  case 0x0215: return 'u';
  case 0x00F9: return 'u';
  case 0x1EE7: return 'u';
  case 0x01B0: return 'u';
  case 0x0217: return 'u';
  case 0x016B: return 'u';
  case 0x0173: return 'u';
  case 0x016F: return 'u';
  case 0x1E75: return 'u';
  case 0x0169: return 'u';
  case 0x0367: return 'u';
  case 0x1D1D: return 'u';
  case 0x1D1E: return 'u';
  case 0x1D64: return 'u';
  case 0x1E7F: return 'v';
  case 0x028B: return 'v';
  case 0x1E7D: return 'v';
  case 0x036E: return 'v';
  case 0x028C: return 'v';
  case 0x1D65: return 'v';
  case 0x1E83: return 'w';
  case 0x0175: return 'w';
  case 0x1E85: return 'w';
  case 0x1E87: return 'w';
  case 0x1E89: return 'w';
  case 0x1E81: return 'w';
  case 0x1E98: return 'w';
  case 0x028D: return 'w';
  case 0x1E8D: return 'x';
  case 0x1E8B: return 'x';
  case 0x036F: return 'x';
  case 0x00FD: return 'y';
  case 0x0177: return 'y';
  case 0x00FF: return 'y';
  case 0x1E8F: return 'y';
  case 0x1EF5: return 'y';
  case 0x1EF3: return 'y';
  case 0x1EF7: return 'y';
  case 0x01B4: return 'y';
  case 0x0233: return 'y';
  case 0x1E99: return 'y';
  case 0x024F: return 'y';
  case 0x1EF9: return 'y';
  case 0x028E: return 'y';
  case 0x017A: return 'z';
  case 0x017E: return 'z';
  case 0x1E91: return 'z';
  case 0x0291: return 'z';
  case 0x017C: return 'z';
  case 0x1E93: return 'z';
  case 0x0225: return 'z';
  case 0x1E95: return 'z';
  case 0x0290: return 'z';
  case 0x01B6: return 'z';
  case 0x0240: return 'z';
  case 0x0251: return 'a';
  case 0x00C1: return 'A';
  case 0x00C2: return 'A';
  case 0x00C4: return 'A';
  case 0x00C0: return 'A';
  case 0x00C5: return 'A';
  case 0x023A: return 'A';
  case 0x00C3: return 'A';
  case 0x1D00: return 'A';
  case 0x0181: return 'B';
  case 0x0243: return 'B';
  case 0x0299: return 'B';
  case 0x1D03: return 'B';
  case 0x00C7: return 'C';
  case 0x023B: return 'C';
  case 0x1D04: return 'C';
  case 0x018A: return 'D';
  case 0x0189: return 'D';
  case 0x1D05: return 'D';
  case 0x00C9: return 'E';
  case 0x00CA: return 'E';
  case 0x00CB: return 'E';
  case 0x00C8: return 'E';
  case 0x0246: return 'E';
  case 0x0190: return 'E';
  case 0x018E: return 'E';
  case 0x1D07: return 'E';
  case 0x0193: return 'G';
  case 0x029B: return 'G';
  case 0x0262: return 'G';
  case 0x029C: return 'H';
  case 0x00CD: return 'I';
  case 0x00CE: return 'I';
  case 0x00CF: return 'I';
  case 0x0130: return 'I';
  case 0x00CC: return 'I';
  case 0x0197: return 'I';
  case 0x026A: return 'I';
  case 0x0248: return 'J';
  case 0x1D0A: return 'J';
  case 0x1D0B: return 'K';
  case 0x023D: return 'L';
  case 0x1D0C: return 'L';
  case 0x029F: return 'L';
  case 0x019C: return 'M';
  case 0x1D0D: return 'M';
  case 0x019D: return 'N';
  case 0x0220: return 'N';
  case 0x00D1: return 'N';
  case 0x0274: return 'N';
  case 0x1D0E: return 'N';
  case 0x00D3: return 'O';
  case 0x00D4: return 'O';
  case 0x00D6: return 'O';
  case 0x00D2: return 'O';
  case 0x019F: return 'O';
  case 0x00D8: return 'O';
  case 0x00D5: return 'O';
  case 0x0186: return 'O';
  case 0x1D0F: return 'O';
  case 0x1D10: return 'O';
  case 0x1D18: return 'P';
  case 0x024A: return 'Q';
  case 0x024C: return 'R';
  case 0x0280: return 'R';
  case 0x0281: return 'R';
  case 0x1D19: return 'R';
  case 0x1D1A: return 'R';
  case 0x023E: return 'T';
  case 0x01AE: return 'T';
  case 0x1D1B: return 'T';
  case 0x0244: return 'U';
  case 0x00DA: return 'U';
  case 0x00DB: return 'U';
  case 0x00DC: return 'U';
  case 0x00D9: return 'U';
  case 0x1D1C: return 'U';
  case 0x01B2: return 'V';
  case 0x0245: return 'V';
  case 0x1D20: return 'V';
  case 0x1D21: return 'W';
  case 0x00DD: return 'Y';
  case 0x0178: return 'Y';
  case 0x024E: return 'Y';
  case 0x028F: return 'Y';
  case 0x1D22: return 'Z';
  case 0x1EAE: return 'A';
  case 0x1EA4: return 'A';
  case 0x1EB0: return 'A';
  case 0x1EA6: return 'A';
  case 0x1EB2: return 'A';
  case 0x1EA8: return 'A';
  case 0x1EB4: return 'A';
  case 0x1EAA: return 'A';
  case 0x1EB6: return 'A';
  case 0x1EAC: return 'A';
  case 0x1EAF: return 'a';
  case 0x1EA5: return 'a';
  case 0x1EB1: return 'a';
  case 0x1EA7: return 'a';
  case 0x1EB3: return 'a';
  case 0x1EA9: return 'a';
  case 0x1EB5: return 'a';
  case 0x1EAB: return 'a';
  case 0x1EB7: return 'a';
  case 0x1EAD: return 'a';
  case 0x1EBE: return 'E';
  case 0x1EC0: return 'E';
  case 0x1EC2: return 'E';
  case 0x1EC4: return 'E';
  case 0x1EC6: return 'E';
  case 0x1EBF: return 'e';
  case 0x1EC1: return 'e';
  case 0x1EC3: return 'e';
  case 0x1EC5: return 'e';
  case 0x1EC7: return 'e';
  case 0x1ED0: return 'O';
  case 0x1EDA: return 'O';
  case 0x1ED2: return 'O';
  case 0x1EDC: return 'O';
  case 0x1ED4: return 'O';
  case 0x1EDE: return 'O';
  case 0x1ED6: return 'O';
  case 0x1EE0: return 'O';
  case 0x1ED8: return 'O';
  case 0x1EE2: return 'O';
  case 0x1ED1: return 'o';
  case 0x1EDB: return 'o';
  case 0x1ED3: return 'o';
  case 0x1EDD: return 'o';
  case 0x1ED5: return 'o';
  case 0x1EDF: return 'o';
  case 0x1ED7: return 'o';
  case 0x1EE1: return 'o';
  case 0x1ED9: return 'o';
  case 0x1EE3: return 'o';
  case 0x1EE8: return 'U';
  case 0x1EEA: return 'U';
  case 0x1EEC: return 'U';
  case 0x1EEE: return 'U';
  case 0x1EF0: return 'U';
  case 0x1EE9: return 'u';
  case 0x1EEB: return 'u';
  case 0x1EED: return 'u';
  case 0x1EEF: return 'u';
  case 0x1EF1: return 'u';
  }
  return cp;
}

static int16_t max16(int16_t a, int16_t b) {
  return (a > b) ? a : b;
}

static size_t min64u(size_t a, size_t b) {
  return (a < b) ? a : b;
}

static size_t index_at(size_t index, size_t max, bool forward) {
  if (forward) {
    return index;
  }
  return max - index - 1;
}

static position_t *pos_array(bool with_pos, size_t len) {
  if (with_pos) {
    position_t *pos = (position_t *)malloc(sizeof(position_t));
    pos->size = 0;
    pos->cap = len;
    pos->data = (size_t *)malloc(len * sizeof(size_t));
    return pos;
  }
  return NULL;
}

static void resize_pos(position_t *pos, size_t add_len) {
  if (pos->size + add_len > pos->cap) {
    pos->cap += add_len;
    pos->data = (size_t *)realloc(pos->data, sizeof(size_t) * pos->cap);
  }
}

static void append_pos(position_t *pos, size_t value) {
  resize_pos(pos, pos->cap);
  pos->data[pos->size] = value;
  pos->size++;
}

static void concat_pos(position_t *left, position_t *right) {
  resize_pos(left, right->size);
  memcpy(left->data + left->size, right->data, right->size * sizeof(size_t));
  left->size += right->size;
}

static void insert_pos(position_t *pos, size_t start, size_t end) {
  resize_pos(pos, end - start);
  for (size_t k = start; k < end; k++) {
    pos->data[pos->size] = k;
    pos->size++;
  }
}

static i16_t alloc16(size_t *offset, slab_t *slab, size_t size) {
  if (slab != NULL && slab->I16.cap > *offset + size) {
    i16_slice_t slice = slice_i16(slab->I16.data, *offset, (*offset) + size);
    *offset = *offset + size;
    return (i16_t){.data = slice.data,
                   .size = slice.size,
                   .cap = slice.size,
                   .allocated = false};
  }
  int16_t *data = (int16_t *)malloc(size * sizeof(int16_t));
  return (i16_t){.data = data, .size = size, .cap = size, .allocated = true};
}

static i32_t alloc32(size_t *offset, slab_t *slab, size_t size) {
  if (slab != NULL && slab->I32.cap > *offset + size) {
    i32_slice_t slice = slice_i32(slab->I32.data, *offset, (*offset) + size);
    *offset = *offset + size;
    return (i32_t){.data = slice.data,
                   .size = slice.size,
                   .cap = slice.size,
                   .allocated = false};
  }
  int32_t *data = (int32_t *)malloc(size * sizeof(int32_t));
  return (i32_t){.data = data, .size = size, .cap = size, .allocated = true};
}

static char_class char_class_of_ascii(int32_t cp) {
  if (cp >= 'a' && cp <= 'z') {
    return char_lower;
  } else if (cp >= 'A' && cp <= 'Z') {
    return char_upper;
  } else if (cp >= '0' && cp <= '9') {
    return char_number;
  }
  return char_non_word;
}

static char_class char_class_of_non_ascii(int32_t cp) {
  if (utf8_islower(cp)) {
    return char_lower;
  } else if (utf8_isupper(cp)) {
    return char_upper;
    // } else if (utf8_isnumber(cp)) {
    //   return char_number;
    // } else if (utf8_isletter(cp)) {
    //   return char_letter;
  }
  return char_non_word;
}

static char_class char_class_of(char ch) {
  if (ch <= UNICODE_MAXASCII) {
    return char_class_of_ascii(ch);
  }
  return char_class_of_non_ascii(ch);
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

static int16_t bonus_at(utf8str_t *input, size_t idx) {
  if (idx == 0) {
    return bonus_boundary;
  }
  return bonus_for(char_class_of(input->data[idx - 1]),
                   char_class_of(input->data[idx]));
}

static int32_t normalize_rune(int32_t r) {
  if (r < 0x00C0 || r > 0x2184) {
    return r;
  }
  return normalize(r);
}

static int32_t try_skip(utf8str_t *input, bool case_sensitive, int32_t cp,
                        int32_t from) {
  i32_slice_t slice = slice_i32(input->data, (size_t)from, input->len);
  int32_t idx = index_byte(&slice, cp);
  if (idx == 0) {
    return from;
  }

  if (!case_sensitive && cp >= 'a' && cp <= 'z') {
    if (idx > 0) {
      // TODO(conni2461): Now in retospect i think this is just
      // slice.size = (size_t)idx;
      i32_slice_t tmp = slice_i32_right(slice.data, (size_t)idx);
      slice.data = tmp.data;
      slice.size = tmp.size;
    }
    int32_t uidx = index_byte(&slice, cp - 32);
    if (uidx >= 0) {
      idx = uidx;
    }
  }
  if (idx < 0) {
    return -1;
  }

  return from + idx;
}

static bool is_ascii(utf8str_t *str) {
  for (size_t i = 0; i < str->len; i++) {
    if (str->data[i] >= UNICODE_MAXASCII) {
      return false;
    }
  }
  return true;
}

static int32_t ascii_fuzzy_index(utf8str_t *input, utf8str_t *pattern,
                                 bool case_sensitive) {
  if (!is_ascii(pattern)) {
    return -1;
  }

  int32_t first_idx = 0, idx = 0;
  for (size_t pidx = 0; pidx < pattern->len; pidx++) {
    idx = try_skip(input, case_sensitive, pattern->data[pidx], idx);
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

result_t fuzzy_match_v2(bool case_sensitive, bool normalize, bool forward,
                        utf8str_t *input, utf8str_t *pattern, bool with_pos,
                        slab_t *slab) {
  const size_t M = pattern->len;
  const size_t N = input->len;
  if (M == 0) {
    return (result_t){0, 0, 0, pos_array(with_pos, M)};
  }
  if (slab != NULL && N * M > slab->I16.cap) {
    return fuzzy_match_v1(case_sensitive, normalize, forward, input, pattern,
                          with_pos, slab);
  }

  size_t idx;
  {
    int32_t tmp_idx = ascii_fuzzy_index(input, pattern, case_sensitive);
    if (tmp_idx < 0) {
      return (result_t){-1, -1, 0, NULL};
    }
    idx = (size_t)tmp_idx;
  }

  size_t offset16 = 0, offset32 = 0;
  i16_t H0 = alloc16(&offset16, slab, N);
  i16_t C0 = alloc16(&offset16, slab, N);
  // Bonus point for each positions
  i16_t B = alloc16(&offset16, slab, N);
  // The first occurrence of each character in the pattern
  i32_t F = alloc32(&offset32, slab, M);
  // Rune array
  i32_t T = alloc32(&offset32, slab, N);
  copy_runes(input, &T); // input.CopyRunes(T)

  // Phase 2. Calculate bonus for each point
  int16_t max_score = 0;
  size_t max_score_pos = 0;

  size_t pidx = 0, last_idx = 0;

  int32_t pchar0 = pattern->data[0];
  int32_t pchar = pattern->data[0];
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
    int32_t c = Tsub.data[off];
    char_class class = char_class_of_ascii(c);
    if (!case_sensitive && class == char_upper) {
      c = utf8_tolower(c);
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
    free_alloc(T);
    free_alloc(F);
    free_alloc(B);
    free_alloc(C0);
    free_alloc(H0);
    return (result_t){-1, -1, 0, NULL};
  }
  if (M == 1) {
    free_alloc(T);
    free_alloc(F);
    free_alloc(B);
    free_alloc(C0);
    free_alloc(H0);
    result_t res = {(int32_t)max_score_pos, (int32_t)max_score_pos + 1,
                    max_score, NULL};
    if (!with_pos) {
      return res;
    }
    position_t *pos = pos_array(with_pos, 1);
    append_pos(pos, max_score_pos);
    res.pos = pos;
    return res;
  }

  size_t f0 = (size_t)F.data[0];
  size_t width = last_idx - f0 + 1;
  i16_t H = alloc16(&offset16, slab, width * M);
  {
    i16_slice_t H0_tmp_slice = slice_i16(H0.data, f0, last_idx + 1);
    copy_into_i16(&H0_tmp_slice, &H);
  }

  i16_t C = alloc16(&offset16, slab, width * M);
  {
    i16_slice_t C0_tmp_slice = slice_i16(C0.data, f0, last_idx + 1);
    copy_into_i16(&C0_tmp_slice, &C);
  }

  i32_slice_t Fsub = slice_i32(F.data, 1, F.size);
  i32_slice_t Psub =
      slice_i32_right(slice_i32(pattern->data, 1, M).data, Fsub.size);
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
      size_t col = j + f;
      int16_t s1 = 0, s2 = 0, consecutive = 0;

      if (in_gap) {
        s2 = Hleft.data[j] + score_gap_extention;
      } else {
        s2 = Hleft.data[j] + score_gap_start;
      }

      if (pchar == Tsub.data[j]) {
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
      if (pidx == M - 1 && ((forward && score > max_score) ||
                            (!forward && score >= max_score))) {
        max_score = score;
        max_score_pos = col;
      }
      Hsub.data[j] = score;
    }
  }

  position_t *pos = pos_array(with_pos, M);
  if (with_pos) {
    size_t i = M - 1;
    size_t j = max_score_pos;
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
  return (result_t){(int32_t)f0, (int32_t)max_score_pos + 1, (int32_t)max_score,
                    pos};
}

typedef struct {
  int32_t score;
  position_t *pos;
} score_pos_tuple_t;

static score_pos_tuple_t calculate_score(bool case_sensitive, bool normalize,
                                         utf8str_t *text, utf8str_t *pattern,
                                         size_t sidx, size_t eidx,
                                         bool with_pos) {
  const size_t len_pattern = pattern->len;

  size_t pidx = 0;
  int32_t score = 0, consecutive = 0, prev_class = char_non_word;
  bool in_gap = false;
  int16_t first_bonus = 0;
  position_t *pos = pos_array(with_pos, len_pattern);
  if (sidx > 0) {
    prev_class = char_class_of(text->data[sidx - 1]);
  }
  for (size_t idx = sidx; idx < eidx; idx++) {
    int32_t c = text->data[idx];
    int32_t class = char_class_of(c);
    if (!case_sensitive) {
      c = utf8_tolower(c);
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

result_t fuzzy_match_v1(bool case_sensitive, bool normalize, bool forward,
                        utf8str_t *text, utf8str_t *pattern, bool with_pos,
                        slab_t *slab) {
  const size_t len_pattern = pattern->len;
  const size_t len_runes = text->len;
  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0, NULL};
  }

  int32_t pidx = 0, sidx = -1, eidx = -1;
  for (size_t idx = 0; idx < len_runes; idx++) {
    int32_t c = text->data[index_at(idx, len_runes, forward)];
    /* TODO(conni2461): Common pattern maybe a macro would be good here */
    if (!case_sensitive) {
      c = utf8_tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c == pattern->data[index_at((size_t)pidx, len_pattern, forward)]) {
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
      char c = text->data[index_at(idx, len_runes, forward)];
      if (!case_sensitive) {
        c = utf8_tolower(c);
      }
      if (c == pattern->data[index_at((size_t)pidx, len_pattern, forward)]) {
        pidx--;
        if (pidx < 0) {
          start = idx;
          break;
        }
      }
    }
    if (!forward) {
      start = len_runes - end;
      end = len_runes - start;
    }

    score_pos_tuple_t tuple = calculate_score(case_sensitive, normalize, text,
                                              pattern, start, end, with_pos);
    return (result_t){(int32_t)start, (int32_t)end, tuple.score, tuple.pos};
  }
  return (result_t){-1, -1, 0, NULL};
}

result_t exact_match_naive(bool case_sensitive, bool normalize, bool forward,
                           utf8str_t *text, utf8str_t *pattern, bool with_pos,
                           slab_t *slab) {
  const size_t len_pattern = pattern->len;
  const size_t len_runes = text->len;

  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }
  if (len_runes < len_pattern) {
    return (result_t){-1, -1, 0, NULL};
  }
  if (ascii_fuzzy_index(text, pattern, case_sensitive) < 0) {
    return (result_t){-1, -1, 0, NULL};
  }

  size_t pidx = 0;
  int32_t best_pos = -1;
  int16_t bonus = 0, best_bonus = -1;
  for (size_t idx = 0; idx < len_runes; idx++) {
    size_t idx_ = index_at(idx, len_runes, forward);
    int32_t c = text->data[idx_];
    if (!case_sensitive) {
      c = utf8_tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    size_t pidx_ = index_at(pidx, len_pattern, forward);
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
    size_t sidx, eidx, bp = (size_t)best_pos;
    if (forward) {
      sidx = bp - len_pattern + 1;
      eidx = bp + 1;
    } else {
      sidx = len_runes - (bp + 1);
      eidx = len_runes - (bp - len_pattern + 1);
    }
    int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                    sidx, eidx, false)
                        .score;
    return (result_t){(int32_t)sidx, (int32_t)eidx, score, NULL};
  }
  return (result_t){-1, -1, 0, NULL};
}

result_t prefix_match(bool case_sensitive, bool normalize, bool forward,
                      utf8str_t *text, utf8str_t *pattern, bool with_pos,
                      slab_t *slab) {
  const size_t len_pattern = pattern->len;
  if (len_pattern == 0) {
    return (result_t){0, 0, 0, NULL};
  }
  size_t trimmed_len = 0;
  /* TODO(conni2461): i feel this is wrong */
  if (!isspace(pattern->data[0])) {
    trimmed_len = leading_whitespaces(text);
  }
  if (text->len - trimmed_len < len_pattern) {
    return (result_t){-1, -1, 0, NULL};
  }
  for (size_t i = 0; i < len_pattern; i++) {
    int32_t c = text->data[trimmed_len + i];
    if (!case_sensitive) {
      c = utf8_tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c != pattern->data[i]) {
      return (result_t){-1, -1, 0, NULL};
    }
  }
  size_t start = trimmed_len;
  size_t end = trimmed_len + len_pattern;
  int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                  start, end, false)
                      .score;
  return (result_t){(int32_t)start, (int32_t)end, score, NULL};
}

result_t suffix_match(bool case_sensitive, bool normalize, bool forward,
                      utf8str_t *text, utf8str_t *pattern, bool with_pos,
                      slab_t *slab) {
  const size_t len_runes = text->len;
  size_t trimmed_len = len_runes;
  const size_t len_pattern = pattern->len;
  /* TODO(conni2461): i feel this is wrong */
  if (len_pattern == 0 || !isspace(pattern->data[len_pattern - 1])) {
    trimmed_len -= trailing_whitespaces(text);
  }
  if (len_pattern == 0) {
    return (result_t){(int32_t)trimmed_len, (int32_t)trimmed_len, 0, NULL};
  }
  size_t diff = trimmed_len - len_pattern;
  if (diff < 0) {
    return (result_t){-1, -1, 0, NULL};
  }

  for (size_t idx = 0; idx < len_pattern; idx++) {
    char c = text->data[idx + diff];
    if (!case_sensitive) {
      c = utf8_tolower(c);
    }
    if (normalize) {
      c = normalize_rune(c);
    }
    if (c != pattern->data[idx]) {
      return (result_t){-1, -1, 0, NULL};
    }
  }
  size_t start = trimmed_len - len_pattern;
  size_t end = trimmed_len;
  int32_t score = calculate_score(case_sensitive, normalize, text, pattern,
                                  start, end, false)
                      .score;
  return (result_t){(int32_t)start, (int32_t)end, score, NULL};
}

result_t equal_match(bool case_sensitive, bool normalize, bool forward,
                     utf8str_t *text, utf8str_t *pattern, bool withPos,
                     slab_t *slab) {
  const size_t len_pattern = pattern->len;
  if (len_pattern == 0) {
    return (result_t){-1, -1, 0, NULL};
  }
  size_t trimmed_len = leading_whitespaces(text);
  size_t trimmed_end_len = trailing_whitespaces(text);

  if ((text->len - trimmed_len - trimmed_end_len) != len_pattern) {
    return (result_t){-1, -1, 0, NULL};
  }

  bool match = true;
  if (normalize) {
    for (size_t idx = 0; idx < len_pattern; idx++) {
      int32_t c = text->data[trimmed_len + idx];
      if (!case_sensitive) {
        c = utf8_tolower(c);
      }
      if (normalize_rune(c) != normalize_rune(pattern->data[idx])) {
        match = false;
        break;
      }
    }
  } else {
    for (size_t idx = 0; idx < len_pattern; idx++) {
      int32_t c = text->data[trimmed_len + idx];
      if (!case_sensitive) {
        c = utf8_tolower(c);
      }
      if (c != pattern->data[idx]) {
        match = false;
        break;
      }
    }
  }
  if (match) {
    return (result_t){(int32_t)trimmed_len,
                      ((int32_t)trimmed_len + (int32_t)len_pattern),
                      (score_match + bonus_boundary) * (int32_t)len_pattern +
                          (bonus_first_char_multiplier - 1) * bonus_boundary,
                      NULL};
  }
  return (result_t){-1, -1, 0, NULL};
}

static void append_set(term_set_t *set, term_t value) {
  if (set->cap == 0) {
    set->cap = 1;
    set->ptr = (term_t *)malloc(sizeof(term_t));
  } else if (set->size + 1 > set->cap) {
    set->cap *= 2;
    set->ptr = realloc(set->ptr, sizeof(term_t) * set->cap);
    assert(set->ptr != NULL);
  }
  set->ptr[set->size] = value;
  set->size++;
}

static void append_pattern(pattern_t *pattern, term_set_t *value) {
  if (pattern->cap == 0) {
    pattern->cap = 1;
    pattern->ptr = (term_set_t **)malloc(sizeof(term_set_t *));
  } else if (pattern->size + 1 > pattern->cap) {
    pattern->cap *= 2;
    pattern->ptr = realloc(pattern->ptr, sizeof(term_set_t *) * pattern->cap);
    assert(pattern->ptr != NULL);
  }
  pattern->ptr[pattern->size] = value;
  pattern->size++;
}

static algorithm_t get_alg(alg_types typ) {
  switch (typ) {
  case term_fuzzy: return &fuzzy_match_v2;
  case term_exact: return &exact_match_naive;
  case term_prefix: return &prefix_match;
  case term_suffix: return &suffix_match;
  case term_equal: return &equal_match;
  }
  return &fuzzy_match_v2;
}

/* assumption (maybe i change that later)
 * - bool fuzzy always true
 * - always v2 alg
 * - bool extended always true (thats the whole point of this isn't it)
 */
pattern_t *parse_pattern(case_types case_mode, bool normalize, char *pattern) {
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

  pattern_t *pat_obj = (pattern_t *)malloc(sizeof(pattern_t));
  memset(pat_obj, 0, sizeof(*pat_obj));
  term_set_t *set = (term_set_t *)malloc(sizeof(term_set_t));
  memset(set, 0, sizeof(*set));

  bool switch_set = false;
  bool after_bar = false;
  while (ptr != NULL) {
    alg_types typ = term_fuzzy;
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
        append_pattern(pat_obj, set);
        set = (term_set_t *)malloc(sizeof(term_set_t));
        set->cap = 0;
        set->size = 0;
      }
      append_set(set, (term_t){.typ = typ,
                               .alg = get_alg(typ),
                               .inv = inv,
                               // TODO(conni2461): we have the len here
                               .text = into_utf8(text),
                               .case_sensitive = case_sensitive});
      free(og_str);
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
    term_set_t *term_set = pat_obj->ptr[i];
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

void free_pattern(pattern_t *pattern) {
  for (size_t i = 0; i < pattern->size; i++) {
    term_set_t *term_set = pattern->ptr[i];
    for (size_t j = 0; j < term_set->size; j++) {
      term_t *term = &term_set->ptr[j];
      free_utfstr(&term->text);
    }
    free(term_set->ptr);
    free(term_set);
  }
  free(pattern->ptr);
  free(pattern);
}

int32_t get_score(char *text, pattern_t *pattern, slab_t *slab) {
  utf8str_t input = into_utf8(text);

  if (pattern->only_inv) {
    int final = 0;
    for (size_t i = 0; i < pattern->size; i++) {
      term_set_t *term_set = pattern->ptr[i];
      term_t *term = &term_set->ptr[0];
      final += term->alg(term->case_sensitive, false, true, &input, &term->text,
                         false, slab)
                   .score;
    }
    free_utfstr(&input);
    return (final > 0) ? 0 : 1;
  }

  int32_t total_score = 0;
  for (size_t i = 0; i < pattern->size; i++) {
    term_set_t *term_set = pattern->ptr[i];
    int32_t current_score = 0;
    bool matched = false;
    for (size_t j = 0; j < term_set->size; j++) {
      term_t *term = &term_set->ptr[j];
      result_t res = term->alg(term->case_sensitive, false, true, &input,
                               &term->text, false, slab);
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

  free_utfstr(&input);
  return total_score;
}

position_t *get_positions(char *text, pattern_t *pattern, slab_t *slab) {
  utf8str_t input = into_utf8(text);

  position_t *all_pos = pos_array(true, 1);

  for (size_t i = 0; i < pattern->size; i++) {
    term_set_t *term_set = pattern->ptr[i];
    result_t current_res = (result_t){0, 0, 0, NULL};
    bool matched = false;
    for (size_t j = 0; j < term_set->size; j++) {
      term_t *term = &term_set->ptr[j];
      result_t res = term->alg(term->case_sensitive, false, true, &input,
                               &term->text, true, slab);
      if (res.start >= 0) {
        if (term->inv) {
          free_positions(res.pos);
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
        free_positions(current_res.pos);
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
  free_utfstr(&input);
  return all_pos;
}

void free_positions(position_t *pos) {
  if (pos) {
    if (pos->data) {
      free(pos->data);
    }
    free(pos);
  }
}

void iter_positions(position_t *pos, void (*handle_position)(size_t pos)) {
  for (size_t i = 0; i < pos->size; i++) {
    handle_position(pos->data[i]);
  }
}

slab_t *make_slab(size_t size_16, size_t size_32) {
  slab_t *slab = (slab_t *)malloc(sizeof(slab_t));
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

slab_t *make_default_slab(void) {
  return make_slab(100 * 1024, 2048);
}

void free_slab(slab_t *slab) {
  if (slab) {
    free(slab->I16.data);
    free(slab->I32.data);
    free(slab);
  }
}
