local MODREV, SPECREV = 'scm', '-1'
rockspec_format = '3.0'
package = 'telescope-fzf-native.nvim'
version = MODREV .. SPECREV

description = {
  summary = 'FZF sorter for telescope written in c',
  detailed = [[
fzf-native is a c port of fzf.
It only covers the algorithm and implements few functions to support calculating the score.
]],
  labels = { 'neovim', 'plugin', },
  homepage = 'https://github.com/nvim-telescope/telescope-fzf-native.nvim',
  license = 'MIT',
}

dependencies = {
  'lua == 5.1',
  'telescope.nvim',
}

source = {
  url = 'https://github.com/nvim-telescope/telescope-fzf-native.nvim/archive/refs/tags/' .. MODREV .. '.zip',
  dir = 'telescope-fzf-native.nvim-' .. MODREV
}

if MODREV == 'scm' then
  source = {
    url = 'git://github.com/nvim-telescope/telescope-fzf-native.nvim',
  }
end

build = {
  type = 'make',
  build_pass = false,
  install_variables = {
    INST_PREFIX='$(PREFIX)',
    INST_BINDIR='$(BINDIR)',
    INST_LIBDIR='$(LIBDIR)',
    INST_LUADIR='$(LUADIR)',
    INST_CONFDIR='$(CONFDIR)',
  },
}
