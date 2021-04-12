local ffi = require'ffi'

local dirname = string.sub(debug.getinfo(1).source, 2, string.len('/fzf.lua') * -1)
local library_path = dirname .. '../../../build/libfzf.so'

local native = ffi.load(library_path)

ffi.cdef[[
int32_t get_match_bad(bool case_sensitive, bool normalize, char *text,
                  char *pattern);
]]

local get_score = function(input, prompt)
  local text = ffi.new("char[?]", #input + 1)
  local pattern = ffi.new("char[?]", #prompt + 1)
  ffi.copy(text, input)
  ffi.copy(pattern, prompt)

  return native.get_match_bad(false, false, text, pattern)
end

-- local test1 = function()
--   print(get_score('install.conf.yaml', 'install yam'))
-- end

-- local test2 = function(prompt)
--   local scan = require'plenary.scandir'
--   local files = scan.scan_dir(vim.fn.expand('~'), { add_dirs = true })

--   local res = {}
--   for _, v in ipairs(files) do
--     local s = get_score(v, prompt)
--     if s ~= 0 then
--       table.insert(res, { v, s })
--     end
--   end
--   return res
-- end

-- test1()
-- print('fzf', #test2('fzf'))
-- print('fzf.h', #test2('fzf.h'))
-- print('fzf.h$', #test2('fzf.h$'))

local sorters = require('telescope.sorters')

local get_fzf_sorter = function()
  return sorters.Sorter:new{
    scoring_function = function(_, prompt, line)
      if prompt == "" then
        return 0
      end

      local score = get_score(line, prompt)
      if score == 0 then
        return -1
      else
        return score
      end
    end,
  }
end

return require('telescope').register_extension {
  setup = function(_, config)
    config.file_sorter = function()
      return get_fzf_sorter()
    end

    config.generic_sorter = function()
      return get_fzf_sorter()
    end
  end,
  exports = {
    native_fzf_sorter = function()
      return get_fzf_sorter()
    end
  }
}
