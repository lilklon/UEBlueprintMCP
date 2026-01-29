"""
Editor tools - Level actors, viewport, and general editor operations.
"""

import asyncio
import json
from typing import Any
from mcp.types import Tool, TextContent

from ..connection import get_connection, CommandResult


def _send_command_sync(command_type: str, params: dict | None = None) -> list[TextContent]:
    """Helper to send command and format response (synchronous)."""
    conn = get_connection()
    if not conn.is_connected:
        conn.connect()
    result = conn.send_command(command_type, params)
    return [TextContent(type="text", text=json.dumps(result.to_dict(), indent=2))]


async def _send_command(command_type: str, params: dict | None = None) -> list[TextContent]:
    """Helper to send command and format response (async-safe)."""
    return await asyncio.to_thread(_send_command_sync, command_type, params)


def get_tools() -> list[Tool]:
    """Get all editor tools."""
    return [
        # Level actors
        Tool(
            name="get_actors_in_level",
            description="Get a list of all actors in the current level.",
            inputSchema={"type": "object", "properties": {}}
        ),
        Tool(
            name="find_actors_by_name",
            description="Find actors by name pattern.",
            inputSchema={
                "type": "object",
                "properties": {
                    "pattern": {"type": "string", "description": "Name pattern to search for"}
                },
                "required": ["pattern"]
            }
        ),
        Tool(
            name="spawn_actor",
            description="Create a new actor in the current level.",
            inputSchema={
                "type": "object",
                "properties": {
                    "name": {"type": "string", "description": "Name to give the new actor (must be unique)"},
                    "type": {"type": "string", "description": "Type of actor (e.g. StaticMeshActor, PointLight)"},
                    "location": {
                        "type": "array",
                        "items": {"type": "number"},
                        "description": "[x, y, z] world location"
                    },
                    "rotation": {
                        "type": "array",
                        "items": {"type": "number"},
                        "description": "[pitch, yaw, roll] in degrees"
                    }
                },
                "required": ["name", "type"]
            }
        ),
        Tool(
            name="spawn_blueprint_actor",
            description="Spawn an actor from a Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "blueprint_name": {"type": "string", "description": "Name of the Blueprint to spawn from"},
                    "actor_name": {"type": "string", "description": "Name to give the spawned actor"},
                    "location": {
                        "type": "array",
                        "items": {"type": "number"},
                        "description": "[x, y, z] world location"
                    },
                    "rotation": {
                        "type": "array",
                        "items": {"type": "number"},
                        "description": "[pitch, yaw, roll] in degrees"
                    }
                },
                "required": ["blueprint_name", "actor_name"]
            }
        ),
        Tool(
            name="delete_actor",
            description="Delete an actor by name.",
            inputSchema={
                "type": "object",
                "properties": {
                    "name": {"type": "string", "description": "Name of the actor to delete"}
                },
                "required": ["name"]
            }
        ),
        Tool(
            name="set_actor_transform",
            description="Set the transform of an actor.",
            inputSchema={
                "type": "object",
                "properties": {
                    "name": {"type": "string", "description": "Name of the actor"},
                    "location": {"type": "array", "items": {"type": "number"}, "description": "[x, y, z]"},
                    "rotation": {"type": "array", "items": {"type": "number"}, "description": "[pitch, yaw, roll]"},
                    "scale": {"type": "array", "items": {"type": "number"}, "description": "[x, y, z]"}
                },
                "required": ["name"]
            }
        ),
        Tool(
            name="get_actor_properties",
            description="Get all properties of an actor.",
            inputSchema={
                "type": "object",
                "properties": {
                    "name": {"type": "string", "description": "Name of the actor"}
                },
                "required": ["name"]
            }
        ),
        Tool(
            name="set_actor_property",
            description="Set a property on an actor.",
            inputSchema={
                "type": "object",
                "properties": {
                    "name": {"type": "string", "description": "Name of the actor"},
                    "property_name": {"type": "string", "description": "Name of the property"},
                    "property_value": {"type": "string", "description": "Value to set"}
                },
                "required": ["name", "property_name", "property_value"]
            }
        ),

        # Viewport
        Tool(
            name="focus_viewport",
            description="Focus the viewport on a specific actor or location.",
            inputSchema={
                "type": "object",
                "properties": {
                    "target": {"type": "string", "description": "Name of actor to focus on"},
                    "location": {"type": "array", "items": {"type": "number"}, "description": "[x, y, z]"},
                    "distance": {"type": "number", "description": "Distance from target"},
                    "orientation": {"type": "array", "items": {"type": "number"}, "description": "[pitch, yaw, roll]"}
                }
            }
        ),
        Tool(
            name="get_viewport_transform",
            description="Get the current viewport camera location and rotation.",
            inputSchema={"type": "object", "properties": {}}
        ),
        Tool(
            name="set_viewport_transform",
            description="Set the viewport camera location and/or rotation directly.",
            inputSchema={
                "type": "object",
                "properties": {
                    "location": {"type": "array", "items": {"type": "number"}, "description": "[x, y, z]"},
                    "rotation": {"type": "array", "items": {"type": "number"}, "description": "[pitch, yaw, roll]"}
                }
            }
        ),

        # Utility
        Tool(
            name="save_all",
            description="Save all dirty packages (blueprints, levels, assets).",
            inputSchema={"type": "object", "properties": {}}
        ),
    ]


# Map tool names to command types
TOOL_HANDLERS = {
    "get_actors_in_level": "get_actors_in_level",
    "find_actors_by_name": "find_actors_by_name",
    "spawn_actor": "spawn_actor",
    "spawn_blueprint_actor": "spawn_blueprint_actor",
    "delete_actor": "delete_actor",
    "set_actor_transform": "set_actor_transform",
    "get_actor_properties": "get_actor_properties",
    "set_actor_property": "set_actor_property",
    "focus_viewport": "focus_viewport",
    "get_viewport_transform": "get_viewport_transform",
    "set_viewport_transform": "set_viewport_transform",
    "save_all": "save_all",
}


async def handle_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Handle an editor tool call."""
    command_type = TOOL_HANDLERS.get(name)
    if not command_type:
        return [TextContent(type="text", text=f'{{"success": false, "error": "Unknown tool: {name}"}}')]

    return await _send_command(command_type, arguments if arguments else None)
