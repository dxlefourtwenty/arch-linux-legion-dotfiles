return {
  "shaunsingh/nord.nvim",
  lazy = true,          -- do not load on startup
  priority = 1000,      -- still fine for themes
  config = function()
    -- ONLY set globals / options here
    -- Do NOT call colorscheme
    vim.g.nord_contrast = true
    vim.g.nord_borders = true
    vim.g.nord_disable_background = true
  end,
}
