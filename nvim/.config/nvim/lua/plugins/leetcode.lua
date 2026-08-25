local leetcode_config = {
  lang = "cpp",
  plugins = {
    non_standalone = true,
  },
}

return {
  {
    "kawre/leetcode.nvim",
    cmd = "Leet",
    dependencies = {
      "nvim-lua/plenary.nvim",
      "MunifTanjim/nui.nvim",
      "folke/snacks.nvim",
    },
    opts = leetcode_config,
  },
}
