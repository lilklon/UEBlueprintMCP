# UE Blueprint MCP

A robust MCP (Model Context Protocol) server for Unreal Engine 5.7+ Blueprint manipulation with persistent connections and crash protection.

## Features

- **Persistent TCP Connections**: Socket stays open between commands (no reconnect per command)
- **Crash Protection**: SEH on Windows, defensive programming everywhere
- **Stateful Editor Context**: Track current Blueprint, graph, and recently created objects
- **Auto-Save**: Dirty packages saved automatically after successful operations
- **Comprehensive Error Reporting**: Clear error messages with error types

## Architecture

### Python Side (`Python/`)
- `ue_blueprint_mcp/` - Main package
  - `server.py` - MCP server entry point
  - `connection.py` - PersistentUnrealConnection class
  - `tools/` - Tool modules (blueprint, editor, nodes, project, umg)

### C++ Plugin (`UnrealPlugin/UEBlueprintMCP/`)
- Editor subsystem with TCP bridge
- Action-based command handling with crash protection
- Modular architecture for maintainability

## Setup

### Python MCP Server

1. Create virtual environment:
```bash
cd Python
python -m venv venv
venv\Scripts\activate  # Windows
pip install -r requirements.txt
```

2. Configure MCP in your project's `.mcp.json`:
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

### Unreal Plugin

1. Copy `UnrealPlugin/UEBlueprintMCP/` to your project's `Plugins/` folder
2. Regenerate project files
3. Build in Visual Studio

## Usage

The MCP server exposes tools for:

- **Editor**: Actor spawning, viewport control, save operations
- **Blueprint**: Create/compile blueprints, add components, set properties
- **Nodes**: Event nodes, function calls, variables, graph operations
- **Project**: Input mappings, Enhanced Input system
- **UMG**: Widget Blueprint creation and manipulation

## Development Status

Phase 0: Project Setup ✅
Phase 1: Port Core Infrastructure 🔄
Phase 2: Safe Execution (coming)
Phase 3: Persistent Connection (coming)
Phase 4: Port Command Handlers (coming)
Phase 5: Integration & Cleanup (coming)

## License

MIT
