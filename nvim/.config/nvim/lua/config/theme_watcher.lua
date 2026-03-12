local uv = vim.uv or vim.loop

local theme_file = vim.fn.stdpath("config") .. "/lua/config/current_theme.lua"

local last_mtime = 0

local function apply()
  print("THEME UPDATED")
  local ok, err = pcall(dofile, theme_file)
  if not ok then
    vim.notify("Theme reload failed: " .. err, vim.log.levels.ERROR)
  end
end

local function check()
  local stat = uv.fs_stat(theme_file)
  if stat and stat.mtime.sec ~= last_mtime then
    last_mtime = stat.mtime.sec
    apply()
  end
end

-- initial load
local stat = uv.fs_stat(theme_file)
if stat then
  last_mtime = stat.mtime.sec
  apply()
end

-- poll every 300ms
local timer = uv.new_timer()
timer:start(300, 300, function()
  vim.schedule(check)
end)
