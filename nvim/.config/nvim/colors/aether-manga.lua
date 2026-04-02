local opts = vim.deepcopy(require("config.themes.aether_manga"))

opts.name = "aether-manga"

require("aether.config").setup(opts)
require("aether").load()
