"""
UE Blueprint MCP Server - Main entry point.

Registers all tools with the MCP SDK and handles the server lifecycle.
"""

import asyncio
import logging
from typing import Any

from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import Tool, TextContent

from .connection import get_connection, PersistentUnrealConnection, CommandResult

# Import tool modules
from .tools import blueprint, editor, nodes, project, umg

logger = logging.getLogger(__name__)

# Create MCP server instance
server = Server("ue-blueprint-mcp")


def _result_to_text(result: CommandResult) -> list[TextContent]:
    """Convert CommandResult to MCP text content."""
    import json
    return [TextContent(type="text", text=json.dumps(result.to_dict(), indent=2))]


def _send_command(command_type: str, params: dict | None = None) -> list[TextContent]:
    """Helper to send command and format response."""
    conn = get_connection()
    if not conn.is_connected:
        conn.connect()
    result = conn.send_command(command_type, params)
    return _result_to_text(result)


# =============================================================================
# Connection Tools
# =============================================================================

@server.list_tools()
async def list_tools() -> list[Tool]:
    """List all available tools."""
    tools = []

    # Connection tools
    tools.append(Tool(
        name="ping",
        description="Test connection to Unreal Engine",
        inputSchema={"type": "object", "properties": {}}
    ))

    tools.append(Tool(
        name="get_context",
        description="Get current editor context (active blueprint, graph, etc.)",
        inputSchema={"type": "object", "properties": {}}
    ))

    # Add tools from all modules
    tools.extend(editor.get_tools())
    tools.extend(blueprint.get_tools())
    tools.extend(nodes.get_tools())
    tools.extend(project.get_tools())
    tools.extend(umg.get_tools())

    return tools


@server.call_tool()
async def call_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Route tool calls to appropriate handlers."""

    # Connection tools
    if name == "ping":
        conn = get_connection()
        if not conn.is_connected:
            conn.connect()
        success = conn.ping()
        return [TextContent(type="text", text=f'{{"success": {str(success).lower()}, "pong": {str(success).lower()}}}')]

    if name == "get_context":
        return _send_command("get_context")

    # Route to tool modules
    if name in editor.TOOL_HANDLERS:
        return await editor.handle_tool(name, arguments)

    if name in blueprint.TOOL_HANDLERS:
        return await blueprint.handle_tool(name, arguments)

    if name in nodes.TOOL_HANDLERS:
        return await nodes.handle_tool(name, arguments)

    if name in project.TOOL_HANDLERS:
        return await project.handle_tool(name, arguments)

    if name in umg.TOOL_HANDLERS:
        return await umg.handle_tool(name, arguments)

    return [TextContent(type="text", text=f'{{"success": false, "error": "Unknown tool: {name}"}}')]


async def run_server():
    """Run the MCP server."""
    logger.info("Starting UE Blueprint MCP server...")

    # Establish connection to Unreal
    conn = get_connection()
    conn.connect()

    async with stdio_server() as (read_stream, write_stream):
        await server.run(read_stream, write_stream, server.create_initialization_options())


def main():
    """Main entry point."""
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )

    try:
        asyncio.run(run_server())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    finally:
        # Clean up connection
        conn = get_connection()
        conn.disconnect()


if __name__ == "__main__":
    main()
