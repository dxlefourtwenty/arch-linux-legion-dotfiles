local M = {}

M.options = {
  build_directory = "build",
  configure_args = {},
  parallel = 4,
}

function M.root(buffer)
  local directory = vim.fs.dirname(vim.api.nvim_buf_get_name(buffer))
  local root
  while directory do
    if vim.uv.fs_stat(vim.fs.joinpath(directory, "CMakeLists.txt")) then
      root = directory
    end
    if vim.uv.fs_stat(vim.fs.joinpath(directory, ".git")) then
      break
    end
    local parent = vim.fs.dirname(directory)
    directory = parent ~= directory and parent or nil
  end
  return root
end

function M.build_directory(root)
  return vim.fs.abspath(vim.fs.joinpath(root, M.options.build_directory))
end

function M.root_dir(server)
  return function(buffer, on_dir)
    local root = M.root(buffer)
    if root then
      on_dir(root)
    elseif type(server.root_dir) == "function" then
      server.root_dir(buffer, on_dir)
    else
      on_dir(server.root_dir or vim.fs.root(buffer, server.root_markers or { ".git" }))
    end
  end
end

function M.before_init(_, config)
  local root = config.root_dir
  if root and vim.uv.fs_stat(vim.fs.joinpath(root, "CMakeLists.txt")) then
    config.init_options = config.init_options or {}
    config.init_options.compilationDatabasePath = M.build_directory(root)
  end
end

local function run(command, root, on_success)
  vim.system(
    command,
    { cwd = root, text = true },
    vim.schedule_wrap(function(result)
      local output = (result.stdout or "") .. (result.stderr or "")
      if result.code ~= 0 then
        vim.notify(output, vim.log.levels.ERROR, { title = "CMake" })
        return
      end
      if output:find("Warning") or output:find("Error") then
        vim.notify(output, vim.log.levels.WARN, { title = "CMake" })
      end
      if on_success then
        on_success()
      else
        vim.notify("Build completed: " .. root, vim.log.levels.INFO, { title = "CMake" })
      end
    end)
  )
end

function M.generate(build, dry_run)
  local root = M.root(0)
  if not root then
    vim.notify("No parent CMakeLists.txt found", vim.log.levels.ERROR)
    return
  end
  local directory = M.build_directory(root)
  local configure = { "cmake", "-S", root, "-B", directory }
  vim.list_extend(configure, M.options.configure_args)
  table.insert(configure, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
  local compile = { "cmake", "--build", directory, "--parallel", tostring(M.options.parallel) }
  if dry_run then
    local commands = { table.concat(vim.tbl_map(vim.fn.shellescape, configure), " ") }
    if build then
      table.insert(commands, table.concat(vim.tbl_map(vim.fn.shellescape, compile), " "))
    end
    vim.notify(table.concat(commands, "\n"), vim.log.levels.INFO, { title = "CMake dry run" })
    return
  end
  vim.notify("Configuring: " .. root, vim.log.levels.INFO, { title = "CMake" })
  run(configure, root, function()
    if build then
      run(compile, root)
    else
      vim.notify("Generated compiler settings: " .. directory, vim.log.levels.INFO, { title = "CMake" })
    end
  end)
end

function M.setup(options)
  M.options = vim.tbl_deep_extend("force", M.options, options or {})
  vim.env.CMAKE_EXPORT_COMPILE_COMMANDS = "ON"
  vim.api.nvim_create_user_command("CMakeGenerate", function(args)
    M.generate(false, args.bang)
  end, { bang = true, desc = "Configure the root CMake project (! previews commands)" })
  vim.api.nvim_create_user_command("CMakeBuild", function(args)
    M.generate(true, args.bang)
  end, { bang = true, desc = "Configure and build the root CMake project (! previews commands)" })
end

return M
