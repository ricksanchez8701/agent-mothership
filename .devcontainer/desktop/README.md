# Omarchy desktop mode (headless, browser-viewed)

A tech demo: run the actual **Hyprland** desktop (Omarchy's bar, themes, rounded
windows) inside the codespace and view it in your browser over VNC.

## How it works

```
Hyprland (WLR_BACKENDS=headless, llvmpipe software rendering)
   → wayvnc (VNC server, :5900)
   → websockify + noVNC (web viewer, :6080)
   → your browser (forwarded port 6080 → /vnc.html)
```

There is **no GPU** in a codespace, so everything is software-rendered and
streamed. It will look right but feel laggy (~5–15 fps, input latency). It's a
demo, not a daily driver.

## Usage

1. Open a terminal in the codespace (you should be the `codespace` user).
2. Run:

   ```bash
   omarchy-desktop
   ```

3. In VS Code: **Ports** tab → port **6080** → open in browser (it opens
   `/vnc.html`; if you land on a blank page, add `/vnc.html` yourself).
4. Press a key / click inside the viewer to grab input. The Omarchy bar should
   be at the top; Hyprland hotkeys work (check the Omarchy manual).

To stop: `pkill -f wayvnc; pkill -f websockify; hyprctl dispatch exit` — or just
rebuild the codespace.

## Troubleshooting

| Symptom | What to check |
|---|---|
| "Hyprland did not come up" | `cat /tmp/hyprland.log` — most often a config error from the Hyprland session |
| Black/gray screen in viewer | Known Hyprland+wayvnc flakiness on headless outputs. `cat /tmp/wayvnc.log`, try `hyprctl output create headless` again, or use `hyprctl keyword monitor` to change resolution |
| noVNC page won't load | Check `/tmp/websockify.log`; confirm port 6080 is forwarded (Ports tab) |
| Nothing responds in viewer | Click inside the viewer first (VNC input grab), then try Hyprland hotkeys |

## Honest limits

- Laggy (software rendering + VNC). Fine for "seeing it", not for real work.
- If the wayvnc screen is black with your Hyprland version, the fallback paths
  are Omarchy's official remote guides (RDP or Sunshine) — see the Omarchy
  manual's remote-access sections — or just run Omarchy on real hardware,
  where this all works natively and smoothly.
