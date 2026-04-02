local M = {
  ["aether-manga"] = {
    plugin = "aether",
    colorscheme = "aether-manga",
  },
  ristretto = {
    plugin = "monokai-pro",
    colorscheme = "monokai-pro-ristretto",
  },
}

function M.resolve(theme)
  return M[theme] or {
    plugin = theme,
    colorscheme = theme,
  }
end

return M
