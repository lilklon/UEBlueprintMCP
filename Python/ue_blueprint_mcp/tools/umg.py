"""
UMG tools - Widget Blueprint creation and manipulation.
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
    """Get all UMG tools."""
    return [
        Tool(
            name="create_umg_widget_blueprint",
            description="Create a new UMG Widget Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the widget blueprint"},
                    "parent_class": {"type": "string", "description": "Parent class (default: UserWidget)"},
                    "path": {"type": "string", "description": "Content browser path (default: /Game/UI)"}
                },
                "required": ["widget_name"]
            }
        ),
        Tool(
            name="add_text_block_to_widget",
            description="Add a Text Block widget to a UMG Widget Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the Widget Blueprint"},
                    "text_block_name": {"type": "string", "description": "Name for the Text Block"},
                    "text": {"type": "string", "description": "Initial text content"},
                    "position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"},
                    "size": {"type": "array", "items": {"type": "number"}, "description": "[Width, Height]"},
                    "font_size": {"type": "integer", "description": "Font size in points"},
                    "color": {"type": "array", "items": {"type": "number"}, "description": "[R, G, B, A] values 0.0-1.0"}
                },
                "required": ["widget_name", "text_block_name"]
            }
        ),
        Tool(
            name="add_button_to_widget",
            description="Add a Button widget to a UMG Widget Blueprint.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the Widget Blueprint"},
                    "button_name": {"type": "string", "description": "Name for the Button"},
                    "text": {"type": "string", "description": "Button text"},
                    "position": {"type": "array", "items": {"type": "number"}, "description": "[X, Y] position"},
                    "size": {"type": "array", "items": {"type": "number"}, "description": "[Width, Height]"},
                    "font_size": {"type": "integer", "description": "Font size"},
                    "color": {"type": "array", "items": {"type": "number"}, "description": "[R, G, B, A] text color"},
                    "background_color": {"type": "array", "items": {"type": "number"}, "description": "[R, G, B, A] background"}
                },
                "required": ["widget_name", "button_name"]
            }
        ),
        Tool(
            name="bind_widget_event",
            description="Bind an event on a widget component (e.g., button OnClicked). Creates a Component Bound Event node in the graph that fires when the event occurs.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the Widget Blueprint"},
                    "widget_component_name": {"type": "string", "description": "Name of the widget component (e.g., RestartButton)"},
                    "event_name": {"type": "string", "description": "Event to bind (OnClicked, OnPressed, OnReleased, etc.)"}
                },
                "required": ["widget_name", "widget_component_name", "event_name"]
            }
        ),
        Tool(
            name="add_widget_to_viewport",
            description="Add a Widget Blueprint instance to the viewport.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the Widget Blueprint"},
                    "z_order": {"type": "integer", "description": "Z-order (higher = on top)"}
                },
                "required": ["widget_name"]
            }
        ),
        Tool(
            name="set_text_block_binding",
            description="Set up a property binding for a Text Block widget.",
            inputSchema={
                "type": "object",
                "properties": {
                    "widget_name": {"type": "string", "description": "Name of the Widget Blueprint"},
                    "text_block_name": {"type": "string", "description": "Name of the Text Block"},
                    "binding_property": {"type": "string", "description": "Property to bind to"},
                    "binding_type": {"type": "string", "description": "Type of binding (Text, Visibility, etc.)"}
                },
                "required": ["widget_name", "text_block_name", "binding_property"]
            }
        ),
    ]


TOOL_HANDLERS = {
    "create_umg_widget_blueprint": "create_umg_widget_blueprint",
    "add_text_block_to_widget": "add_text_block_to_widget",
    "add_button_to_widget": "add_button_to_widget",
    "bind_widget_event": "bind_widget_event",
    "add_widget_to_viewport": "add_widget_to_viewport",
    "set_text_block_binding": "set_text_block_binding",
}


async def handle_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Handle a UMG tool call."""
    command_type = TOOL_HANDLERS.get(name)
    if not command_type:
        return [TextContent(type="text", text=f'{{"success": false, "error": "Unknown tool: {name}"}}')]

    return _send_command(command_type, arguments if arguments else None)
