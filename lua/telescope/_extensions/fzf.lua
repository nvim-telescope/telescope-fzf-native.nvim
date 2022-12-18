local fzf = require "fzf_lib"
local sorters = require "telescope.sorters"

local case_enum = setmetatable({
  ["smart_case"] = 0,
  ["ignore_case"] = 1,
  ["respect_case"] = 2,
}, {
  __index = function(_, k)
    error(string.format("%s is not a valid case mode", k))
  end,
  __newindex = function()
    error "Don't set new things"
  end,
})

local get_fzf_sorter = function(opts)
  local case_mode = case_enum[opts.case_mode]
  local fuzzy_mode = opts.fuzzy == nil and true or opts.fuzzy
  local post_or = false
  local post_inv = false
  local post_escape = false

  local get_struct = function(self, prompt)
    local struct = self.state.prompt_cache[prompt]
    if not struct then
      struct = fzf.parse_pattern(prompt, case_mode, fuzzy_mode)
      self.state.prompt_cache[prompt] = struct
    end
    return struct
  end

  local clear_filter_fun = function(self, prompt)
    local filter = "^(" .. self._delimiter .. "(%S+)" .. "[" .. self._delimiter .. "%s]" .. ")"
    local matched = prompt:match(filter)

    if matched == nil then
      return prompt
    end
    return prompt:sub(#matched + 1, -1)
  end

  return sorters.Sorter:new {
    init = function(self)
      self.state.slab = fzf.allocate_slab()
      self.state.prompt_cache = {}

      if self.filter_function then
        self.__highlight_prefilter = clear_filter_fun
      end
    end,
    destroy = function(self)
      for _, v in pairs(self.state.prompt_cache) do
        fzf.free_pattern(v)
      end
      self.state.prompt_cache = {}
      if self.state.slab ~= nil then
        fzf.free_slab(self.state.slab)
        self.state.slab = nil
      end
    end,
    start = function(self, prompt)
      local last = prompt:sub(-1, -1)

      if last == "|" then
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

      if last == "\\" and not post_escape then
        self._discard_state.filtered = {}
        post_escape = true
      else
        self._discard_state.filtered = {}
        post_escape = false
      end

      if last == "!" and not post_inv then
        post_inv = true
        self._discard_state.filtered = {}
      elseif post_inv then
        self._discard_state.filtered = {}
      elseif post_inv and " " then
        post_inv = false
      end
    end,
    discard = true,
    scoring_function = function(self, prompt, line)
      local obj = get_struct(self, prompt)
      local score = fzf.get_score(line, obj, self.state.slab)
      if score == 0 then
        return -1
      else
        return 1 / score
      end
    end,
    highlighter = function(self, prompt, display)
      if self.__highlight_prefilter then
        prompt = self:__highlight_prefilter(prompt)
      end
      return fzf.get_pos(display, get_struct(self, prompt), self.state.slab)
    end,
  }
end

local fast_extend = function(opts, conf)
  local ret = {}
  ret.case_mode = vim.F.if_nil(opts.case_mode, conf.case_mode)
  ret.fuzzy = vim.F.if_nil(opts.fuzzy, conf.fuzzy)
  return ret
end

local wrap_sorter = function(conf)
  return function(opts)
    opts = opts or {}
    return get_fzf_sorter(fast_extend(opts, conf))
  end
end

return require("telescope").register_extension {
  setup = function(ext_config, config)
    local override_file = vim.F.if_nil(ext_config.override_file_sorter, true)
    local override_generic = vim.F.if_nil(ext_config.override_generic_sorter, true)

    local conf = {}
    conf.case_mode = vim.F.if_nil(ext_config.case_mode, "smart_case")
    conf.fuzzy = vim.F.if_nil(ext_config.fuzzy, true)

    if override_file then
      config.file_sorter = wrap_sorter(conf)
    end

    if override_generic then
      config.generic_sorter = wrap_sorter(conf)
    end
  end,
  exports = {
    native_fzf_sorter = function(opts)
      return get_fzf_sorter(opts or { case_mode = "smart_case", fuzzy = true })
    end,
  },
  health = function()
    local health = vim.health or require "health"

    local good = true
    local eq = function(expected, actual)
      if tostring(expected) ~= tostring(actual) then
        good = false
      end
    end

    local p = fzf.parse_pattern("fzf", 0)
    local slab = fzf.allocate_slab()

    eq(80, fzf.get_score("src/fzf", p, slab))
    eq(0, fzf.get_score("asdf", p, slab))
    eq(54, fzf.get_score("fasdzasdf", p, slab))

    fzf.free_pattern(p)
    fzf.free_slab(slab)

    if good then
      health.report_ok "lib working as expected"
    else
      health.report_error "lib not working as expected, please reinstall and open an issue if this error persists"
      return
    end

    local has, config = pcall(require, "telescope.config")
    if not has then
      health.report_error "unexpected: telescope configuration couldn't be loaded"
    end

    local test_sorter = function(name, sorter)
      good = true
      sorter:_init()
      local prompt = "fzf !lua"
      eq(1 / 80, sorter:scoring_function(prompt, "src/fzf"))
      eq(-1, sorter:scoring_function(prompt, "lua/fzf"))
      eq(-1, sorter:scoring_function(prompt, "asdf"))
      sorter:_destroy()

      if good then
        health.report_ok(name .. " correctly configured")
      else
        health.report_warn(name .. " is not configured")
      end
    end
    test_sorter("file_sorter", config.values.file_sorter {})
    test_sorter("generic_sorter", config.values.generic_sorter {})
  end,
}
