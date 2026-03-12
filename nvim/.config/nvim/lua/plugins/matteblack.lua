return {
  {
    "tahayvr/matteblack.nvim",
    lazy = false,
    priority = 1000,
  },
  {
    "LazyVim/LazyVim",
    opts = {
      colorscheme = "matteblack",
    },
    config = function(_, opts)
      require("lazyvim.config").setup(opts)

      local function clear_bg()
        local groups = {
          "Normal",
          "NormalNC",
          "NormalFloat",
          "FloatBorder",
          "SignColumn",
          "FoldColumn",
          "LineNr",
          "EndOfBuffer",
          "MsgArea",
          "MsgSeparator",
          "StatusLine",
          "StatusLineNC",
          "WinSeparator",
          "NormalSB",
          "SignColumnSB",
          "EndOfBufferSB",
        }

        for _, g in ipairs(groups) do
          vim.api.nvim_set_hl(0, g, { bg = "none" })
        end
      end

      -- apply globally
      vim.api.nvim_create_autocmd({ "ColorScheme", "VimEnter" }, {
        callback = clear_bg,
      })

      -- 🔥 THIS IS THE IMPORTANT PART 🔥
      -- clear window-local winhighlight for sidebars
      vim.api.nvim_create_autocmd("WinEnter", {
        callback = function()
          local ft = vim.bo.filetype
          if ft == "neo-tree" or ft == "NvimTree" or ft == "snacks" then
            vim.wo.winhighlight = ""
          end
        end,
      })
    end,
  },
}
