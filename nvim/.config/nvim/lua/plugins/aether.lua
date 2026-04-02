return {
  {
    "bjarneo/aether.nvim",
    branch = "v2",
    name = "aether",
    priority = 1000,
    opts = require("config.themes.aether"),
    config = function(_, opts)
      require("aether").setup(opts)
      require("aether.hotreload").setup()
    end,
  },
}
