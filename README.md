# telescope-fzf-native.nvim

**fzf-native** is a `c` port of **[fzf][fzf]**. It only covers the algorithm and
implements few functions to support calculating the score.

## Installation

To get **fzf-native** working, you need to run make at the root directory. As of
now, we do not ship binaries.

### vim-plug

```viml
Plug 'nvim-telescope/telescope-fzf-native.nvim', { 'do': 'make' }
```

### packer.nvim

```lua
use {'nvim-telescope/telescope-fzf-native.nvim', run = 'make' }
```

## Telescope Setup and Configuration:

```lua
require('telescope').setup {
  extensions = {
    fzf = {
      override_generic_sorter = false, -- override the generic sorter
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
slab_t *slab = make_default_slab();
/* case_mode enum: case_smart = 0, case_ignore, case_respect
 * normalize bool: always set to false because its not implemented yet. This
 *                 is reserved for future use
 * pattern char* : pattern you want to match. e.g. "src | lua !.c$
 */
pattern_t *pattern = parse_pattern(case_smart, false, "src | lua !.c$");

/* you can get the score/position for as many items as you want */
get_score(line, pattern, slab);
get_positions(line, pattern, slab);

free_pattern(pattern);
free_slab(slab);
```

### Lua Interface

```lua
local fzf = require('fzf_lib')

local slab = fzf.allocate_slab()
-- pattern: string
-- case_mode: number with 0 = smart_case, 1 = ignore_case, 2 = respect_case
local pattern_obj = fzf.parse_pattern(pattern, case_mode)

-- you can get the score/position for as many items as you want
-- line: string
fzf.get_score(line, pattern_obj, slab)
fzf.get_pos(line, pattern_obj, slab)

fzf.free_pattern(pattern_obj)
fzf.free_slab(slab)
```

## Disclaimer

This project is work in progress and might not be complete compared to
**[fzf][fzf]**. But i think we are at a pretty usable state already.

Another point to mention is that, this will not compare 1:1 with
**[fzf][fzf]**, because telescope handles the sorting, this extension is
only handling the calculation of the score. This means that the order of items
with the same score might be different in telescope compared to **[fzf][fzf]**.

Example for this behaivor is this pattern in this repository: `src | lua`.
All files that match this pattern have the same score which results in a
slightly different order for telescope compared to **[fzf][fzf]**.

### TODO

Stuff still missing that is present in fzf.

- [ ] normalize
- [ ] case for unicode (i don't think this works currently)
- [ ] and probably more

## Credit

All credit for the algorithm goes to junegunn and his work on **[fzf][fzf]**.
This is merely a c fork distributed under MIT for telescope.

[fzf]: https://github.com/junegunn/fzf
