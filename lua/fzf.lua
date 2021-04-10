local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf.lua') * -1)
local library_path = dirname .. '../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
int32_t get_match(bool case_sensitive, bool normalize, char *text,
                  char *pattern);
]]

local str1 = 'install.conf.yaml'
local str2 = 'install yam'
local text = ffi.new("char[?]", #str1 + 1)
local pattern = ffi.new("char[?]", #str2 + 1)
ffi.copy(text, str1)
ffi.copy(pattern, str2)

print(native.get_match(false, false, text, pattern))
