#include "fzf.h"

#include <examiner.h>
#include <stdlib.h>

typedef struct {
  char *data;
  size_t size;
} fzf_string_t;

#define call_alg(alg, case, txt, pat, assert_block)                            \
  {                                                                            \
    fzf_result_t res = alg(case, false, txt, pat, true, NULL);                 \
    assert_block;                                                              \
    if (res.pos) {                                                             \
      free(res.pos->data);                                                     \
      free(res.pos);                                                           \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    fzf_slab_t *slab = fzf_make_default_slab();                                \
    fzf_result_t res = alg(case, false, txt, pat, true, slab);                 \
    assert_block;                                                              \
    if (res.pos) {                                                             \
      free(res.pos->data);                                                     \
      free(res.pos);                                                           \
    }                                                                          \
    fzf_free_slab(slab);                                                       \
  }

// TODO(conni2461): Implement normalize and test it here
TEST(fuzzy_match_v2, case1) {
  call_alg(fzf_fuzzy_match_v2, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(2, res.end);
    ASSERT_EQUAL(56, res.score);

    ASSERT_EQUAL(2, res.pos->size);
    ASSERT_EQUAL(1, res.pos->data[0]);
    ASSERT_EQUAL(0, res.pos->data[1]);
  });
}

TEST(fuzzy_match_v2, case2) {
  call_alg(fzf_fuzzy_match_v2, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(7, res.end);
    ASSERT_EQUAL(89, res.score);

    ASSERT_EQUAL(4, res.pos->size);
    ASSERT_EQUAL(6, res.pos->data[0]);
    ASSERT_EQUAL(3, res.pos->data[1]);
    ASSERT_EQUAL(1, res.pos->data[2]);
    ASSERT_EQUAL(0, res.pos->data[3]);
  });
}

TEST(fuzzy_match_v2, case3) {
  call_alg(fzf_fuzzy_match_v2, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);

    ASSERT_EQUAL(5, res.pos->size);
    ASSERT_EQUAL(4, res.pos->data[0]);
    ASSERT_EQUAL(3, res.pos->data[1]);
    ASSERT_EQUAL(2, res.pos->data[2]);
    ASSERT_EQUAL(1, res.pos->data[3]);
    ASSERT_EQUAL(0, res.pos->data[4]);
  });
}

TEST(fuzzy_match_v1, case1) {
  call_alg(fzf_fuzzy_match_v1, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(2, res.end);
    ASSERT_EQUAL(56, res.score);

    ASSERT_EQUAL(2, res.pos->size);
    ASSERT_EQUAL(0, res.pos->data[0]);
    ASSERT_EQUAL(1, res.pos->data[1]);
  });
}

TEST(fuzzy_match_v1, case2) {
  call_alg(fzf_fuzzy_match_v1, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(7, res.end);
    ASSERT_EQUAL(89, res.score);

    ASSERT_EQUAL(4, res.pos->size);
    ASSERT_EQUAL(0, res.pos->data[0]);
    ASSERT_EQUAL(1, res.pos->data[1]);
    ASSERT_EQUAL(3, res.pos->data[2]);
    ASSERT_EQUAL(6, res.pos->data[3]);
  });
}

TEST(fuzzy_match_v1, case3) {
  call_alg(fzf_fuzzy_match_v1, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);

    ASSERT_EQUAL(5, res.pos->size);
    ASSERT_EQUAL(0, res.pos->data[0]);
    ASSERT_EQUAL(1, res.pos->data[1]);
    ASSERT_EQUAL(2, res.pos->data[2]);
    ASSERT_EQUAL(3, res.pos->data[3]);
    ASSERT_EQUAL(4, res.pos->data[4]);
  });
}

TEST(exact_match, case1) {
  call_alg(fzf_exact_match_naive, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(2, res.end);
    ASSERT_EQUAL(56, res.score);
  });
}

TEST(exact_match, case2) {
  call_alg(fzf_exact_match_naive, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(exact_match, case3) {
  call_alg(fzf_exact_match_naive, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);
  });
}

TEST(prefix_match, case1) {
  call_alg(fzf_prefix_match, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(2, res.end);
    ASSERT_EQUAL(56, res.score);
  });
}

TEST(prefix_match, case2) {
  call_alg(fzf_prefix_match, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(prefix_match, case3) {
  call_alg(fzf_prefix_match, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);
  });
}

TEST(suffix_match, case1) {
  call_alg(fzf_suffix_match, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(suffix_match, case2) {
  call_alg(fzf_suffix_match, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(suffix_match, case3) {
  call_alg(fzf_suffix_match, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);
  });
}

TEST(equal_match, case1) {
  call_alg(fzf_equal_match, true, "So Danco Samba", "So", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(equal_match, case2) {
  call_alg(fzf_equal_match, false, "So Danco Samba", "sodc", {
    ASSERT_EQUAL(-1, res.start);
    ASSERT_EQUAL(-1, res.end);
    ASSERT_EQUAL(0, res.score);
  });
}

TEST(equal_match, case3) {
  call_alg(fzf_equal_match, false, "Danco", "danco", {
    ASSERT_EQUAL(0, res.start);
    ASSERT_EQUAL(5, res.end);
    ASSERT_EQUAL(128, res.score);
  });
}

TEST(pattern_parsing, simple) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, "lua", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("lua", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, with_escaped_space) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, "file\\ ", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("file ", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, with_complex_escaped_space) {
  fzf_pattern_t *pat =
      fzf_parse_pattern(case_smart, false, "file\\ with\\ space", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("file with space",
               ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, with_escaped_space_and_normal_space) {
  fzf_pattern_t *pat =
      fzf_parse_pattern(case_smart, false, "file\\  new", true);
  ASSERT_EQUAL(2, pat->size);
  ASSERT_EQUAL(2, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);
  ASSERT_EQUAL(1, pat->ptr[1]->size);
  ASSERT_EQUAL(1, pat->ptr[1]->cap);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("file ", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[1]->ptr[0].typ);
  ASSERT_EQUAL("new", ((fzf_string_t *)(pat->ptr[1]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[1]->ptr[0].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, invert) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, "!Lua", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_TRUE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_exact, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("Lua", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_TRUE(pat->ptr[0]->ptr[0].case_sensitive);
  ASSERT_TRUE(pat->ptr[0]->ptr[0].inv);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, invert_multiple) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, "!fzf !test", true);
  ASSERT_EQUAL(2, pat->size);
  ASSERT_EQUAL(2, pat->cap);
  ASSERT_TRUE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);
  ASSERT_EQUAL(1, pat->ptr[1]->size);
  ASSERT_EQUAL(1, pat->ptr[1]->cap);

  ASSERT_EQUAL(term_exact, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("fzf", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);
  ASSERT_TRUE(pat->ptr[0]->ptr[0].inv);

  ASSERT_EQUAL(term_exact, pat->ptr[1]->ptr[0].typ);
  ASSERT_EQUAL("test", ((fzf_string_t *)(pat->ptr[1]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[1]->ptr[0].case_sensitive);
  ASSERT_TRUE(pat->ptr[1]->ptr[0].inv);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, smart_case) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, "Lua", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("Lua", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_TRUE(pat->ptr[0]->ptr[0].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, simple_or) {
  fzf_pattern_t *pat =
      fzf_parse_pattern(case_smart, false, "'src | ^Lua", true);
  ASSERT_EQUAL(1, pat->size);
  ASSERT_EQUAL(1, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(2, pat->ptr[0]->size);
  ASSERT_EQUAL(2, pat->ptr[0]->cap);

  ASSERT_EQUAL(term_exact, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL("src", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);

  ASSERT_EQUAL(term_prefix, pat->ptr[0]->ptr[1].typ);
  ASSERT_EQUAL("Lua", ((fzf_string_t *)(pat->ptr[0]->ptr[1].text))->data);
  ASSERT_TRUE(pat->ptr[0]->ptr[1].case_sensitive);
  fzf_free_pattern(pat);
}

TEST(pattern_parsing, complex_and) {
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false,
                                         ".lua$ 'previewer !'term !asdf", true);
  ASSERT_EQUAL(4, pat->size);
  ASSERT_EQUAL(4, pat->cap);
  ASSERT_FALSE(pat->only_inv);

  ASSERT_EQUAL(1, pat->ptr[0]->size);
  ASSERT_EQUAL(1, pat->ptr[0]->cap);
  ASSERT_EQUAL(1, pat->ptr[1]->size);
  ASSERT_EQUAL(1, pat->ptr[1]->cap);
  ASSERT_EQUAL(1, pat->ptr[2]->size);
  ASSERT_EQUAL(1, pat->ptr[2]->cap);
  ASSERT_EQUAL(1, pat->ptr[3]->size);
  ASSERT_EQUAL(1, pat->ptr[3]->cap);

  ASSERT_EQUAL(term_suffix, pat->ptr[0]->ptr[0].typ);
  ASSERT_EQUAL(".lua", ((fzf_string_t *)(pat->ptr[0]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[0]->ptr[0].case_sensitive);

  ASSERT_EQUAL(term_exact, pat->ptr[1]->ptr[0].typ);
  ASSERT_EQUAL("previewer", ((fzf_string_t *)(pat->ptr[1]->ptr[0].text))->data);
  ASSERT_EQUAL(0, pat->ptr[1]->ptr[0].case_sensitive);

  ASSERT_EQUAL(term_fuzzy, pat->ptr[2]->ptr[0].typ);
  ASSERT_EQUAL("term", ((fzf_string_t *)(pat->ptr[2]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[2]->ptr[0].case_sensitive);
  ASSERT_TRUE(pat->ptr[2]->ptr[0].inv);

  ASSERT_EQUAL(term_exact, pat->ptr[3]->ptr[0].typ);
  ASSERT_EQUAL("asdf", ((fzf_string_t *)(pat->ptr[3]->ptr[0].text))->data);
  ASSERT_FALSE(pat->ptr[3]->ptr[0].case_sensitive);
  ASSERT_TRUE(pat->ptr[3]->ptr[0].inv);
  fzf_free_pattern(pat);
}

static void score_wrapper(char *pattern, char **input, int *expected) {
  fzf_slab_t *slab = fzf_make_default_slab();
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, pattern, true);
  for (size_t i = 0; input[i] != NULL; ++i) {
    ASSERT_EQUAL(expected[i], fzf_get_score(input[i], pat, slab));
  }
  fzf_free_pattern(pat);
  fzf_free_slab(slab);
}

TEST(score_integration, simple) {
  char *input[] = {"fzf", "main.c", "src/fzf", "fz/noooo", NULL};
  int expected[] = {0, 1, 0, 1};
  score_wrapper("!fzf", input, expected);
}

TEST(score_integration, invert_and) {
  char *input[] = {"src/fzf.c", "README.md", "lua/asdf", "test/test.c", NULL};
  int expected[] = {0, 1, 1, 0};
  score_wrapper("!fzf !test", input, expected);
}

TEST(score_integration, with_escaped_space) {
  char *input[] = {"file ", "file lua", "lua", NULL};
  int expected[] = {0, 200, 0};
  score_wrapper("file\\ lua", input, expected);
}

TEST(score_integration, only_escaped_space) {
  char *input[] = {"file with space", "file lua", "lua", "src", "test", NULL};
  int expected[] = {32, 32, 0, 0, 0};
  score_wrapper("\\ ", input, expected);
}

TEST(score_integration, simple_or) {
  char *input[] = {"src/fzf.h",       "README.md",       "build/fzf",
                   "lua/fzf_lib.lua", "Lua/fzf_lib.lua", NULL};
  int expected[] = {80, 0, 0, 0, 80};
  score_wrapper("'src | ^Lua", input, expected);
}

TEST(score_integration, complex_term) {
  char *input[] = {"lua/random_previewer", "README.md",
                   "previewers/utils.lua", "previewers/buffer.lua",
                   "previewers/term.lua",  NULL};
  int expected[] = {0, 0, 328, 328, 0};
  score_wrapper(".lua$ 'previewer !'term", input, expected);
}

static void pos_wrapper(char *pattern, char **input, int **expected) {
  fzf_slab_t *slab = fzf_make_default_slab();
  fzf_pattern_t *pat = fzf_parse_pattern(case_smart, false, pattern, true);
  for (size_t i = 0; input[i] != NULL; ++i) {
    fzf_position_t *pos = fzf_get_positions(input[i], pat, slab);
    ASSERT_EQUAL_MEM(expected[i], pos->data, pos->size);
    fzf_free_positions(pos);
  }
  fzf_free_pattern(pat);
  fzf_free_slab(slab);
}

TEST(pos_integration, simple) {
  char *input[] = {"src/fzf.c",       "src/fzf.h",
                   "lua/fzf_lib.lua", "lua/telescope/_extensions/fzf.lua",
                   "README.md",       NULL};
  int match1[] = {6, 5, 4};
  int match2[] = {6, 5, 4};
  int match3[] = {6, 5, 4};
  int match4[] = {28, 27, 26};
  int *expected[] = {match1, match2, match3, match4, NULL};
  pos_wrapper("fzf", input, expected);
}

TEST(pos_integration, invert) {
  char *input[] = {"fzf", "main.c", "src/fzf", "fz/noooo", NULL};
  int *expected[] = {};
  pos_wrapper("!fzf", input, expected);
}

TEST(pos_integration, and_with_second_invert) {
  char *input[] = {"src/fzf.c", "lua/fzf_lib.lua", "build/libfzf", NULL};
  int match1[] = {6, 5, 4};
  int *expected[] = {match1, NULL, NULL};
  pos_wrapper("fzf !lib", input, expected);
}

TEST(pos_integration, and_all_invert) {
  char *input[] = {"src/fzf.c", "README.md", "lua/asdf", "test/test.c", NULL};
  int *expected[] = {};
  pos_wrapper("!fzf !test", input, expected);
}

TEST(pos_integration, with_escaped_space) {
  char *input[] = {"file ", "file lua", "lua", NULL};
  int match1[] = {7, 6, 5, 4, 3, 2, 1, 0};
  int *expected[] = {NULL, match1, NULL};
  pos_wrapper("file\\ lua", input, expected);
}

TEST(pos_integration, only_escaped_space) {
  char *input[] = {"file with space", "lul lua", "lua", "src", "test", NULL};
  int match1[] = {4};
  int match2[] = {3};
  int *expected[] = {match1, match2, NULL, NULL, NULL};
  pos_wrapper("\\ ", input, expected);
}

TEST(pos_integration, simple_or) {
  char *input[] = {"src/fzf.h",       "README.md",       "build/fzf",
                   "lua/fzf_lib.lua", "Lua/fzf_lib.lua", NULL};
  int match1[] = {0, 1, 2};
  int match2[] = {0, 1, 2};
  int *expected[] = {match1, NULL, NULL, NULL, match2};
  pos_wrapper("'src | ^Lua", input, expected);
}

TEST(pos_integration, complex_term) {
  char *input[] = {"lua/random_previewer", "README.md",
                   "previewers/utils.lua", "previewers/buffer.lua",
                   "previewers/term.lua",  NULL};
  int match1[] = {16, 17, 18, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8};
  int match2[] = {17, 18, 19, 20, 0, 1, 2, 3, 4, 5, 6, 7, 8};
  int *expected[] = {NULL, NULL, match1, match2, NULL};
  pos_wrapper(".lua$ 'previewer !'term", input, expected);
}

int main(int argc, char **argv) {
  exam_init(argc, argv);
  return exam_run();
}
