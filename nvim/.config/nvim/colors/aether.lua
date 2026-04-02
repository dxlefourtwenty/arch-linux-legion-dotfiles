local opts = vim.deepcopy(require("config.themes.aether"))

opts.name = "aether"

require("aether.config").setup(opts)
require("aether").load()
