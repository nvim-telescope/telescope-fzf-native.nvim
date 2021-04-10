local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf.lua') * -1)
local library_path = dirname .. '../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
int32_t calculate_score(bool case_sensitive, bool normalize, char *text,
                        char *pattern);
]]

local str = 'hello'
local text = ffi.new("char[?]", #str + 1)
local pattern = ffi.new("char[?]", #str + 1)
ffi.copy(text, str)
ffi.copy(pattern, str)

print(native.calculate_score(false, false, text, pattern))
