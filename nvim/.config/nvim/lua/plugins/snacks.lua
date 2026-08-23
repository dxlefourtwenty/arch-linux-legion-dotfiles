return {
  {
    "nvim-mini/mini.icons",
    opts = {
      file = {
        Makefile = { glyph = "", hl = "MiniIconsYellow" },
      },
      extension = {
        tpp = { glyph = "󰬁", hl = "MiniIconsAzure" },
      },
    },
  },
  {
    "folke/snacks.nvim",
    opts = {
      picker = {
        sources = {
          files = {
            hidden = true,
            no_ignore = true,
          },
        },
      },
    },
  },
}
