return {
  { "catppuccin/nvim", name = "catppuccin" },
  { "shaunsingh/nord.nvim", name = "nord" },
  { "folke/tokyonight.nvim", name = "tokyonight" },
  { "ribru17/bamboo.nvim", name = "bamboo" },
  { "neanias/everforest-nvim", name = "everforest" },
  { "ellisonleao/gruvbox.nvim", name = "gruvbox" },
  { "rose-pine/neovim", name = "rose-pine" },
  { "maxmx03/solarized.nvim", name = "solarized" },
  { "Mofiqul/dracula.nvim", name = "dracula" },
  { "tahayvr/matteblack.nvim", name = "matteblack" },
  { "bjarneo/firesky.nvim", name = "firesky" },
  { "gthelding/monokai-pro.nvim", name = "monokai-pro" },

  {
    "LazyVim/LazyVim",
    opts = function(_, opts)
      opts.colorscheme = nil
    end,
  },
}
