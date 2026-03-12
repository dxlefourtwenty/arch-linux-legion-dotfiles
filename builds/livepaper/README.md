# livepaper (Qt/QML)

Bottom-row live wallpaper picker rebuilt in C++/QML.

- Qt Quick UI with LayerShell anchoring at the bottom.
- Reads theme colors from `~/.config/hellpaper/hellpaper.conf`.
- Loads wallpapers from a directory (default: `~/.config/themes/current/wallpapers`).
- Applies wallpaper command when selection changes.
- Optionally updates a wallpaper symlink.

## Build

```bash
make build
```

This builds `build/livepaper` and copies it to `./livepaper` for launcher compatibility.

## Run

```bash
./livepaper
```

```bash
./livepaper /path/to/wallpapers
```

## Options

- `--switch-cmd <cmd>` command template; use `{}` for selected image path.
- `--link <path>` symlink updated to selected wallpaper.
- `--no-link` skip symlink updates.
- `--output <name>` screen/output name to open on. If omitted, Livepaper uses `~/.config/dashboard/config.json` `outputName`, then falls back to `HDMI-A-1`.

Default switch command:

```bash
swww img --transition-type=grow --transition-duration=1.6 {}
```

You can also set `LIVEPAPER_SWITCH_CMD`.
