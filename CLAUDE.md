# UEBlueprintMCP - Project Context for Claude

## Overview

UEBlueprintMCP is a Model Context Protocol (MCP) server that enables AI-assisted manipulation of Unreal Engine 5.7+ Blueprints, Materials, Widgets, and Editor state through a persistent TCP connection.

## Repository Structure

```
UEBlueprintMCP/
├── Python/                          # MCP server (Python)
│   └── ue_blueprint_mcp/
│       ├── server.py                # Entry point, MCP server setup
│       ├── connection.py            # PersistentUnrealConnection (TCP)
│       └── tools/                   # Tool definitions by category
│           ├── blueprint.py         # Blueprint creation, compilation
│           ├── editor.py            # Actor manipulation, viewport, save
│           ├── nodes.py             # Graph nodes (events, functions, variables)
│           ├── project.py           # Enhanced Input assets
│           ├── umg.py               # Widget Blueprints
│           └── materials.py         # Materials, expressions, post-process
│
├── UnrealPlugin/UEBlueprintMCP/     # Editor plugin (C++)
│   └── Source/UEBlueprintMCP/
│       ├── Public/
│       │   ├── MCPBridge.h          # UEditorSubsystem, command router
│       │   ├── MCPServer.h          # TCP server
│       │   ├── MCPContext.h         # Stateful editor context
│       │   ├── MCPCommonUtils.h     # Shared utilities
│       │   └── Actions/             # Action class headers
│       │       ├── EditorAction.h   # Base classes (FEditorAction hierarchy)
│       │       ├── BlueprintActions.h
│       │       ├── EditorActions.h
│       │       ├── NodeActions.h
│       │       ├── ProjectActions.h
│       │       ├── UMGActions.h
│       │       └── MaterialActions.h
│       └── Private/
│           ├── MCPBridge.cpp        # Action registration & dispatch
│           ├── MCPServer.cpp        # TCP listener, JSON parsing
│           ├── MCPContext.cpp       # Context state, auto-save
│           ├── MCPCommonUtils.cpp   # Blueprint/asset resolution
│           └── Actions/             # Action implementations
│               ├── EditorAction.cpp # Base class + execution pipeline
│               ├── BlueprintActions.cpp
│               ├── EditorActions.cpp
│               ├── NodeActions.cpp
│               ├── ProjectActions.cpp
│               ├── UMGActions.cpp
│               └── MaterialActions.cpp
│
└── docs/
    └── SKILL.md                     # Claude Code skill file (reference doc)
```

## Architecture

### Execution Flow

```
MCP Client (Claude Code)
    ↓ MCP protocol
Python Server (server.py)
    ↓ TCP socket (port 55558, persistent)
C++ Plugin (MCPServer → MCPBridge)
    ↓ command dispatch
FEditorAction subclass
    ↓ Validate() → ExecuteInternal() → PostValidate()
Response + Auto-Save
```

### The FEditorAction Pipeline

ALL Unreal Editor operations MUST flow through the `FEditorAction` base class execution pipeline. This ensures:

1. **Validation** - Parameters checked before touching editor state
2. **Error Handling** - Graceful failures with descriptive messages
3. **Auto-Save** - Dirty packages saved after every successful action
4. **Crash Protection** - SEH wrappers (Windows) prevent editor crashes

```cpp
// FEditorAction::Execute() pipeline:
1. Validate(Params, Context, OutError)     // Pre-check params & state
2. ExecuteInternal(Params, Context)        // Do the actual work
3. PostValidate(Context, OutError)         // Verify results (optional)
4. Context.SaveDirtyPackages()             // Auto-save on success
```

### Action Class Hierarchy

```
FEditorAction                    (base - validation, error handling, save)
├── FBlueprintAction             (Blueprint resolution, modification marking)
│   └── FBlueprintNodeAction     (Graph access, node registration)
├── FMaterialAction              (Material resolution, context tracking)
└── [Direct FEditorAction]       (Actor, viewport, project actions)
```

## Contribution Guidelines

### 1. ALL Editor Operations Flow Through FEditorAction

**CRITICAL:** Never directly manipulate editor state outside the action pipeline. Every command must:

- Inherit from the appropriate action base class
- Implement `Validate()` to check preconditions
- Implement `ExecuteInternal()` for the actual operation
- Return proper success/error responses via `CreateSuccessResponse()` or `CreateErrorResponse()`

The pipeline handles:
- Crash protection (SEH wrapper on Windows)
- Automatic dirty package tracking
- Auto-save after successful operations
- Consistent error formatting

```cpp
// CORRECT: Use the pipeline
class FMyAction : public FBlueprintNodeAction
{
    bool Validate(...) override { /* check params */ }
    TSharedPtr<FJsonObject> ExecuteInternal(...) override {
        // Do work here
        MarkBlueprintModified(Blueprint, Context);
        return CreateSuccessResponse(ResultData);
    }
};

// WRONG: Direct manipulation
void DoSomething() {
    UBlueprint* BP = FindBlueprint("MyBP");
    BP->Modify();  // NO! Not tracked, not validated, won't auto-save
}
```

### 2. Extend Existing Commands Before Creating New Ones

When adding functionality, **prefer adding parameters to existing commands** over creating new specific commands:

```cpp
// GOOD: Extended existing command with new parameter
// add_blueprint_function_node now supports `graph_name` parameter
// to work on function graphs, not just event graph

// BAD: Creating a separate command
// add_blueprint_function_node_to_function_graph  // DON'T DO THIS
```

This keeps the API surface manageable and reduces duplication.

### 3. Keep Files Under 1000 Lines

Large files become difficult to navigate and maintain. Current file sizes:

| File | Lines | Status |
|------|-------|--------|
| NodeActions.cpp | ~1800 | NEEDS SPLIT |
| MaterialActions.cpp | ~1560 | NEEDS SPLIT |
| BlueprintActions.cpp | ~1090 | NEEDS SPLIT |
| EditorActions.cpp | ~636 | OK |
| UMGActions.cpp | ~611 | OK |
| ProjectActions.cpp | ~388 | OK |
| EditorAction.cpp | ~378 | OK |

When a file exceeds 1000 lines, split it by creating a new action category file (e.g., `NodeActions.cpp` could split off `DispatcherActions.cpp` or `FunctionGraphActions.cpp`).

### 4. Error Messages Must Be Descriptive

Return clear error messages that help diagnose issues:

```cpp
// GOOD: Specific, actionable error
OutError = FString::Printf(TEXT("Blueprint '%s' not found. Check spelling or create it first."), *BlueprintName);

// BAD: Vague error
OutError = TEXT("Operation failed");
```

Error types should indicate the category:
- `validation_failed` - Parameter or precondition issue
- `not_found` - Asset/node/pin doesn't exist
- `execution_failed` - Operation threw exception
- `crash_prevented` - SEH caught access violation

### 5. Python Tool Docstrings

Every Python tool must have a docstring that will be shown to MCP clients:

```python
@mcp.tool()
async def my_command(ctx: Context, blueprint_name: str, param: str) -> str:
    """
    Brief description of what this command does.

    Args:
        blueprint_name: Target Blueprint
        param: Description of this parameter

    Returns:
        JSON with node_id on success
    """
    return await send_command_async("my_command", {...})
```

### 6. Register Actions in MCPBridge.cpp

After creating a new action class, register it in `MCPBridge::RegisterActions()`:

```cpp
void UMCPBridge::RegisterActions()
{
    // ... existing registrations ...

    // Add your new action:
    ActionHandlers.Add(TEXT("my_new_command"), MakeShared<FMyNewAction>());
}
```

## Common Patterns

### Creating Blueprint Nodes

Use `FEdGraphSchemaAction_K2NewNode::SpawnNode<T>()` - it handles all the boilerplate:

```cpp
#include "EdGraphSchema_K2_Actions.h"

// Simple node:
UK2Node_IfThenElse* Node = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_IfThenElse>(
    Graph, Position, EK2NewNodeFlags::None
);

// Node with initialization (lambda runs BEFORE pin allocation):
UK2Node_DynamicCast* CastNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_DynamicCast>(
    Graph, Position, EK2NewNodeFlags::None,
    [TargetClass](UK2Node_DynamicCast* Node) {
        Node->TargetType = TargetClass;
    }
);
```

### String-Based Class Resolution

Accept user-friendly strings and resolve them:

```cpp
// In FMCPCommonUtils or your action:
UBlueprint* BP = FMCPCommonUtils::FindBlueprint(BlueprintName);  // "BP_Enemy"
UClass* Class = LoadClass<UObject>(nullptr, TEXT("/Script/Engine.Actor"));  // Engine class
```

### Using Helper Methods

```cpp
// Parameter extraction
FString Name;
if (!GetRequiredString(Params, TEXT("name"), Name, OutError)) return false;
FString Optional = GetOptionalString(Params, TEXT("graph_name"));
FVector2D Position = GetNodePosition(Params);

// Blueprint/Graph access (after validation)
UBlueprint* BP = GetTargetBlueprint(Params, Context);
UEdGraph* Graph = GetTargetGraph(Params, Context);

// State management
MarkBlueprintModified(BP, Context);
RegisterCreatedNode(Node, Context);
```

## Testing Changes

After modifying the C++ plugin:

1. Close Unreal Editor
2. Build: `UnrealBuildTool.exe GravityGokuEditor Win64 Development -project="path/to/project.uproject"`
3. Launch Unreal Editor
4. Reconnect MCP (`/mcp` in Claude Code)
5. Test the modified command

## UE 5.7 Compatibility Notes

Changes made for UE 5.7 compatibility:
- `ANY_PACKAGE` → `nullptr` (removed in 5.7)
- `FImageUtils::CompressImageArray` → `PNGCompressImageArray` with `TArray64<uint8>`
- `.uplugin` `WhitelistPlatforms` → `PlatformAllowList`
- Global `BufferSize` renamed to `MCPRecvBufferSize` (name conflict)

## Mac Compatibility Notes

- **TCP_NODELAY required**: Python socket must set `TCP_NODELAY` to avoid 200-500ms latency from Nagle + macOS Delayed ACK interaction.
- **HasPendingData before Recv**: C++ must check `HasPendingData()` before peek recv; Mac returns errors on blocking recv with no data.
