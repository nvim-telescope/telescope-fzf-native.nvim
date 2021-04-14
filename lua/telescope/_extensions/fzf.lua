local if_nil = function(x, y)
  if x == nil then return y end
  return x
end

local fzf = require('fzf')
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

  local get_struct = function(self, prompt)
    local struct = self.state.prompt_cache[prompt]
    if not struct then
      struct = fzf.parse_prompt(prompt, case_mode)
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
        fzf.free_prompt(v)
      end
      fzf.free_slab(self.state.slab)
    end,
    discard = false,
    scoring_function = function(self, prompt, line)
      if prompt == "" then
        return 1
      end

      local score = fzf.get_score(line, get_struct(self, prompt), self.state.slab)
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
    native_fzf_sorter = function()
      return get_fzf_sorter()
    end
  }
}
