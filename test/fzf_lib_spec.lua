local fzf = require("fzf_lib")
local eq = assert.are.same

describe("score", function()
  local slab = fzf.allocate_slab()
  it("can return the score", function()
    local p = fzf.parse_pattern("fzf", 0)
    eq(80, fzf.get_score("fzf", p, slab))
    eq(0, fzf.get_score("asdf", p, slab))
    fzf.free_pattern(p)
  end)
end)
