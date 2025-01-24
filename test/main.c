#include "../src/fzf.h"
#include <stdio.h>

int main(void)
{
  fzf_slab_t *slab = fzf_make_default_slab();
  /* fzf_case_mode enum : CaseSmart = 0, CaseIgnore, CaseRespect
  * normalize bool     : always set to false because its not implemented yet.
  *                      This is reserved for future use
  * pattern char*      : pattern you want to match. e.g. "src | lua !.c$
  * fuzzy bool         : enable or disable fuzzy matching
  */
  fzf_pattern_t *pattern = fzf_parse_pattern(CaseSmart, false, "fzf", true);

  /* you can get the score/position for as many items as you want */
  char *line1 = "src/fzf";
  int score = fzf_get_score(line1, pattern, slab);
  printf("%s score %d\n", line1, score);

  fzf_free_pattern(pattern);
  fzf_free_slab(slab);
}
