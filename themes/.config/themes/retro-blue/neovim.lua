local theme = "retro-82"  -- just change this per theme file

pcall(function()
  require("lazy").load({ plugins = { theme } })
end)

vim.cmd.colorscheme(theme)
