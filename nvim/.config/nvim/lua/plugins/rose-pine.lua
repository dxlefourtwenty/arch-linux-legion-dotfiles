return {
  "rose-pine/neovim",
  name = "rose-pine",
  lazy = false,
  priority = 1000,
  opts = {
    variant = "main",
    dark_variant = "main",
    disable_background = true,
    styles = {
      bold = true,
      italic = true,
    },
  },
  config = function(_, opts)
    require("rose-pine").setup(opts)

    local group = vim.api.nvim_create_augroup("RosePineTransparency", { clear = true })
    vim.api.nvim_create_autocmd("ColorScheme", {
      group = group,
      pattern = "rose-pine",
      callback = function()
        vim.api.nvim_set_hl(0, "Normal", { bg = "none" })
        vim.api.nvim_set_hl(0, "NormalFloat", { bg = "none" })
        vim.api.nvim_set_hl(0, "SignColumn", { bg = "none" })
        vim.api.nvim_set_hl(0, "EndOfBuffer", { bg = "none" })
      end,
    })
  end,
}
