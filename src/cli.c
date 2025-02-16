#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fzf.h"

// sorting
typedef struct {
  char *str;
  int32_t score;
  size_t len;
} fzf_tuple_t;

typedef struct {
  fzf_tuple_t *data;
  size_t len;
  size_t cap;
} heap_t;

heap_t *new_heap() {
  heap_t *res = (heap_t *)malloc(sizeof(heap_t));
  res->cap = 512;
  res->data = malloc(sizeof(fzf_tuple_t) * res->cap);
  res->len = 0;
  return res;
}

void free_heap(heap_t *heap) {
  free(heap->data);
  free(heap);
}

static size_t parent(size_t i) {
  return (i - 1) / 2;
}

static size_t left_child(size_t i) {
  return 2 * i + 1;
}

static size_t right_child(size_t i) {
  return 2 * i + 2;
}

void swap(fzf_tuple_t *x, fzf_tuple_t *y) {
  fzf_tuple_t temp = *x;
  *x = *y;
  *y = temp;
}

static bool smaller(fzf_tuple_t *lhs, fzf_tuple_t *rhs) {
  return (lhs->score == rhs->score && lhs->len > rhs->len) ||
         lhs->score < rhs->score;
}

bool greater(fzf_tuple_t *lhs, fzf_tuple_t *rhs) {
  return (lhs->score == rhs->score && lhs->len < rhs->len) ||
         lhs->score > rhs->score;
}

static void insert(heap_t *heap, fzf_tuple_t data) {
  if (heap->len >= heap->cap) {
    heap->cap *= 2;
    heap->data = realloc(heap->data, sizeof(fzf_tuple_t) * heap->cap);
  }
  size_t i = heap->len;
  heap->data[heap->len++] = data;
  while (i != 0 && smaller(&heap->data[parent(i)], &heap->data[i])) {
    swap(&heap->data[parent(i)], &heap->data[i]);
    i = parent(i);
  }
}

static void max_heapify(heap_t *heap, size_t i) {
  size_t left = left_child(i);
  size_t right = right_child(i);
  size_t largest = i;

  if (left <= heap->len && greater(&heap->data[left], &heap->data[largest])) {
    largest = left;
  }

  if (right <= heap->len && greater(&heap->data[right], &heap->data[largest])) {
    largest = right;
  }

  if (largest != i) {
    swap(&heap->data[i], &heap->data[largest]);
    max_heapify(heap, largest);
  }
}

static fzf_tuple_t extract_max(heap_t *heap) {
  fzf_tuple_t max_item = heap->data[0];
  heap->data[0] = heap->data[--heap->len];
  max_heapify(heap, 0);
  return max_item;
}

int main(int argc, char **argv) {
  fzf_slab_t *slab = fzf_make_default_slab();
  fzf_pattern_t *pattern = fzf_parse_pattern(CaseSmart, false, argv[1], true);
  heap_t *heap = new_heap();

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, stdin)) != -1) {
    line[read - 1] = '\0';
    int32_t score = fzf_get_score(line, pattern, slab);
    if (score > 0) {
      char *copy = (char *)malloc(sizeof(char) * read);
      strncpy(copy, line, read - 1);
      copy[read - 1] = '\0';
      insert(heap, (fzf_tuple_t){.str = copy, .score = score, .len = read - 1});
    }
  }

  while (heap->len > 0) {
    fzf_tuple_t el = extract_max(heap);
    printf("%s\n", el.str);
    free(el.str);
  }
  free_heap(heap);
  fzf_free_pattern(pattern);
  fzf_free_slab(slab);
  free(line);
  return 0;
}
