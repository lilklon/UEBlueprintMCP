---
name: unreal-bp
description: "Use this skill when working with Unreal Engine via MCP, including: blueprint creation, node manipulation, spawning actors, connecting pins, graph operations, viewport control, editor automation, widget UMG, shader/material creation, post-process effects, component setup, variable management, event handling, function calls, or dispatcher setup. Also use when the user mentions blueprint, node, spawn, connect, actor, graph, viewport, editor, widget, shader, material, post-process, designer, or component in relation to Unreal Engine."
---

# Unreal Engine 5.7+ Blueprint Node Skill

This skill provides specialized knowledge for working with Unreal Engine Blueprint nodes via **UEBlueprintMCP** - a Model Context Protocol plugin with persistent TCP connection and modular action-based architecture.

**Key Features:**
- Persistent socket connection (no reconnect overhead per command)
- Central command handler with unified validation/execution pipeline
- Auto-save after successful actions
- String-based class resolution (accepts blueprint names, paths, or engine class names)

## MCP Tools Available for Blueprint Manipulation

### Blueprint Creation & Management
- `create_blueprint` - Create new Blueprint class (supports parent: Actor, Pawn, PlayerController, GameModeBase, GameStateBase)
- `compile_blueprint` - Compile and auto-save; returns error_count, warning_count, detailed errors with node IDs
- `find_blueprint_nodes` - List all nodes in a blueprint (returns node_guid, node_class, node_title); supports `graph_name` for function graphs
- `delete_blueprint_node` - Remove a node by GUID; supports `graph_name` for function graphs
- `get_node_pins` - Debug tool: list all pins on a node; supports `graph_name` for function graphs

### Components
- `add_component_to_blueprint` - Add StaticMeshComponent, BoxComponent, SphereComponent, SceneComponent, CameraComponent
- `set_component_property` - Set properties on components
- `set_static_mesh_properties` - Set mesh, material, and overlay_material on StaticMeshComponent
- `set_physics_properties` - Configure physics simulation

### Variables
- `add_blueprint_variable` - Add variable (Boolean, Integer, Float, Vector, String)
- `add_blueprint_variable_get` - Create getter node for variable
- `add_blueprint_variable_set` - Create setter node for variable

### Event Nodes
- `add_blueprint_event_node` - Standard events (ReceiveBeginPlay, ReceiveTick)
- `add_blueprint_custom_event` - Custom event with optional parameters [{name, type}]
- `add_blueprint_input_action_node` - Legacy input action events
- `add_enhanced_input_action_node` - Enhanced Input events (Started/Triggered/Ongoing/Canceled/Completed pins + ActionValue)

### Function Nodes
- `add_blueprint_function_node` - Call function on target (use StaticClass name for static functions)
- `add_blueprint_self_reference` - Get reference to self
- `add_blueprint_get_self_component_reference` - Get reference to owned component
- `add_blueprint_get_subsystem_node` - Get typed subsystem from PlayerController (K2Node_GetSubsystemFromPC)
- `add_blueprint_cast_node` - Dynamic cast to Blueprint or Engine class
- `add_blueprint_branch_node` - If/Then/Else branch
- `add_spawn_actor_from_class_node` - Spawn actor at runtime (accepts blueprint name like 'BP_Enemy' or class name)
- `call_blueprint_function` - Call custom Blueprint function (target_blueprint must be compiled)

### Blueprint Function Graphs
- `create_blueprint_function` - Create function graph with inputs/outputs, returns entry_node_id and result_node_id
- All node commands support `graph_name` parameter to add nodes to function graphs instead of event graph

### Pin & Connection Management
- `connect_blueprint_nodes` - Wire two nodes together (source_node_id, source_pin, target_node_id, target_pin); supports `graph_name`
- `set_node_pin_default` - Set default value; handles object pins by loading assets with StaticLoadObject

### Event Dispatchers
- `add_event_dispatcher` - Create multicast delegate with optional parameters
- `call_event_dispatcher` - Create broadcast node
- `bind_event_dispatcher` - Create bind node + matching custom event automatically

### Enhanced Input System
- `create_input_action` - Create IA asset (Boolean, Axis1D, Axis2D, Axis3D)
- `create_input_mapping_context` - Create IMC asset
- `add_key_mapping_to_context` - Map key to action with modifiers (Negate, SwizzleYXZ, etc.)

### Materials & Post-Process
- `create_material` - Create Material asset with domain (Surface, PostProcess, DeferredDecal, LightFunction, UI) and blend mode
- `set_material_property` - Set material-level properties (ShadingModel, TwoSided, BlendMode, OpacityMaskClipValue, etc.)
- `add_material_expression` - Add expression node (40+ types: SceneTexture, Noise, Time, Add, Multiply, Lerp, Constant, ScalarParameter, VectorParameter, Custom HLSL, etc.)
- `connect_material_expressions` - Wire expression inputs (A, B, Alpha, Input, Position, etc.)
- `connect_to_material_output` - Connect to material outputs (BaseColor, EmissiveColor, Metallic, Roughness, Normal, Opacity, etc.)
- `set_material_expression_property` - Set properties on expression nodes
- `compile_material` - Force material recompilation
- `create_material_instance` - Create Material Instance with scalar/vector parameter overrides
- `create_post_process_volume` - Spawn Post Process Volume actor with materials assigned

## UE5.7 API Quirks

### Function Name Suffixes
- Math functions use `_DoubleDouble` suffix (not `_FloatFloat`):
  - `Add_DoubleDouble`, `Subtract_DoubleDouble`, `Multiply_DoubleDouble`, `GreaterEqual_DoubleDouble`
- Movement functions use `K2_` prefix:
  - `K2_AddActorWorldOffset`, `K2_SetActorLocation`, `K2_GetActorLocation`

### Static Class Targets for Functions
When calling static functions, use the class name as target:
- `KismetMathLibrary` - Math operations
- `KismetSystemLibrary` - System utilities, printing
- `GameplayStatics` - Actor spawning, player access
- `EnhancedInputLocalPlayerSubsystem` - Enhanced Input functions

### Node Types
- `K2Node_Event` - Standard and custom events
- `K2Node_CallFunction` - Function calls
- `K2Node_VariableGet` / `K2Node_VariableSet` - Variable access
- `K2Node_IfThenElse` - Branch node
- `K2Node_DynamicCast` - Type casting
- `K2Node_GetSubsystemFromPC` - Get subsystem with typed return value
- `K2Node_EnhancedInputAction` - Enhanced Input events
- `K2Node_Self` - Self reference
- `K2Node_CustomEvent` - Custom events

### Pin Names by Node Type

**K2Node_Event (BeginPlay/Tick):**
- `then` (exec output)
- `Delta Seconds` (float output, Tick only)

**K2Node_CallFunction:**
- `execute` (exec input)
- `then` (exec output)
- `self` (target object)
- `ReturnValue` (if function returns value)
- Parameter names match function signature

**K2Node_IfThenElse (Branch):**
- `Execute` (exec input)
- `Condition` (bool input)
- `Then` (exec output, true branch)
- `Else` (exec output, false branch)

**K2Node_VariableSet:**
- `execute` (exec input)
- `then` (exec output)
- Variable name (input)
- `Output_Get` (output, returns set value)

**K2Node_VariableGet:**
- Variable name (output)

**K2Node_EnhancedInputAction:**
- `Started` (exec output)
- `Triggered` (exec output)
- `Ongoing` (exec output)
- `Canceled` (exec output)
- `Completed` (exec output)
- `ActionValue` (FInputActionValue output)

**K2Node_GetSubsystemFromPC:**
- `PlayerController` (object input)
- `ReturnValue` (typed subsystem output)

**K2Node_DynamicCast:**
- `execute` (exec input)
- `then` (exec output, success)
- `CastFailed` (exec output, failure)
- `Object` (input to cast)
- `As [ClassName]` (output, casted reference)

**K2Node_SpawnActorFromClass:**
- `execute` (exec input)
- `then` (exec output)
- `Class` (class input - set via TrySetDefaultObject)
- `Spawn Transform` (Transform input)
- `Collision Handling Override` (enum input)
- `Owner` (Actor input, optional)
- `ReturnValue` (spawned actor output)

**K2Node_CallFunction (Custom Blueprint Function):**
- `execute` (exec input, if not pure)
- `then` (exec output, if not pure)
- `self` (target object - the BP instance containing the function)
- Input parameter pins match function signature
- Output/return pins match function signature

## Common Patterns

### Enhanced Input Setup in PlayerController
```
1. BeginPlay event
2. Self reference → K2Node_GetSubsystemFromPC (EnhancedInputLocalPlayerSubsystem)
3. GetSubsystem.ReturnValue → AddMappingContext.self
4. Set MappingContext pin to /Game/Input/IMC_YourContext.IMC_YourContext
5. BeginPlay.then → AddMappingContext.execute
```

### Fixed Timestep Pattern
```
GameState:
1. Tick → accumulate time
2. When accumulated >= FixedDelta → broadcast EventDispatcher

Subscribers:
1. BeginPlay → Get GameState → Bind to EventDispatcher
2. Custom Event handler → process fixed-timestep logic
```

### Casting Pattern
```
1. Get reference (e.g., GetGameState returns GameStateBase)
2. Cast node to specific Blueprint class
3. Connect success exec to downstream logic
4. Use "As [ClassName]" output for typed access
```

### Spawning Actors at Runtime
```
1. add_spawn_actor_from_class_node (class_to_spawn: "BP_Enemy")
2. Set Spawn Transform via MakeTransform or variable
3. Connect exec pins to control when spawn happens
4. Use ReturnValue for reference to spawned actor
```

### Calling Custom Blueprint Functions
```
1. Create function in target BP: create_blueprint_function
2. Compile target BP: compile_blueprint (REQUIRED before calling)
3. In caller BP: call_blueprint_function(target_blueprint, function_name)
4. Wire self pin to instance of target BP class
5. Connect exec and data pins as needed
```

## Object Pin Default Values

For object reference pins (InputMappingContext, Materials, Meshes, etc.):
- Use asset path format: `/Game/Path/AssetName.AssetName`
- The `set_node_pin_default` command will call `StaticLoadObject` automatically
- Example: `/Game/Input/IMC_Default.IMC_Default`

## Debugging Tips

1. **Use `get_node_pins`** to inspect available pins when connection fails
2. **Check `compile_blueprint` errors** - includes node_id for problematic nodes
3. **Use `find_blueprint_nodes`** to get current state before modifications
4. **Interface pins** (like EnhancedInputSubsystemInterface) need properly-typed inputs - use K2Node_GetSubsystemFromPC instead of generic function calls

## Best Practices

1. **Always compile after changes** - catches errors early, auto-saves
2. **Delete old nodes before recreating** - avoids duplicates
3. **Use typed subsystem nodes** - K2Node_GetSubsystemFromPC returns correct type for interface compatibility
4. **Check pin categories** - exec, object, interface, struct, etc. have different connection rules
5. **Wire exec pins first** - establishes flow, then wire data pins

## UEBlueprintMCP Architecture

The MCP plugin uses a **persistent socket connection** and **modular action-based architecture** for reliable Blueprint manipulation.

### Connection Model
```
Python MCP Server (persistent connection)
    ↓ JSON over TCP (port 55558, stays open)
MCPBridge.cpp (central command router)
    ↓ dispatches to registered FEditorAction handlers
Action Classes (BlueprintActions, NodeActions, etc.)
    ↓ unified Execute() pipeline
Validation → Execution → Post-Validation → Auto-Save
```

**Key Features:**
- **Persistent socket** - Connection stays open between commands (no reconnect overhead)
- **Central handler** - All commands flow through `MCPBridge::ExecuteCommand()`
- **Auto-save** - Dirty packages saved after each successful action
- **Crash protection** - Actions validate inputs before execution

### Action Class Hierarchy
```
FEditorAction (base)
├── FBlueprintAction (Blueprint operations)
│   └── FBlueprintNodeAction (graph node operations)
├── FViewportAction (camera, viewport)
├── FLevelAction (actors in level)
├── FProjectAction (input settings)
└── FUMGAction (widget blueprints)
```

### Existing Action Files
| File | Purpose |
|------|---------|
| `BlueprintActions.cpp` | create_blueprint, compile_blueprint, components, materials |
| `EditorActions.cpp` | Actor manipulation, viewport, save_all |
| `NodeActions.cpp` | 40+ node types: events, variables, functions, dispatchers, spawn |
| `ProjectActions.cpp` | Enhanced Input: actions, mapping contexts, key bindings |
| `UMGActions.cpp` | Widget blueprints, text blocks, buttons, bindings |

## Extending the MCP (Adding New Commands)

When a Blueprint or Editor manipulation feature is missing, extend the plugin rather than asking the user to do it manually.

### Adding a New Action

**1. Declare the action class (Public/Actions/[Category]Actions.h):**
```cpp
class UEBLUEPRINTMCP_API FMyNewAction : public FBlueprintNodeAction
{
public:
    virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
    virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
    virtual FString GetActionName() const override { return TEXT("my_new_command"); }
};
```

**2. Implement the action (Private/Actions/[Category]Actions.cpp):**
```cpp
bool FMyNewAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
    FString RequiredParam;
    if (!GetRequiredString(Params, TEXT("required_param"), RequiredParam, OutError)) return false;
    return ValidateBlueprint(Params, Context, OutError);  // or ValidateGraph for node actions
}

TSharedPtr<FJsonObject> FMyNewAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
    UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
    UEdGraph* Graph = GetTargetGraph(Params, Context);
    FVector2D Position = GetNodePosition(Params);

    // ... your logic here ...

    MarkBlueprintModified(Blueprint, Context);
    RegisterCreatedNode(Node, Context);  // if creating a node

    TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
    ResultData->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
    return CreateSuccessResponse(ResultData);
}
```

**3. Register in MCPBridge.cpp:**
```cpp
// In RegisterActions():
ActionHandlers.Add(TEXT("my_new_command"), MakeShared<FMyNewAction>());
```

**4. Add Python tool (Python/ue_blueprint_mcp/tools/*.py):**
```python
@mcp.tool()
async def my_new_command(
    ctx: Context,
    blueprint_name: str,
    required_param: str,
    node_position: Optional[List[float]] = None
) -> str:
    """Docstring for MCP clients."""
    return await send_command_async("my_new_command", {
        "blueprint_name": blueprint_name,
        "required_param": required_param,
        "node_position": node_position
    })
```

### Creating Blueprint Nodes (Use Editor's API)

**Use `FEdGraphSchemaAction_K2NewNode::SpawnNode<T>()` instead of manual boilerplate:**
```cpp
#include "EdGraphSchema_K2_Actions.h"

// Simple node (no initialization needed):
UK2Node_IfThenElse* BranchNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_IfThenElse>(
    TargetGraph, Position, EK2NewNodeFlags::None
);

// Node with initialization (lambda runs BEFORE pin allocation):
UK2Node_DynamicCast* CastNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_DynamicCast>(
    EventGraph, Position, EK2NewNodeFlags::None,
    [TargetClass, bPureCast](UK2Node_DynamicCast* Node) {
        Node->TargetType = TargetClass;
        Node->SetPurity(bPureCast);
    }
);

// This single call handles: NewObject, position, AddNode, CreateNewGuid, PostPlacedNewNode, AllocateDefaultPins
```

### String-Based Class Resolution

The MCP accepts user-friendly string inputs and resolves them automatically:

**Blueprint names:** `"BP_Enemy"` → searches asset registry
**Content paths:** `"/Game/Blueprints/BP_Enemy"` → loads directly
**Engine classes:** `"Actor"` → tries `/Script/Engine.Actor`
**Subsystems:** `"EnhancedInputLocalPlayerSubsystem"` → searches common modules

Example from `FAddBlueprintCastNodeAction`:
```cpp
// Try blueprint name first
UBlueprint* TargetBP = FMCPCommonUtils::FindBlueprint(TargetClassName);
if (TargetBP && TargetBP->GeneratedClass) {
    TargetClass = TargetBP->GeneratedClass;
}

// Try engine classes
if (!TargetClass) {
    static const FString ModulePaths[] = { TEXT("/Script/Engine"), TEXT("/Script/CoreUObject") };
    for (const FString& ModulePath : ModulePaths) {
        FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *TargetClassName);
        TargetClass = LoadClass<UObject>(nullptr, *FullPath);
        if (TargetClass) break;
    }
}
```

### Helper Methods (FEditorAction base class)

```cpp
// Parameter extraction
GetRequiredString(Params, TEXT("key"), OutValue, OutError)  // returns false if missing
GetOptionalString(Params, TEXT("key"))                       // returns empty if missing
GetOptionalBool(Params, TEXT("key"), DefaultValue)
GetOptionalArray(Params, TEXT("key"))
GetNodePosition(Params)                                      // returns FVector2D from "node_position"

// Blueprint/Graph access
GetTargetBlueprint(Params, Context)    // uses "blueprint_name" or context
GetTargetGraph(Params, Context)        // uses "graph_name" or event graph

// State management
MarkBlueprintModified(Blueprint, Context)   // marks dirty, adds to save queue
RegisterCreatedNode(Node, Context)          // stores in Context.LastCreatedNodeId

// Response helpers
CreateSuccessResponse(ResultData)
CreateErrorResponse(TEXT("Error message"))
```

## UMG Widget Event Binding

The MCP `bind_widget_event` command creates a `UK2Node_ComponentBoundEvent` node in the widget's event graph. The widget component must be marked as a variable ("Is Variable" = true in designer) for the binding to resolve correctly.

## Material Expression Types

### Available Expression Classes
- **Scene/Texture:** SceneTexture, SceneDepth, ScreenPosition, TextureCoordinate, TextureSample, PixelDepth, WorldPosition, CameraPosition, VertexNormalWS, TwoSidedSign
- **Math:** Add, Subtract, Multiply, Divide, Power, SquareRoot, Abs, Min, Max, Clamp, Saturate, Floor, Ceil, Frac, OneMinus, Step, SmoothStep
- **Trig:** Sin, Cos
- **Vector:** DotProduct, CrossProduct, Normalize, AppendVector, ComponentMask
- **Constants:** Constant, Constant2Vector, Constant3Vector, Constant4Vector
- **Parameters:** ScalarParameter, VectorParameter
- **Procedural:** Noise, Time, Panner
- **Derivatives:** DDX, DDY
- **Control:** If, Lerp/LinearInterpolate
- **Custom:** Custom (HLSL code)

### Expression Input Pin Names
| Expression Type | Input Pins |
|-----------------|------------|
| Add, Subtract, Multiply, Divide, DotProduct, Min, Max | A, B |
| Power | Base, Exponent |
| Lerp | A, B, Alpha |
| Clamp | Input, Min, Max |
| If | A, B, AGreaterThanB, AEqualsB, ALessThanB |
| ComponentMask, SquareRoot, Abs | Input |
| Noise | Position, FilterWidth |
| Panner | Coordinate, Time, Speed |
| AppendVector | A, B |
| Custom | Dynamic (matches input names in Inputs array) |

### Material Output Properties
BaseColor, EmissiveColor, Metallic, Roughness, Specular, Normal, Opacity, OpacityMask, AmbientOcclusion, WorldPositionOffset, Refraction

### Material Properties (set_material_property)
- **ShadingModel:** Unlit, DefaultLit, Subsurface, PreintegratedSkin, ClearCoat, SubsurfaceProfile, TwoSidedFoliage, Hair, Cloth, Eye
- **TwoSided:** true/false
- **BlendMode:** Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout
- **OpacityMaskClipValue:** float (0.0-1.0)
- **DitheredLODTransition:** true/false
- **AllowNegativeEmissiveColor:** true/false

### Material Context Tracking
The MCP tracks `CurrentMaterial` and `MaterialNodeMap` in `FMCPEditorContext`. After creating expressions with `add_material_expression`, they can be referenced by name in subsequent `connect_material_expressions` calls. Use `$last` or `$last_node` to reference the most recently created expression.
