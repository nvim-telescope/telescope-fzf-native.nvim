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
  } pattern_t;

  typedef struct {
    int32_t *data;
    int32_t size;
    int32_t cap;
  } position_t;

  position_t get_positions(char *text, pattern_t *pattern, slab_t *slab);
  int32_t get_score(char *text, pattern_t *pattern, slab_t *slab);

  pattern_t *parse_pattern(int32_t case_mode, bool normalize, char *pattern);
  void free_pattern(pattern_t *pattern);

  slab_t *make_slab(int32_t size_16, int32_t size_32);
  void free_slab(slab_t *slab);

  void free(void *ptr);
]]

local fzf = {}

fzf.get_score = function(input, pattern_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  return native.get_score(text, pattern_struct, slab)
end

fzf.get_pos = function(input, pattern_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  local pos = native.get_positions(text, pattern_struct, slab)
  local res = {}
  for i = 0, pos.size - 1 do
    res[i + 1] = pos.data[i] + 1
  end
  native.free(pos.data)

  return res
end

fzf.parse_pattern = function(pattern, case_mode)
  case_mode = case_mode == nil and 0 or case_mode
  local c_str = ffi.new("char[?]", #pattern + 1)
  ffi.copy(c_str, pattern)
  return native.parse_pattern(case_mode, false, c_str)
end

fzf.free_pattern = function(p)
  native.free_pattern(p)
end

fzf.allocate_slab = function()
  return native.make_slab(100 * 1024, 2048)
end

fzf.free_slab = function(s)
  native.free_slab(s)
end

return fzf
