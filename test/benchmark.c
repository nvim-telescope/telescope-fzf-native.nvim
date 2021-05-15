#include "fzf.h"

#include <curl/curl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <math.h>

#define RUNS 1000

static double get_time(void) {
  struct timeval t;
  struct timezone tzp;
  gettimeofday(&t, &tzp);
  return t.tv_sec + t.tv_usec * 1e-6;
}

static double mean(double *a) {
  double sum = 0;
  size_t count = 0;

  for (; count < RUNS; count++) {
    sum += a[count];
  }

  return (sum / count);
}

static void swap(double *lhs, double *rhs) {
  double t = *lhs;
  *lhs = *rhs;
  *rhs = t;
}

static double median(double *a) {
  double copy[RUNS];
  memcpy(copy, a, sizeof(copy));

  for (size_t i = 0; i < RUNS - 1; i++) {
    for (size_t j = 0; j < RUNS - i - 1; j++) {
      if (copy[j] > copy[j + 1]) {
        swap(&copy[j], &copy[j + 1]);
      }
    }
  }

  size_t n = (RUNS + 1) / 2 - 1;
  return copy[n];
}

static double stdDeviation(double *a) {
  double m = mean(a), vm = 0, sum = 0;
  size_t count = 0;

  for (; count < RUNS; count++) {
    vm = a[count] - m;
    sum += (vm * vm);
  }

  return sqrt(sum / (count - 1));
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct {
  double min;
  double max;
} min_max_t;

static min_max_t minmax(double *a) {
  double min = DBL_MAX, max = DBL_MIN;
  for (size_t i = 0; i < RUNS; i++) {
    max = MAX(max, a[i]);
    min = MIN(min, a[i]);
  }

  return (min_max_t){.min = min, .max = max};
}

typedef struct {
  char *data;
  size_t size;
} fzf_string_t;

typedef struct {
  fzf_string_t *data;
  size_t cap;
  size_t len;
} str_arr_t;

static void free_str_array(str_arr_t *array) {
  for (size_t i = 0; i < array->len; i++) {
    if (array->data[i].data) {
      free(array->data[i].data);
    }
  }
  free(array->data);
}

static void init_string(fzf_string_t *s) {
  s->size = 0;
  s->data = malloc((s->size + 1) * sizeof(char));
  s->data[0] = '\0';
}

static size_t writefunc(void *ptr, size_t size, size_t nmemb, fzf_string_t *s) {
  size_t new_len = s->size + size * nmemb;
  s->data = realloc(s->data, new_len + 1);

  memcpy(s->data + s->size, ptr, size * nmemb);
  s->data[new_len] = '\0';
  s->size = new_len;

  return size * nmemb;
}

static int get_test_file(str_arr_t *array) {
  CURL *curl;
  CURLcode res = 1;

  curl = curl_easy_init();
  if (curl) {
    fzf_string_t s;
    init_string(&s);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");

    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://raw.githubusercontent.com/nlohmann/json/develop/"
                     "single_include/nlohmann/json.hpp");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    char *ptr = strtok(s.data, "\n");
    while (ptr != NULL) {
      size_t len = strlen(ptr);
      if (array->len + 1 < array->cap) {
        fzf_string_t *new = &array->data[array->len];
        new->size = len;
        new->data = (char *)malloc((len + 1) * sizeof(char));
        strcpy(new->data, ptr);
        new->data[len] = '\0';
        array->len++;
      }
      ptr = strtok(NULL, "\n");
    }

    free(s.data);
  }

  return res;
}

int main(int argc, char *argv[]) {
  double results[RUNS];
  memset(results, 0, sizeof(results));
  fzf_slab_t *slab = fzf_make_default_slab();

  str_arr_t file;
  file.len = 0;
  // lets just allocate 30000 lines for now. Its not production code
  file.cap = 30000;
  file.data = malloc(file.cap * sizeof(fzf_string_t));
  for (size_t i = 0; i < file.cap; i++) {
    memset(&file.data[i], 0, sizeof(fzf_string_t));
  }
  get_test_file(&file);

  char *patterns[] = {"size_t", "'int | 'short", "^include", "define",
                      "std::vector"};
  size_t patterns_size = 4;

  double main_start = get_time();

  for (size_t i = 0; i < RUNS; i++) {
    double start = get_time();
    for (size_t k = 0; k < patterns_size; k++) {
      char *pat_str = patterns[k];
      fzf_pattern_t *pattern = fzf_parse_pattern(case_smart, false, pat_str);
      for (size_t j = 0; j < file.len; j++) {
        fzf_get_score(file.data[j].data, pattern, slab);
      }
      fzf_free_pattern(pattern);
    }
    results[i] = get_time() - start;
  }
  printf("Total elapsed time: %fs, (%d RUNS, %zu line file, %zu pattern "
         "matched)\n",
         get_time() - main_start, RUNS, file.len, patterns_size);
  min_max_t mm = minmax(results);
  printf("min: %fs , max: %fs, mean: %fs, median: %fs, std: %fs\n", mm.min,
         mm.max, mean(results), median(results), stdDeviation(results));

  fzf_free_slab(slab);
  free_str_array(&file);
  return 0;
}
