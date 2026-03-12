-- Clear existing highlights
vim.cmd("highlight clear")
vim.g.colors_name = "dreamwave"

-- Base colors
local bg       = "#0A0716"
local fg       = "#f5eafd"
local accent   = "#8c53b7"
local pink     = "#f1279f"
local purple   = "#c911f8"
local surface  = "#120c24"

-- Core UI
vim.api.nvim_set_hl(0, "Normal", { fg = fg, bg = bg })
vim.api.nvim_set_hl(0, "NormalNC", { fg = fg, bg = bg })
vim.api.nvim_set_hl(0, "EndOfBuffer", { bg = "NONE" })
vim.api.nvim_set_hl(0, "LineNr", { fg = accent })
vim.api.nvim_set_hl(0, "CursorLine", { bg = surface })

-- Syntax
vim.api.nvim_set_hl(0, "Comment", { fg = accent, italic = true })
vim.api.nvim_set_hl(0, "String", { fg = pink })
vim.api.nvim_set_hl(0, "Function", { fg = "#e37bd1" })
vim.api.nvim_set_hl(0, "Keyword", { fg = purple, bold = true })
vim.api.nvim_set_hl(0, "Type", { fg = "#cc58fc" })
vim.api.nvim_set_hl(0, "Identifier", { fg = fg })

-- Visual
vim.api.nvim_set_hl(0, "Visual", { bg = accent, fg = bg })

-- Neo-tree transparency
vim.api.nvim_set_hl(0, "NeoTreeNormal", { bg = "NONE" })
vim.api.nvim_set_hl(0, "NeoTreeNormalNC", { bg = "NONE" })
vim.api.nvim_set_hl(0, "NeoTreeEndOfBuffer", { bg = "NONE" })
vim.api.nvim_set_hl(0, "NeoTreeWinSeparator", { bg = "NONE" })
