#!/usr/bin/env python3
"""LightLang Playground - a local web UI for the LightLang compiler.

Serves a single page on 127.0.0.1 where you can write LightLang source and see
every compiler phase at once: tokens, AST, symbol and function tables, the
three-address IR, the optimized IR, the target bytecode, and the program
output.

It shells out to the real lightlang executable, so what you see here is exactly
what the compiler produces on the command line.

Usage:
    python tools/playground.py [--port 8000] [--no-browser]
    make ui

Only the Python standard library is used; nothing needs installing.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "playground.html")

# Section banners printed by main.cpp, in pipeline order.
SECTION_HEADERS = [
    "TOKENS",
    "LEXICAL ERRORS",
    "SYNTAX ERRORS",
    "ABSTRACT SYNTAX TREE",
    "SEMANTIC ERRORS",
    "SYMBOL TABLE",
    "FUNCTION TABLE",
    "THREE-ADDRESS INTERMEDIATE CODE",
    "OPTIMIZED THREE-ADDRESS CODE",
    "TARGET BYTECODE",
    "PROGRAM OUTPUT",
    "BYTECODE FILE",
]

ERROR_SECTIONS = {"LEXICAL ERRORS", "SYNTAX ERRORS", "SEMANTIC ERRORS"}

SEPARATOR = re.compile(r"^-{10,}$")

# A runaway LightLang program (or a deliberate infinite loop) must not hang the
# server, and must not fill memory with output.
RUN_TIMEOUT_SECONDS = 10
MAX_OUTPUT_CHARS = 400_000
MAX_SOURCE_CHARS = 200_000


def find_compiler():
    """Locate the built compiler, preferring the platform-native name."""
    for name in ("lightlang.exe", "lightlang"):
        candidate = os.path.join(REPO_ROOT, name)
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate

    found = shutil.which("lightlang")
    return found


def split_sections(text):
    """Split raw compiler output into {header: body} using the banner layout.

    Each section is printed as a header line, a dashed separator, the body, and
    a closing dashed separator.
    """
    sections = {}
    lines = text.splitlines()
    index = 0

    while index < len(lines):
        header = lines[index].strip()

        if header in SECTION_HEADERS:
            index += 1

            # Skip the opening separator when present.
            if index < len(lines) and SEPARATOR.match(lines[index].strip()):
                index += 1

            body = []
            while index < len(lines) and not SEPARATOR.match(lines[index].strip()):
                body.append(lines[index])
                index += 1

            # Consume the closing separator.
            if index < len(lines):
                index += 1

            # A phase can appear twice (for example PROGRAM OUTPUT); keep the
            # richest capture rather than letting a later empty one win.
            existing = sections.get(header, "")
            captured = "\n".join(body)
            if len(captured.strip()) >= len(existing.strip()):
                sections[header] = captured
            continue

        index += 1

    return sections


def extract_status(text):
    """Pull the compiler's own summary lines out of the raw output."""
    notes = []
    for line in text.splitlines():
        stripped = line.strip()
        if (
            stripped.startswith("Compilation stopped:")
            or stripped.startswith("Program execution stopped:")
            or stripped.startswith("Internal compiler error:")
            or stripped.startswith("Bytecode file error:")
            or stripped.startswith("Runtime error")
            or stripped == "Compilation completed successfully."
            or stripped == "Program execution completed successfully."
        ):
            notes.append(stripped)
    return notes


def run_source(source):
    """Compile and execute LightLang source, returning a structured result."""
    compiler = find_compiler()
    if compiler is None:
        return {
            "ok": False,
            "fatal": "Could not find the lightlang executable. Build it first with 'make'.",
        }

    if len(source) > MAX_SOURCE_CHARS:
        return {"ok": False, "fatal": "Source is too large for the playground."}

    # The system temp directory is fine here: Python hands back a native path,
    # which the compiler executable understands directly.
    handle, path = tempfile.mkstemp(suffix=".lw", prefix="playground_")
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as source_file:
            source_file.write(source)

        try:
            completed = subprocess.run(
                [compiler, "run", path],
                capture_output=True,
                text=True,
                timeout=RUN_TIMEOUT_SECONDS,
                cwd=REPO_ROOT,
            )
        except subprocess.TimeoutExpired:
            return {
                "ok": False,
                "timeout": True,
                "fatal": (
                    f"Execution exceeded {RUN_TIMEOUT_SECONDS} seconds and was stopped. "
                    "The program most likely contains an endless loop."
                ),
            }

        raw = (completed.stdout or "") + (completed.stderr or "")
        truncated = len(raw) > MAX_OUTPUT_CHARS
        if truncated:
            raw = raw[:MAX_OUTPUT_CHARS] + "\n... output truncated ...\n"

        sections = split_sections(raw)

        return {
            "ok": True,
            "exitCode": completed.returncode,
            "sections": sections,
            "notes": extract_status(raw),
            "hasErrors": any(name in sections for name in ERROR_SECTIONS),
            "truncated": truncated,
            "raw": raw,
        }
    finally:
        try:
            os.remove(path)
        except OSError:
            pass


class PlaygroundHandler(BaseHTTPRequestHandler):
    # Keep the console readable; the UI reports its own errors.
    def log_message(self, fmt, *args):
        pass

    def _send(self, status, body, content_type):
        payload = body if isinstance(body, bytes) else body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            try:
                with open(PAGE_PATH, "rb") as page:
                    self._send(200, page.read(), "text/html; charset=utf-8")
            except OSError:
                self._send(500, "playground.html is missing", "text/plain; charset=utf-8")
            return

        if self.path == "/api/examples":
            self._send(200, json.dumps(load_examples()), "application/json; charset=utf-8")
            return

        self._send(404, "Not found", "text/plain; charset=utf-8")

    def do_POST(self):
        if self.path != "/api/run":
            self._send(404, "Not found", "text/plain; charset=utf-8")
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            length = 0

        try:
            request = json.loads(self.rfile.read(length).decode("utf-8") or "{}")
        except (ValueError, UnicodeDecodeError):
            self._send(400, json.dumps({"ok": False, "fatal": "Malformed request."}),
                       "application/json; charset=utf-8")
            return

        result = run_source(request.get("source", ""))
        self._send(200, json.dumps(result), "application/json; charset=utf-8")


def load_examples():
    """Expose the repository's example programs to the UI."""
    examples = []
    directory = os.path.join(REPO_ROOT, "examples")

    if os.path.isdir(directory):
        for name in sorted(os.listdir(directory)):
            if not name.endswith(".lw"):
                continue
            try:
                with open(os.path.join(directory, name), encoding="utf-8") as handle:
                    examples.append({"name": name, "source": handle.read()})
            except OSError:
                continue

    return examples


def main():
    parser = argparse.ArgumentParser(description="LightLang playground server")
    parser.add_argument("--port", type=int, default=8000, help="port to listen on")
    parser.add_argument("--no-browser", action="store_true",
                        help="do not open a browser automatically")
    arguments = parser.parse_args()

    if find_compiler() is None:
        print("warning: lightlang executable not found. Run 'make' first.", file=sys.stderr)

    # Bind to loopback only. The server runs the compiler on whatever it is
    # sent, so it must never be reachable from the network.
    server = ThreadingHTTPServer(("127.0.0.1", arguments.port), PlaygroundHandler)
    url = f"http://127.0.0.1:{arguments.port}/"

    print("LightLang Playground")
    print(f"  serving {url}")
    print("  press Ctrl+C to stop")

    if not arguments.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
