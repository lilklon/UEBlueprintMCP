# UEBlueprintMCP

A Model Context Protocol (MCP) server for AI-assisted Unreal Engine 5.7+ Blueprint manipulation with persistent TCP connections and auto-save.

## Features

- **60+ MCP Commands** for Blueprints, Materials, Widgets, Enhanced Input, and Editor control
- **Persistent TCP Connection** - Socket stays open between commands (port 55558)
- **Auto-Save** - Dirty packages saved automatically after every successful operation
- **Crash Protection** - All operations flow through a validation/execution pipeline
- **String-Based Resolution** - Accept Blueprint names, asset paths, or engine class names

## Supported Operations

| Category | Commands |
|----------|----------|
| **Blueprints** | Create, compile, add components, set properties |
| **Graph Nodes** | Events, functions, variables, branches, casts, spawns |
| **Event Dispatchers** | Create, bind, and call multicast delegates |
| **Enhanced Input** | Create Input Actions, Mapping Contexts, key bindings |
| **Materials** | Create materials, add expressions, connect nodes, post-process |
| **UMG Widgets** | Create widgets, add text/buttons, bind events |
| **Editor** | Spawn actors, viewport control, save operations |

## Setup

### 1. Install the Unreal Plugin

Copy or symlink `UnrealPlugin/UEBlueprintMCP/` to your project's `Plugins/` folder, then rebuild.

### 2. Set Up the Python MCP Server

```bash
cd Python
python -m venv venv
venv\Scripts\activate  # Windows
pip install -r requirements.txt
```

### 3. Configure MCP Client

Add to your project's `.mcp.json`:

```json
{
  "mcpServers": {
    "unreal-mcp": {
      "command": "path/to/UEBlueprintMCP/Python/venv/Scripts/python.exe",
      "args": ["-m", "ue_blueprint_mcp.server"]
    }
  }
}
```

### 4. Launch

1. Open your Unreal project (plugin starts TCP server on port 55558)
2. Connect your MCP client (e.g., Claude Code with `/mcp`)

## Architecture

```
MCP Client (Claude Code, etc.)
    |
    v  MCP Protocol
Python Server (ue_blueprint_mcp)
    |
    v  TCP/JSON (port 55558, persistent)
C++ Plugin (MCPBridge)
    |
    v  Action dispatch
FEditorAction subclasses
    |
    v  Validate -> Execute -> Auto-Save
Unreal Editor
```

All editor operations flow through `FEditorAction` subclasses that provide:
- Pre-execution validation
- Graceful error handling with descriptive messages
- Automatic dirty package tracking and save

## Documentation

- **[CLAUDE.md](CLAUDE.md)** - Architecture details and contribution guidelines
- **[docs/SKILL.md](docs/SKILL.md)** - Full command reference for Claude Code

## Requirements

- Unreal Engine 5.7+
- Python 3.10+
- Visual Studio 2022 (for plugin builds)

## License

MIT
