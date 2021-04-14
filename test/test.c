#include "fzf.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>
#include <stdio.h>

#define call_alg(alg, case, txt, pat, assert_block)                            \
  {                                                                            \
    string_t text = {.data = txt, .size = strlen(txt)};                        \
    string_t pattern = {.data = pat, .size = strlen(pat)};                     \
    result_t res = alg(case, false, true, &text, &pattern, true, NULL);        \
    assert_block;                                                              \
    if (res.pos) {                                                             \
      free(res.pos->data);                                                     \
      free(res.pos);                                                           \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    slab_t *slab = make_slab(100 * 1024, 2048);                                \
    string_t text = {.data = txt, .size = strlen(txt)};                        \
    string_t pattern = {.data = pat, .size = strlen(pat)};                     \
    result_t res = alg(case, false, true, &text, &pattern, true, slab);        \
    assert_block;                                                              \
    if (res.pos) {                                                             \
      free(res.pos->data);                                                     \
      free(res.pos);                                                           \
    }                                                                          \
    free_slab(slab);                                                           \
  }

// TODO(conni2461): Implement normalize and test it here

static void test_fuzzy_match_v2(void **state) {
  call_alg(fuzzy_match_v2, true, "So Danco Samba", "So", {
    assert_int_equal(0, res.start);
    assert_int_equal(2, res.end);
    assert_int_equal(56, res.score);

    assert_int_equal(2, res.pos->size);
    assert_int_equal(1, res.pos->data[0]);
    assert_int_equal(0, res.pos->data[1]);
  });

  call_alg(fuzzy_match_v2, false, "So Danco Samba", "sodc", {
    assert_int_equal(0, res.start);
    assert_int_equal(7, res.end);
    assert_int_equal(89, res.score);

    assert_int_equal(4, res.pos->size);
    assert_int_equal(6, res.pos->data[0]);
    assert_int_equal(3, res.pos->data[1]);
    assert_int_equal(1, res.pos->data[2]);
    assert_int_equal(0, res.pos->data[3]);
  });

  call_alg(fuzzy_match_v2, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);

    assert_int_equal(5, res.pos->size);
    assert_int_equal(4, res.pos->data[0]);
    assert_int_equal(3, res.pos->data[1]);
    assert_int_equal(2, res.pos->data[2]);
    assert_int_equal(1, res.pos->data[3]);
    assert_int_equal(0, res.pos->data[4]);
  });
}

static void test_fuzzy_match_v1(void **state) {
  call_alg(fuzzy_match_v1, true, "So Danco Samba", "So", {
    assert_int_equal(0, res.start);
    assert_int_equal(2, res.end);
    assert_int_equal(56, res.score);

    assert_int_equal(2, res.pos->size);
    assert_int_equal(0, res.pos->data[0]);
    assert_int_equal(1, res.pos->data[1]);
  });

  call_alg(fuzzy_match_v1, false, "So Danco Samba", "sodc", {
    assert_int_equal(0, res.start);
    assert_int_equal(7, res.end);
    assert_int_equal(89, res.score);

    assert_int_equal(4, res.pos->size);
    assert_int_equal(0, res.pos->data[0]);
    assert_int_equal(1, res.pos->data[1]);
    assert_int_equal(3, res.pos->data[2]);
    assert_int_equal(6, res.pos->data[3]);
  });

  call_alg(fuzzy_match_v1, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);

    assert_int_equal(5, res.pos->size);
    assert_int_equal(0, res.pos->data[0]);
    assert_int_equal(1, res.pos->data[1]);
    assert_int_equal(2, res.pos->data[2]);
    assert_int_equal(3, res.pos->data[3]);
    assert_int_equal(4, res.pos->data[4]);
  });
}

static void test_prefix_match(void **state) {
  call_alg(prefix_match, true, "So Danco Samba", "So", {
    assert_int_equal(0, res.start);
    assert_int_equal(2, res.end);
    assert_int_equal(56, res.score);
  });

  call_alg(prefix_match, false, "So Danco Samba", "sodc", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(prefix_match, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);
  });
}

static void test_exact_match(void **state) {
  call_alg(exact_match_naive, true, "So Danco Samba", "So", {
    assert_int_equal(0, res.start);
    assert_int_equal(2, res.end);
    assert_int_equal(56, res.score);
  });

  call_alg(exact_match_naive, false, "So Danco Samba", "sodc", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(exact_match_naive, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);
  });
}

static void test_suffix_match(void **state) {
  call_alg(suffix_match, true, "So Danco Samba", "So", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(suffix_match, false, "So Danco Samba", "sodc", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(suffix_match, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);
  });
}

static void test_equal_match(void **state) {
  call_alg(equal_match, true, "So Danco Samba", "So", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(equal_match, false, "So Danco Samba", "sodc", {
    assert_int_equal(-1, res.start);
    assert_int_equal(-1, res.end);
    assert_int_equal(0, res.score);
  });

  call_alg(equal_match, false, "Danco", "danco", {
    assert_int_equal(0, res.start);
    assert_int_equal(5, res.end);
    assert_int_equal(128, res.score);
  });
}

#define test_pat(case, str, test_block)                                        \
  {                                                                            \
    pattern_t *pat = parse_pattern(case, false, str);                          \
    test_block;                                                                \
    free_pattern(pat);                                                         \
  }

static void test_parse_pattern(void **state) {
  test_pat(case_smart, "lua", {
    assert_int_equal(1, pat->size);
    assert_int_equal(1, pat->cap);

    assert_int_equal(1, pat->ptr[0]->size);
    assert_int_equal(1, pat->ptr[0]->cap);

    assert_int_equal(term_fuzzy, pat->ptr[0]->ptr[0].typ);
    assert_string_equal("lua", pat->ptr[0]->ptr[0].text.data);
    assert_false(pat->ptr[0]->ptr[0].case_sensitive);
  });

  test_pat(case_smart, "Lua", {
    assert_int_equal(1, pat->size);
    assert_int_equal(1, pat->cap);

    assert_int_equal(1, pat->ptr[0]->size);
    assert_int_equal(1, pat->ptr[0]->cap);

    assert_int_equal(term_fuzzy, pat->ptr[0]->ptr[0].typ);
    assert_string_equal("Lua", pat->ptr[0]->ptr[0].text.data);
    assert_true(pat->ptr[0]->ptr[0].case_sensitive);
  });

  test_pat(case_smart, "'src | ^Lua", {
    assert_int_equal(1, pat->size);
    assert_int_equal(1, pat->cap);

    assert_int_equal(2, pat->ptr[0]->size);
    assert_int_equal(2, pat->ptr[0]->cap);

    assert_int_equal(term_exact, pat->ptr[0]->ptr[0].typ);
    assert_string_equal("src", pat->ptr[0]->ptr[0].text.data);
    assert_false(pat->ptr[0]->ptr[0].case_sensitive);

    assert_int_equal(term_prefix, pat->ptr[0]->ptr[1].typ);
    assert_string_equal("Lua", pat->ptr[0]->ptr[1].text.data);
    assert_true(pat->ptr[0]->ptr[1].case_sensitive);
  });

  test_pat(case_smart, ".lua$ 'previewer !'term !asdf", {
    assert_int_equal(4, pat->size);
    assert_int_equal(4, pat->cap);

    assert_int_equal(1, pat->ptr[0]->size);
    assert_int_equal(1, pat->ptr[0]->cap);
    assert_int_equal(1, pat->ptr[1]->size);
    assert_int_equal(1, pat->ptr[1]->cap);
    assert_int_equal(1, pat->ptr[2]->size);
    assert_int_equal(1, pat->ptr[2]->cap);
    assert_int_equal(1, pat->ptr[3]->size);
    assert_int_equal(1, pat->ptr[3]->cap);

    assert_int_equal(term_suffix, pat->ptr[0]->ptr[0].typ);
    assert_string_equal(".lua", pat->ptr[0]->ptr[0].text.data);
    assert_false(pat->ptr[0]->ptr[0].case_sensitive);

    assert_int_equal(term_exact, pat->ptr[1]->ptr[0].typ);
    assert_string_equal("previewer", pat->ptr[1]->ptr[0].text.data);
    assert_int_equal(0, pat->ptr[1]->ptr[0].case_sensitive);

    assert_int_equal(term_fuzzy, pat->ptr[2]->ptr[0].typ);
    assert_string_equal("term", pat->ptr[2]->ptr[0].text.data);
    assert_false(pat->ptr[2]->ptr[0].case_sensitive);
    assert_true(pat->ptr[2]->ptr[0].inv);

    assert_int_equal(term_exact, pat->ptr[3]->ptr[0].typ);
    assert_string_equal("asdf", pat->ptr[3]->ptr[0].text.data);
    assert_false(pat->ptr[3]->ptr[0].case_sensitive);
    assert_true(pat->ptr[3]->ptr[0].inv);
  });
}

int main(void) {
  const struct CMUnitTest tests[] = {
      // Algorithms
      cmocka_unit_test(test_fuzzy_match_v2),
      cmocka_unit_test(test_fuzzy_match_v1),
      cmocka_unit_test(test_prefix_match),
      cmocka_unit_test(test_exact_match),
      cmocka_unit_test(test_suffix_match),
      cmocka_unit_test(test_equal_match),

      // Pattern
      cmocka_unit_test(test_parse_pattern),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
