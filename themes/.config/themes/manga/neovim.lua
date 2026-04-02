local theme = "aether-manga" -- just change this per theme file
local resolved = require("config.theme_aliases").resolve(theme)

pcall(function()
  require("lazy").load({ plugins = { resolved.plugin } })
end)

vim.cmd.colorscheme(resolved.colorscheme)
