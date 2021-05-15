local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf_lib.lua') * -1)
local library_path = dirname .. '../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
  typedef struct {} fzf_i16_t;
  typedef struct {} fzf_i32_t;
  typedef struct {
    fzf_i16_t I16;
    fzf_i32_t I32;
  } fzf_slab_t;

  typedef struct {} fzf_term_set_t;
  typedef struct {
    fzf_term_set_t **ptr;
    size_t size;
    size_t cap;
  } fzf_pattern_t;
  typedef struct {
    uint32_t *data;
    size_t size;
    size_t cap;
  } fzf_position_t;

  fzf_position_t *fzf_get_positions(char *text, fzf_pattern_t *pattern, fzf_slab_t *slab);
  void fzf_free_positions(fzf_position_t *pos);
  int32_t fzf_get_score(char *text, fzf_pattern_t *pattern, fzf_slab_t *slab);

  fzf_pattern_t *fzf_parse_pattern(int32_t case_mode, bool normalize, char *pattern);
  void fzf_free_pattern(fzf_pattern_t *pattern);

  fzf_slab_t *fzf_make_default_slab(void);
  void fzf_free_slab(fzf_slab_t *slab);
]]

local fzf = {}

fzf.get_score = function(input, pattern_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  return native.fzf_get_score(text, pattern_struct, slab)
end

fzf.get_pos = function(input, pattern_struct, slab)
  local text = ffi.new("char[?]", #input + 1)
  ffi.copy(text, input)
  local pos = native.fzf_get_positions(text, pattern_struct, slab)
  local res = {}
  for i = 1, tonumber(pos.size) do
    res[i] = pos.data[i - 1] + 1
  end
  native.fzf_free_positions(pos)

  return res
end

fzf.parse_pattern = function(pattern, case_mode)
  case_mode = case_mode == nil and 0 or case_mode
  local c_str = ffi.new("char[?]", #pattern + 1)
  ffi.copy(c_str, pattern)
  return native.fzf_parse_pattern(case_mode, false, c_str)
end

fzf.free_pattern = function(p)
  native.fzf_free_pattern(p)
end

fzf.allocate_slab = function()
  return native.fzf_make_default_slab()
end

fzf.free_slab = function(s)
  native.fzf_free_slab(s)
end

return fzf
