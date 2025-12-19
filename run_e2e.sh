#!/bin/bash
# E2E Test Script for GStreamer Player

APP="./simple_player"
DURATION=5

if [ ! -f "$APP" ]; then
    echo "Error: $APP not found. Please run 'make' first."
    exit 1
fi

echo "Running E2E test for $DURATION seconds..."

# Start the player in the background
$APP &
PID=$!

# Wait for the specified duration
sleep $DURATION

# Check if the process is still running
if ps -p $PID > /dev/null; then
    echo "SUCCESS: Player is still running after $DURATION seconds."
    # Gracious shutdown test could go here, but for now we kill
    kill -SIGINT $PID 2>/dev/null
    wait $PID 2>/dev/null
    exit 0
else
    echo "FAILURE: Player process exited prematurely."
    # Retrieve exit code if possible
    wait $PID
    EXIT_CODE=$?
    echo "Exit code: $EXIT_CODE"
    exit 1
fi
