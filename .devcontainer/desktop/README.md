# Omarchy desktop mode (headless, browser-viewed)

The Omarchy desktop running inside the codespace, viewed in your browser over
VNC. Because codespace containers have **no GPU and no `/dev/dri` render node**,
Hyprland cannot start here; `omarchy-desktop` auto-detects that and runs a
headless **Sway** session instead. The Omarchy shell (Quickshell) runs on Sway
via the compositor-agnostic **wlr layer-shell** protocol, so you get the real
bar, themed Tokyo Night surface, menu, and panels in the browser.

Verified working end-to-end (June 2026): Sway fallback + Omarchy shell + themed
bar + menu with app list + app launching from inside the VNC desktop.

## How it works

```
Sway (headless, wlroots+pixman, no GPU needed)
    └─ exec_always: quickshell -p /usr/share/omarchy/shell  (Omarchy desktop)
                └─ theme: Tokyo Night applied BEFORE compositor starts
                    └─ background: Omarchy's own wallpaper (swaybg removed)
                        └→ wayvnc (:5900) → websockify+noVNC (:6080) → browser
```

```
Sway (fallback, always — no DRM render node in codespaces) ─┐
                                                            ├→ wayvnc → noVNC → browser
Omarchy shell (Quickshell, layer-shell, on Sway)  ──────────┘
```

Everything is software-rendered and streamed, so it looks right but is laggy
(~5–15 fps). Demo, not a daily driver.

> **Why not Hyprland?** Hyprland 0.56+ (Aquamarine) hard-requires a DRM render
> node (`/dev/dri/renderD*`) to build its GBM allocator. Codespaces blocks DRM
> access entirely (`CBackend::create() failed: no allocator available`).
> Fallback Sway (wlroots + pixman) needs no GPU and always comes up.
>
> **Omarchy is not Hyprland.** It's one long-running Quickshell instance
> (`quickshell -n -p /usr/share/omarchy/shell`) drawing bar/widgets/panels
> through layer-shell. Sway implements layer-shell, so the whole shell runs on
> Sway as on Hyprland. Only Hyprland-specific bits degrade (workspace widget
> shows nothing without `HYPRLAND_INSTANCE_SIGNATURE`), and DBus-dependent
> widgets (notifications, SNI tray, NetworkManager, Bluetooth) are inert in a
> container with no session bus.

> **Theme.** Quickshell reads `~/.local/state/omarchy/current/theme/colors.toml`
> **only at startup** (`watchChanges: false`). If no theme exists the shell
> renders the stock fallback palette. `omarchy-desktop` applies **Tokyo Night**
> *before* the compositor starts, so a fresh shell boots themed (bar `#1a1b26`
> / text `#a9b1d6`, "Winding Road" wallpaper).

## Usage

**One command, every login:**

```bash
omarchy-up
```

It starts the whole pipeline (if not already running), waits until the viewer
answers, and prints a copy-paste browser URL like
`https://<codespace-name>-6080.app.github.dev/vnc.html`. Idempotent — safe to
re-run whenever you log in. Install/update it from the repo with:

```bash
sudo cp .devcontainer/desktop/omarchy-up /usr/local/bin/omarchy-up && sudo chmod +x /usr/local/bin/omarchy-up
```

Manual alternative: run `omarchy-desktop` in a terminal, then open port **6080**
in the VS Code Ports tab (`/vnc.html`; add the path if you land on a blank page).

**Click inside the viewer to grab keyboard/mouse input**, then:

   | Shortcut                        | Action                    |
   |---------------------------------|---------------------------|
   | `Ctrl+Alt+Space`                | Open the Omarchy app menu (*browser-safe*) |
   | `Super+Space` (or `Super+d`)    | Open the Omarchy app menu |
   | `Ctrl+Alt+Enter`                | Open foot terminal (*browser-safe*) |
   | `Super+Enter`                   | Open foot terminal       |
   | `Super+Shift+Escape`            | Quit the Sway session    |

   Use the browser-safe pairs listed first: most host OSes eat `Super`/`Meta`
   (Windows/Spotlight/macOS etc.) before it ever reaches the browser, so
   `Super+Space` often silently fails over noVNC while `Ctrl+Alt+Space` works.

### Launching apps (the reliable way)

The menu's click-to-launch path works (it uses Omarchy's `appLibrary.launch`),
but the most dependable route — and what's actually proven in practice — is to
drive the desktop from a terminal **inside** the VNC session:

1. `Ctrl+Alt+Enter` → foot terminal opens.
2. Run `opencode` in it — a full opencode CLI session runs right inside the
   desktop.
3. Ask it to launch things. Example that works today: *"open chromium"* — the
   app opens as a normal window in Omarchy.

This gives you a real AI-in-the-desktop loop without fighting browser key
grabs, and it keeps working even when noVNC/keystroke handling changes.

### Changing the theme later

```bash
omarchy theme set "Tokyo Night"       # writes current/theme files
# then push to the live shell without a restart:
omarchy-shell shell applyTheme "$(base64 -w0 ~/.local/state/omarchy/current/theme/colors.toml)" \
                            "$(base64 -w0 ~/.local/state/omarchy/current/theme/shell.toml)"
```

Environment for any manual IPC (already set inside `omarchy-desktop`):

```bash
export XDG_RUNTIME_DIR=/tmp/xdg-omarchy WAYLAND_DISPLAY=wayland-1
export SWAYSOCK=/tmp/xdg-omarchy/sway-ipc.1000.22278.sock   # or: swaymsg -t get_sockets
```

## Files

| Path | What it is |
|---|---|
| `.devcontainer/desktop/omarchy-desktop` | Source script; installs to `/usr/local/bin/omarchy-desktop` |
| `.devcontainer/desktop/omarchy-up` | One-shot launcher; installs to `/usr/local/bin/omarchy-up` |
| `~/.config/sway/omarchy-headless.conf` | Generated Sway config (bindings + `exec_always` Quickshell) |
| `~/.local/state/omarchy/current/theme/` | Tokyo Night files: `colors.toml`, `shell.toml`, `backgrounds/0-winding-road.jpg` |
| `/tmp/omarchy-quickshell.log` | Omarchy shell (Quickshell) stdout/stderr |
| Logs | `/tmp/sway.out`, `/tmp/wayvnc.log`, `/tmp/websockify.log` |

## Troubleshooting

| Symptom | What to check |
|---|---|
| "no compositor came up" | `cat /tmp/hyprland.log`, `cat /tmp/sway.out` |
| Black/gray screen in viewer | `cat /tmp/wayvnc.log`; confirm port 6080 forwarded |
| noVNC page won't load | Check `/tmp/websockify.log`; Ports tab → open 6080 |
| Keys do nothing in viewer | Click inside the viewer first (input grab). If it's a `Super` combo, use the `Ctrl+Alt` variant (host OS eats Meta) |
| View shows the Sway logo wallpaper | `omarchy-desktop` no longer sets `output * bg`; if you hand-edited configs, ensure that line is absent so Omarchy's background plugin (Winding Road) owns the wallpaper |
| Bar has stock/fallback grey colors | No theme was applied before shell start. `omarchy-desktop` applies Tokyo Night automatically; manually: `omarchy theme set "Tokyo Night"` then restart, or push with `applyTheme` above |

## Honest limits

- Laggy (software rendering + VNC). Fine for "seeing it" / scripting, not daily
  driving.
- `Super`/`Meta` keybindings are unreliable from a browser (host grabs the key);
  use the `Ctrl+Alt` aliases or launch from a foot terminal.
- DBus widgets (notifications, tray, NetworkManager, Bluetooth) are inert.
- No GPU, no sound.