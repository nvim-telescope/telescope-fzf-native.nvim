local if_nil = function(x, y)
  if x == nil then return y end
  return x
end

local fzf = require('fzf_lib')
local sorters = require('telescope.sorters')

local case_enum = setmetatable({
  ["smart_case"] = 0,
  ["ignore_case"] = 1,
  ["respect_case"] = 2,
}, {
  __index = function(_, k)
    error(string.format("%s is not a valid case mode", k))
  end,
  __newindex = function() error("Don't set new things") end
})

local get_fzf_sorter = function(opts)
  local case_mode = case_enum[opts.case_mode]
  local post_or = false
  local post_inv = false
  local post_escape = false
  local pattern_obj = nil

  local get_struct = function(self, prompt)
    if pattern_obj then return pattern_obj end

    local struct = self.state.prompt_cache[prompt]
    if not struct then
      struct = fzf.parse_pattern(prompt, case_mode)
      self.state.prompt_cache[prompt] = struct
    end
    return struct
  end

  return sorters.Sorter:new{
    init = function(self)
      self.state.slab = fzf.allocate_slab()
      self.state.prompt_cache = {}
    end,
    destroy = function(self)
      for _, v in pairs(self.state.prompt_cache) do
        fzf.free_pattern(v)
      end
      fzf.free_slab(self.state.slab)
      pattern_obj = nil
    end,
    start = function(self, prompt)
      pattern_obj = get_struct(self, prompt)
      local last = prompt:sub(-1, -1)

      if last == '|' then
        self._discard_state.filtered = {}
        post_or = true
      elseif last == " " and post_or then
        self._discard_state.filtered = {}
      elseif post_or then
        self._discard_state.filtered = {}
        post_or = false
      else
        post_or = false
      end

      if last == '\\' and not post_escape then
        self._discard_state.filtered = {}
        post_escape = true
      else
        self._discard_state.filtered = {}
        post_escape = false
      end

      if last == '!' and not post_inv then
        post_inv = true
        self._discard_state.filtered = {}
      elseif post_inv then
        self._discard_state.filtered = {}
      elseif post_inv and " " then
        post_inv = false
      end
    end,
    finish = function()
      pattern_obj = nil
    end,
    discard = true,
    scoring_function = function(self, prompt, line)
      local obj = get_struct(self, prompt)
      if obj.size == 0 then return 1 end

      local score = fzf.get_score(line, obj, self.state.slab)
      if score == 0 then
        return -1
      else
        return 1 / score
      end
    end,
    highlighter = function(self, prompt, display)
      return fzf.get_pos(display, get_struct(self, prompt), self.state.slab)
    end,
  }
end

return require('telescope').register_extension {
  setup = function(ext_config, config)
    local override_file = if_nil(ext_config.override_file_sorter, true)
    local override_generic = if_nil(ext_config.override_generic_sorter, true)

    local opts = ext_config.case_mode and { case_mode = ext_config.case_mode } or { case_mode = "smart_case" }

    if override_file then
      config.file_sorter = function()
        return get_fzf_sorter(opts)
      end
    end

    if override_generic then
      config.generic_sorter = function()
        return get_fzf_sorter(opts)
      end
    end
  end,
  exports = {
    native_fzf_sorter = function(opts)
      return get_fzf_sorter(opts or { case_mode = "smart_case" })
    end
  }
}
