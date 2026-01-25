"""
Blueprint graph node tools - Events, functions, variables, and graph operations.
"""

import json
from typing import Any
from mcp.types import Tool, TextContent

from ..connection import get_connection


def _send_command(command_type: str, params: dict | None = None) -> list[TextContent]:
    """Helper to send command and format response."""
    conn = get_connection()
    if not conn.is_connected:
        conn.connect()
    result = conn.send_command(command_type, params)
    return [TextContent(type="text", text=json.dumps(result.to_dict(), indent=2))]


def get_tools() -> list[Tool]:
    """Get all blueprint node tools."""
    return [
        # =====================================================================
        # Event Nodes
        # =====================================================================
        Tool(
            name="add_blueprint_event_node",
            description="Add an event node (ReceiveBeginPlay, ReceiveTick, etc.) to a Blueprint's event graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "event_name": {"type": "string", "description": "Event name (ReceiveBeginPlay, ReceiveTick, etc.)"},
                    "node_position": {"type": "string", "description": "[X, Y] position in graph (as JSON string)"}
                },
                "required": ["blueprint_name", "event_name"]
            }
        ),
        Tool(
            name="add_blueprint_custom_event",
            description="Add a Custom Event node with optional parameters.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "event_name": {"type": "string", "description": "Name for the custom event"},
                    "node_position": {"type": "string", "description": "[X, Y] position"},
                    "parameters": {
                        "type": "array",
                        "items": {"type": "object"},
                        "description": "Parameters with 'name' and 'type' keys (Float, Boolean, Integer, Vector, String)"
                    }
                },
                "required": ["blueprint_name", "event_name"]
            }
        ),
        Tool(
            name="add_blueprint_input_action_node",
            description="Add an input action event node (legacy input system).",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "action_name": {"type": "string", "description": "Name of the input action"},
                    "node_position": {"type": "string", "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "action_name"]
            }
        ),
        Tool(
            name="add_enhanced_input_action_node",
            description="Add an Enhanced Input Action event node with Started/Triggered/Completed exec pins.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "action_name": {"type": "string", "description": "Name of the Input Action asset (e.g., IA_Move)"},
                    "action_path": {"type": "string", "description": "Content path to the asset (default: /Game/Input)"},
                    "node_position": {"type": "string", "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "action_name"]
            }
        ),

        # =====================================================================
        # Event Dispatchers
        # =====================================================================
        Tool(
            name="add_event_dispatcher",
            description="Add an Event Dispatcher (multicast delegate) to a Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "dispatcher_name": {"type": "string", "description": "Name for the dispatcher"},
                    "parameters": {
                        "type": "array",
                        "items": {"type": "object"},
                        "description": "Parameters with 'name' and 'type' keys"
                    }
                },
                "required": ["blueprint_name", "dispatcher_name"]
            }
        ),
        Tool(
            name="call_event_dispatcher",
            description="Add a Call node for an Event Dispatcher (broadcasts the event).",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "dispatcher_name": {"type": "string", "description": "Name of the dispatcher to call"},
                    "node_position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "dispatcher_name"]
            }
        ),
        Tool(
            name="bind_event_dispatcher",
            description="Add a Bind node for an Event Dispatcher (creates bind node + matching custom event).",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Blueprint where to add the bind node"},
                    "dispatcher_name": {"type": "string", "description": "Name of the dispatcher to bind to"},
                    "target_blueprint": {"type": "string", "description": "Blueprint that owns the dispatcher"},
                    "node_position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "dispatcher_name"]
            }
        ),

        # =====================================================================
        # Function Nodes
        # =====================================================================
        Tool(
            name="add_blueprint_function_node",
            description="Add a function call node to a Blueprint graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "target": {"type": "string", "description": "Target object (component name or self)"},
                    "function_name": {"type": "string", "description": "Name of the function to call"},
                    "params": {"type": "string", "description": "Parameters as JSON string"},
                    "node_position": {"type": "string", "description": "[X, Y] position"},
                    "graph_name": {"type": "string", "description": "Optional function graph name (defaults to event graph)"}
                },
                "required": ["blueprint_name", "target", "function_name"]
            }
        ),
        Tool(
            name="create_blueprint_function",
            description="Create a new function graph in a Blueprint with inputs/outputs.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "function_name": {"type": "string", "description": "Name of the function"},
                    "inputs": {
                        "type": "array",
                        "items": {"type": "object"},
                        "description": "Input parameters with 'name' and 'type' keys"
                    },
                    "outputs": {
                        "type": "array",
                        "items": {"type": "object"},
                        "description": "Output parameters with 'name' and 'type' keys"
                    },
                    "is_pure": {"type": "boolean", "description": "Create as pure function (no exec pins)"}
                },
                "required": ["blueprint_name", "function_name"]
            }
        ),
        Tool(
            name="call_blueprint_function",
            description="Add a node that calls a custom Blueprint function.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Blueprint where to add the node"},
                    "target_blueprint": {"type": "string", "description": "Blueprint containing the function"},
                    "function_name": {"type": "string", "description": "Name of the function to call"},
                    "node_position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "target_blueprint", "function_name"]
            }
        ),

        # =====================================================================
        # Variable Nodes
        # =====================================================================
        Tool(
            name="add_blueprint_variable",
            description="Add a variable to a Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "variable_name": {"type": "string", "description": "Name of the variable"},
                    "variable_type": {"type": "string", "description": "Type (Boolean, Integer, Float, Vector, etc.)"},
                    "is_exposed": {"type": "boolean", "description": "Expose to editor"}
                },
                "required": ["blueprint_name", "variable_name", "variable_type"]
            }
        ),
        Tool(
            name="add_blueprint_variable_get",
            description="Add a Variable Get node to a Blueprint graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "variable_name": {"type": "string", "description": "Name of the variable"},
                    "node_position": {"type": "string", "description": "[X, Y] position"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "variable_name"]
            }
        ),
        Tool(
            name="add_blueprint_variable_set",
            description="Add a Variable Set node to a Blueprint graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "variable_name": {"type": "string", "description": "Name of the variable"},
                    "node_position": {"type": "string", "description": "[X, Y] position"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "variable_name"]
            }
        ),
        Tool(
            name="set_node_pin_default",
            description="Set the default value of a pin on an existing node.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_id": {"type": "string", "description": "GUID of the node"},
                    "pin_name": {"type": "string", "description": "Name of the pin"},
                    "default_value": {"type": "string", "description": "Default value as string"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "node_id", "pin_name", "default_value"]
            }
        ),

        # =====================================================================
        # Reference Nodes
        # =====================================================================
        Tool(
            name="add_blueprint_get_self_component_reference",
            description="Add a node that gets a reference to a component owned by the Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "component_name": {"type": "string", "description": "Name of the component"},
                    "node_position": {"type": "string", "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "component_name"]
            }
        ),
        Tool(
            name="add_blueprint_self_reference",
            description="Add a 'Get Self' node that returns a reference to this actor.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_position": {"type": "string", "description": "[X, Y] position"}
                },
                "required": ["blueprint_name"]
            }
        ),
        Tool(
            name="add_blueprint_cast_node",
            description="Add a Cast node for type casting (e.g., GameStateBase to BP_MyGameState).",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "target_class": {"type": "string", "description": "Class to cast to"},
                    "pure_cast": {"type": "boolean", "description": "Create as pure cast (no exec pins)"},
                    "node_position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"}
                },
                "required": ["blueprint_name", "target_class"]
            }
        ),

        # =====================================================================
        # Flow Control
        # =====================================================================
        Tool(
            name="add_blueprint_branch_node",
            description="Add a Branch (If/Then/Else) node.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_position": {"type": "string", "description": "[X, Y] position"}
                },
                "required": ["blueprint_name"]
            }
        ),

        # =====================================================================
        # Spawning
        # =====================================================================
        Tool(
            name="add_spawn_actor_from_class_node",
            description="Add a SpawnActorFromClass node for runtime actor spawning.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "class_to_spawn": {"type": "string", "description": "Class to spawn (e.g., BP_Enemy)"},
                    "node_position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "class_to_spawn"]
            }
        ),

        # =====================================================================
        # Graph Operations
        # =====================================================================
        Tool(
            name="connect_blueprint_nodes",
            description="Connect two nodes in a Blueprint graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "source_node_id": {"type": "string", "description": "GUID of the source node"},
                    "source_pin": {"type": "string", "description": "Name of the output pin"},
                    "target_node_id": {"type": "string", "description": "GUID of the target node"},
                    "target_pin": {"type": "string", "description": "Name of the input pin"},
                    "graph_name": {"type": "string", "description": "Optional function graph name"}
                },
                "required": ["blueprint_name", "source_node_id", "source_pin", "target_node_id", "target_pin"]
            }
        ),
        Tool(
            name="find_blueprint_nodes",
            description="Find nodes in a Blueprint's event graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_type": {"type": "string", "description": "Type of node (Event, Function, Variable, etc.)"},
                    "event_type": {"type": "string", "description": "Specific event type (BeginPlay, Tick, etc.)"}
                },
                "required": ["blueprint_name"]
            }
        ),
        Tool(
            name="delete_blueprint_node",
            description="Delete a node from a Blueprint's graph.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_id": {"type": "string", "description": "GUID of the node to delete"}
                },
                "required": ["blueprint_name", "node_id"]
            }
        ),
        Tool(
            name="get_node_pins",
            description="Get all pins on a node for debugging connection issues.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint"},
                    "node_id": {"type": "string", "description": "GUID of the node"}
                },
                "required": ["blueprint_name", "node_id"]
            }
        ),
    ]


TOOL_HANDLERS = {
    # Events
    "add_blueprint_event_node": "add_blueprint_event_node",
    "add_blueprint_custom_event": "add_blueprint_custom_event",
    "add_blueprint_input_action_node": "add_blueprint_input_action_node",
    "add_enhanced_input_action_node": "add_enhanced_input_action_node",
    # Event Dispatchers
    "add_event_dispatcher": "add_event_dispatcher",
    "call_event_dispatcher": "call_event_dispatcher",
    "bind_event_dispatcher": "bind_event_dispatcher",
    # Functions
    "add_blueprint_function_node": "add_blueprint_function_node",
    "create_blueprint_function": "create_blueprint_function",
    "call_blueprint_function": "call_blueprint_function",
    # Variables
    "add_blueprint_variable": "add_blueprint_variable",
    "add_blueprint_variable_get": "add_blueprint_variable_get",
    "add_blueprint_variable_set": "add_blueprint_variable_set",
    "set_node_pin_default": "set_node_pin_default",
    # References
    "add_blueprint_get_self_component_reference": "add_blueprint_get_self_component_reference",
    "add_blueprint_self_reference": "add_blueprint_self_reference",
    "add_blueprint_cast_node": "add_blueprint_cast_node",
    # Flow Control
    "add_blueprint_branch_node": "add_blueprint_branch_node",
    # Spawning
    "add_spawn_actor_from_class_node": "add_spawn_actor_from_class_node",
    # Graph Operations
    "connect_blueprint_nodes": "connect_blueprint_nodes",
    "find_blueprint_nodes": "find_blueprint_nodes",
    "delete_blueprint_node": "delete_blueprint_node",
    "get_node_pins": "get_node_pins",
}


async def handle_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Handle a node tool call."""
    command_type = TOOL_HANDLERS.get(name)
    if not command_type:
        return [TextContent(type="text", text=f'{{"success": false, "error": "Unknown tool: {name}"}}')]

    return _send_command(command_type, arguments if arguments else None)
