# telescope-fzf-native.nvim

Is a port of **[fzf][fzf]** to c. It only ports the algorithm and implements a
`get_score` and `get_position` function, so we can use fzf sorting in telescope.

## Installation

You have to manually build the library. We do NOT ship binaries (also it takes
like half a second).

```viml
Plug 'nvim-telescope/telescope-fzf-native.nvim', { 'on': 'make' }
```

You can override the file & generic sorter by default simply by adding

```lua
require('telescope').load_extension('fzf')
```

somewhere after your `require('telescope').setup()` call.

To configure them individually, you should do the following:

```lua
require('telescope').setup {
  extensions = {
    fzf = {
      override_generic_sorter = false,
      override_file_sorter = true,
    }
  }
}
require('telescope').load_extension('fzf')
```

## Credit

All credit for the algorithm goes to junegunn and his work on **[fzf][fzf]**.
This is merely a c fork distributed under MIT for telescope.

[fzf]: https://github.com/junegunn/fzf
