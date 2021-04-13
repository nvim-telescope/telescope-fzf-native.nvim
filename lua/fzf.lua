local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf.lua') * -1)
local library_path = dirname .. '../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
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
    i16_t I16;
    i32_t I32;
  } slab_t;

  typedef struct {
    int32_t typ;
    bool inv;
    char *og_str;
    char *text;
    bool case_sensitive;
    bool normalize;
  } term_t;

  typedef struct {
    term_t *ptr;
    int32_t size;
    int32_t cap;
  } term_set_t;

  typedef struct {
    term_set_t **ptr;
    int32_t size;
    int32_t cap;
  } term_set_sets_t;

  typedef struct {
    int32_t *data;
    int32_t size;
    int32_t cap;
  } position_t;

  position_t get_positions(char *text, term_set_sets_t *sets, slab_t *slab);
  int32_t get_match(char *text, term_set_sets_t *sets, slab_t *slab);
  int32_t get_match_bad(bool case_sensitive, bool normalize, char *text,
                        char *pattern);

  term_set_sets_t *build_pattern_fun(bool case_sensitive, bool normalize,
                                     char *pattern);
  void free_sets(term_set_sets_t *sets);

  slab_t *make_slab(int32_t size_16, int32_t size_32);
  void free_slab(slab_t *slab);
]]

local fzf = {}

fzf.get_score_bad = function(input, prompt)
  local text = ffi.new("char[?]", #input + 1)
  local pattern = ffi.new("char[?]", #prompt + 1)
  ffi.copy(text, input)
  ffi.copy(pattern, prompt)

  return native.get_match_bad(false, false, text, pattern)
end

fzf.get_score = function(input, prompt_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  return native.get_match(text, prompt_struct, slab)
end

fzf.get_pos = function(input, prompt_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  local pos = native.get_positions(text, prompt_struct, slab)
  local res = {}
  for i = 0, pos.size - 1 do
    res[i + 1] = pos.data[i] + 1
  end
  ffi.C.free(pos.data)

  return res
end

fzf.parse_prompt = function(prompt)
  local pattern = ffi.new("char[?]", #prompt + 1)
  ffi.copy(pattern, prompt)
  return native.build_pattern_fun(false, false, pattern)
end

fzf.free_prompt = function(p)
  native.free_sets(p)
end

fzf.allocate_slab = function()
  return native.make_slab(100 * 1024, 2048)
end

fzf.free_slab = function(s)
  native.free_slab(s)
end

return fzf
