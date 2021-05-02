// BASED ON https://github.com/sheredom/utf8.h

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char char_u;

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

typedef struct {
  int32_t *data;
  size_t len;
} utf8str_t;

static utf8str_t into_utf8(const char *str) {
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

static void free_utfstr(utf8str_t *str) {
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

/*
func NormalizeRunes(runes []rune) []rune {
  ret := make([]rune, len(runes))
  copy(ret, runes)
  for idx, r := range runes {
    if r < 0x00C0 || r > 0x2184 {
      continue
    }
    n := normalized[r]
    if n > 0 {
      ret[idx] = normalized[r]
    }
  }
  return ret
}
*/

int main(int argc, char *argv[]) {
  const char *str = "Ấ Héllo, WÖörld!";
  utf8str_t transform = into_utf8(str);
  printf("%s : utf8len %zu\n\n", str, transform.len);
  for (size_t i = 0; i < transform.len; i++) {
    int32_t *c = &transform.data[i];
    int32_t n = normalize(*c);
    printf("%d -> ", *c);
    if (n != *c) {
      printf("%d -> ", n);
    }
    if (utf8_islower(*c)) {
      printf("%d\n", utf8_toupper(*c));
    } else {
      printf("%d\n", utf8_tolower(*c));
    }
  }

  free_utfstr(&transform);
  return 0;
}
