#!/bin/bash

# Search Engine Web UI Launcher
# This script starts the REST API server and web UI server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
REST_PORT=8080
WEB_PORT=3000
# Server priority: rest_server_drogon (fastest) > rest_server_crow > rest_server (basic)
# The script will use the first available server from the build directory
DEFAULT_REST_SERVER="rest_server_drogon"  # Default: Drogon (high-performance async)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/../../build/server"
WEB_DIR="$SCRIPT_DIR"

# PID files for cleanup
REST_PID=""
WEB_PID=""

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}🛑 Shutting down servers...${NC}"
    
    if [ ! -z "$REST_PID" ] && kill -0 "$REST_PID" 2>/dev/null; then
        echo -e "${BLUE}   Stopping REST server (PID: $REST_PID)${NC}"
        kill "$REST_PID" 2>/dev/null || true
    fi
    
    if [ ! -z "$WEB_PID" ] && kill -0 "$WEB_PID" 2>/dev/null; then
        echo -e "${BLUE}   Stopping web server (PID: $WEB_PID)${NC}"
        kill "$WEB_PID" 2>/dev/null || true
    fi
    
    echo -e "${GREEN}✨ Cleanup complete. Goodbye!${NC}"
}

# Set up trap to cleanup on exit
trap cleanup EXIT INT TERM

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}❌ Error: Build directory not found: $BUILD_DIR${NC}"
    echo -e "${YELLOW}   Please build the project first:${NC}"
    echo -e "   cd $SCRIPT_DIR/../.."
    echo -e "   mkdir -p build && cd build"
    echo -e "   cmake .. && make"
    exit 1
fi

# Find available REST server (prioritizes Drogon for best performance)
REST_SERVER_PATH=""
REST_SERVER=""
for server in rest_server_drogon rest_server_crow rest_server; do
    if [ -x "$BUILD_DIR/$server" ]; then
        REST_SERVER="$server"
        REST_SERVER_PATH="$BUILD_DIR/$server"
        break
    fi
done

if [ -z "$REST_SERVER_PATH" ]; then
    echo -e "${RED}❌ Error: No REST server found in $BUILD_DIR${NC}"
    echo -e "${YELLOW}   Available servers: rest_server, rest_server_drogon, rest_server_crow${NC}"
    exit 1
fi

# Check if ports are already in use
if lsof -Pi :$REST_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo -e "${RED}❌ Error: Port $REST_PORT is already in use${NC}"
    echo -e "${YELLOW}   Kill the process using: kill \$(lsof -ti:$REST_PORT)${NC}"
    exit 1
fi

if lsof -Pi :$WEB_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo -e "${RED}❌ Error: Port $WEB_PORT is already in use${NC}"
    echo -e "${YELLOW}   Kill the process using: kill \$(lsof -ti:$WEB_PORT)${NC}"
    exit 1
fi

# Print startup banner
echo -e "${GREEN}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║       Search Engine Web UI Launcher                      ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Start REST API server
echo -e "${BLUE}🚀 Starting REST API server ($REST_SERVER)...${NC}"
cd "$BUILD_DIR"
./"$REST_SERVER" $REST_PORT > /tmp/rest_server.log 2>&1 &
REST_PID=$!

# Wait for REST server to start
echo -e "${YELLOW}   Waiting for REST server to initialize...${NC}"
sleep 2

# Check if REST server is running
if ! kill -0 "$REST_PID" 2>/dev/null; then
    echo -e "${RED}❌ Error: REST server failed to start${NC}"
    echo -e "${YELLOW}   Check logs: tail /tmp/rest_server.log${NC}"
    exit 1
fi

# Verify REST server is responding
if curl -s http://localhost:$REST_PORT/stats >/dev/null 2>&1; then
    echo -e "${GREEN}✅ REST API server running on http://localhost:$REST_PORT${NC}"
else
    echo -e "${YELLOW}⚠️  REST server started but not responding yet (may take a moment)${NC}"
fi

# Start web UI server
echo -e "${BLUE}🌐 Starting web UI server...${NC}"
cd "$WEB_DIR"

# Check which web server is available
if command -v python3 &> /dev/null; then
    python3 -m http.server $WEB_PORT > /tmp/web_server.log 2>&1 &
    WEB_PID=$!
    WEB_SERVER_NAME="Python"
elif command -v python &> /dev/null; then
    python -m http.server $WEB_PORT > /tmp/web_server.log 2>&1 &
    WEB_PID=$!
    WEB_SERVER_NAME="Python"
elif command -v php &> /dev/null; then
    php -S localhost:$WEB_PORT > /tmp/web_server.log 2>&1 &
    WEB_PID=$!
    WEB_SERVER_NAME="PHP"
else
    echo -e "${RED}❌ Error: No web server found (python3, python, or php required)${NC}"
    exit 1
fi

# Wait for web server to start
sleep 1

# Check if web server is running
if ! kill -0 "$WEB_PID" 2>/dev/null; then
    echo -e "${RED}❌ Error: Web server failed to start${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Web UI server running on http://localhost:$WEB_PORT${NC}"

# Print access information
echo -e "\n${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                  🎉 All Systems Ready!                    ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
echo -e ""
echo -e "${BLUE}📡 REST API:${NC}  http://localhost:$REST_PORT"
echo -e "${BLUE}🌐 Web UI:${NC}    ${GREEN}http://localhost:$WEB_PORT${NC}"
echo -e ""
echo -e "${YELLOW}Endpoints:${NC}"
echo -e "  • http://localhost:$REST_PORT/stats"
echo -e "  • http://localhost:$REST_PORT/search?q=query"
echo -e ""
echo -e "${YELLOW}Logs:${NC}"
echo -e "  • REST server: tail -f /tmp/rest_server.log"
echo -e "  • Web server:  tail -f /tmp/web_server.log"
echo -e ""
echo -e "${YELLOW}Press Ctrl+C to stop all servers${NC}"
echo -e ""

# Try to open browser (works on macOS and Linux with xdg-open)
if command -v open &> /dev/null; then
    echo -e "${BLUE}🔗 Opening browser...${NC}"
    sleep 1
    open "http://localhost:$WEB_PORT" 2>/dev/null || true
elif command -v xdg-open &> /dev/null; then
    echo -e "${BLUE}🔗 Opening browser...${NC}"
    sleep 1
    xdg-open "http://localhost:$WEB_PORT" 2>/dev/null || true
fi

# Wait for user to press Ctrl+C
wait
