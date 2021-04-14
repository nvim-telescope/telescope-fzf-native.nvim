local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf.lua') * -1)
local library_path = dirname .. '../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
  typedef struct {} i16_t;
  typedef struct {} i32_t;
  typedef struct {
    i16_t I16;
    i32_t I32;
  } slab_t;

  typedef struct {} term_set_t;
  typedef struct {
    term_set_t **ptr;
    int32_t size;
    int32_t cap;
  } prompt_t;

  typedef struct {
    int32_t *data;
    int32_t size;
    int32_t cap;
  } position_t;

  position_t get_positions(char *text, prompt_t *prompt, slab_t *slab);
  int32_t get_score(char *text, prompt_t *prompt, slab_t *slab);

  prompt_t *parse_terms(int32_t case_mode, bool normalize, char *pattern);
  void free_prompt(prompt_t *prompt);

  slab_t *make_slab(int32_t size_16, int32_t size_32);
  void free_slab(slab_t *slab);

  void free(void *ptr);
]]

local fzf = {}

fzf.get_score = function(input, prompt_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  return native.get_score(text, prompt_struct, slab)
end

fzf.get_pos = function(input, prompt_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  local pos = native.get_positions(text, prompt_struct, slab)
  local res = {}
  for i = 0, pos.size - 1 do
    res[i + 1] = pos.data[i] + 1
  end
  native.free(pos.data)

  return res
end

fzf.parse_prompt = function(prompt, case_mode)
  case_mode = case_mode == nil and 0 or case_mode
  local pattern = ffi.new("char[?]", #prompt + 1)
  ffi.copy(pattern, prompt)
  return native.parse_terms(case_mode, false, pattern)
end

fzf.free_prompt = function(p)
  native.free_prompt(p)
end

fzf.allocate_slab = function()
  return native.make_slab(100 * 1024, 2048)
end

fzf.free_slab = function(s)
  native.free_slab(s)
end

return fzf
