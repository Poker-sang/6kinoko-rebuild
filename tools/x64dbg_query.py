"""Call dynamically exposed x64dbg MCP tools without printing connector secrets."""

import argparse
import json
from pathlib import Path
import tomllib
import urllib.request


def call_tool(name, arguments=None):
    with (Path.home() / ".codex/config.toml").open("rb") as stream:
        config = tomllib.load(stream)["mcp_servers"]["x64dbg"]
    payload = {"jsonrpc": "2.0", "id": 1, "method": "tools/call",
               "params": {"name": name, "arguments": arguments or {}}}
    if name == "tools/list":
        payload = {"jsonrpc": "2.0", "id": 1, "method": "tools/list"}
    headers = {**config.get("http_headers", {}), "Content-Type": "application/json",
               "Accept": "application/json, text/event-stream"}
    request = urllib.request.Request(config["url"], json.dumps(payload).encode(), headers)
    with urllib.request.urlopen(request, timeout=55) as response:
        text = response.read().decode()
    if text.startswith("event:") or text.startswith("data:"):
        text = "\n".join(line[5:].strip() for line in text.splitlines() if line.startswith("data:"))
    return json.loads(text)


def result_text(result):
    return "\n".join(item.get("text", "") for item in result.get("result", {}).get("content", []))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tool")
    parser.add_argument("arguments", nargs="?", default="{}")
    args = parser.parse_args()
    result = call_tool(args.tool, json.loads(args.arguments))
    if args.tool == "tools/list":
        for tool in result.get("result", {}).get("tools", []):
            if tool["name"] in {"WaitForPause", "ReadMemory", "GetCallStack", "GetAllRegisters",
                                "Disassemble", "SetBreakpoint", "SetConditionalBreakpoint"}:
                print(json.dumps(tool, ensure_ascii=True))
    else:
        print(json.dumps(result, ensure_ascii=True))


if __name__ == "__main__":
    main()
