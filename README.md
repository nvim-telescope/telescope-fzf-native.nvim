``# telescope-fzf-native.nvim

**fzf-native** is a `c` port of **[fzf][fzf]**. It only covers the algorithm and
implements few functions to support calculating the score.

This means that the [fzf syntax](https://github.com/junegunn/fzf#search-syntax)
is supported:

| Token     | Match type                 | Description                          |
| --------- | -------------------------- | ------------------------------------ |
| `sbtrkt`  | fuzzy-match                | Items that match `sbtrkt`            |
| `'wild`   | exact-match (quoted)       | Items that include `wild`            |
| `^music`  | prefix-exact-match         | Items that start with `music`        |
| `.mp3$`   | suffix-exact-match         | Items that end with `.mp3`           |
| `!fire`   | inverse-exact-match        | Items that do not include `fire`     |
| `!^music` | inverse-prefix-exact-match | Items that do not start with `music` |
| `!.mp3$`  | inverse-suffix-exact-match | Items that do not end with `.mp3`    |

A single bar character term acts as an OR operator. For example, the following
query matches entries that start with `core` and end with either `go`, `rb`,
or `py`.

```
^core go$ | rb$ | py$
```

This is an advantage over the more simpler `fzy` algorithm, which is also
available for telescope (as native component or as lua component).

## Installation

`telescope-fzf-native` is mostly a native binary. We do not commit this to the
repo, so you'll need to include a post install step to get it downloaded from
our github releases.

```lua
use {
  'nvim-telescope/telescope-fzf-native.nvim',
  run = function() require('telescope-fzf-native').download_library() end
}
```

Normally this tries to detect your operating system and defaults to downloading
the latest version.

For other package managers, you'll want to look at the postinstall step:

- [`packer.nvim`](https://github.com/wbthomason/packer.nvim) wants `run`
- [`lazy.nvim`](https://github.com/folke/lazy.nvim) will want you to use `build`
- [`vimplug`](https://github.com/junegunn/vim-plug) will probably want some kind `do` involving `:lua` :shrug:


```vim
Plug 'nvim-telescope/telescope-fzf-native.nvim', {
  'do': ':lua require("telescope-fzf-native").download_library()'
}
```

### download options

```lua
use {
  'nvim-telescope/telescope-fzf-native.nvim',
  run = function()
    require('telescope-fzf-native').download_library({
        platform = 'windows' -- windows | ubuntu | macos
        compiler = 'cc', -- windows: cc, unix: gcc | clang
        version = 'latest' -- any release name found on our github releases page
    })
  end
}
```

> 🤚 Note
>
> You'll need to have both `curl` and `sh` shell installed.
>
> On windows, this is done by installing git, and on linux and mac this should already be solved.



## Building yourself

If you want to build **fzf-native** yourself, you will need either `cmake` or `make`.

### CMake (Windows, Linux, MacOS)

This requires:

- CMake, and the Microsoft C++ Build Tools (vcc) on Windows
- CMake, make, and GCC or Clang on Linux and MacOS

#### vim-plug

```viml
Plug 'nvim-telescope/telescope-fzf-native.nvim', { 'do': 'cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release && cmake --install build --prefix build' }
```

#### packer.nvim


```lua
use {'nvim-telescope/telescope-fzf-native.nvim', run = 'cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release && cmake --install build --prefix build' }
```

### Make (Linux, MacOS, Windows with MinGW)

This requires `gcc` or `clang` and `make`

#### vim-plug

```viml
Plug 'nvim-telescope/telescope-fzf-native.nvim', { 'do': 'make' }
```

#### packer.nvim

```lua
use {'nvim-telescope/telescope-fzf-native.nvim', run = 'make' }
```

## Telescope Setup and Configuration:

```lua
-- You dont need to set any of these options. These are the default ones. Only
-- the loading is important
require('telescope').setup {
  extensions = {
    fzf = {
      fuzzy = true,                    -- false will only do exact matching
      override_generic_sorter = true,  -- override the generic sorter
      override_file_sorter = true,     -- override the file sorter
      case_mode = "smart_case",        -- or "ignore_case" or "respect_case"
                                       -- the default case_mode is "smart_case"
    }
  }
}
-- To get fzf loaded and working with telescope, you need to call
-- load_extension, somewhere after setup function:
require('telescope').load_extension('fzf')
```

## Developer Interface

This section is only addressed towards developers who plan to use the library
(c or lua bindings).
This section is not addressed towards users of the telescope extension.

### C Interface

```c
fzf_slab_t *slab = fzf_make_default_slab();
/* fzf_case_mode enum : CaseSmart = 0, CaseIgnore, CaseRespect
 * normalize bool     : always set to false because its not implemented yet.
 *                      This is reserved for future use
 * pattern char*      : pattern you want to match. e.g. "src | lua !.c$
 * fuzzy bool         : enable or disable fuzzy matching
 */
fzf_pattern_t *pattern = fzf_parse_pattern(CaseSmart, false, "src | lua !.c$", true);

/* you can get the score/position for as many items as you want */
int score = fzf_get_score(line, pattern, slab);
fzf_position_t *pos = fzf_get_positions(line, pattern, slab);

fzf_free_positions(pos);
fzf_free_pattern(pattern);
fzf_free_slab(slab);
```

### Lua Interface

```lua
local fzf = require('fzf_lib')

local slab = fzf.allocate_slab()
-- pattern: string
-- case_mode: number with 0 = smart_case, 1 = ignore_case, 2 = respect_case
-- fuzzy: enable or disable fuzzy matching. default true
local pattern_obj = fzf.parse_pattern(pattern, case_mode, fuzzy)

-- you can get the score/position for as many items as you want
-- line: string
-- score: number
local score = fzf.get_score(line, pattern_obj, slab)

-- table (does not have to be freed)
local pos = fzf.get_pos(line, pattern_obj, slab)

fzf.free_pattern(pattern_obj)
fzf.free_slab(slab)
```

## Disclaimer

This projects implements **[fzf][fzf]** algorithm in c. So there might be
differences in matching. I don't guarantee completeness.

### TODO

Stuff still missing that is present in **[fzf][fzf]**.

- [ ] normalize
- [ ] case for unicode (i don't think this works currently)

## Benchmark

Comparison with fzy-native and fzy-lua with a table containing 240201 file
strings. It calculated the score and position (if score > 0) for each of these
strings with the pattern that is listed below:

![benchmark 1](https://raw.githubusercontent.com/wiki/nvim-telescope/telescope.nvim/imgs/bench1.png)
![benchmark 2](https://raw.githubusercontent.com/wiki/nvim-telescope/telescope.nvim/imgs/bench2.png)

## Credit

All credit for the algorithm goes to junegunn and his work on **[fzf][fzf]**.
This is merely a c fork distributed under MIT for telescope.

[fzf]: https://github.com/junegunn/fzf
