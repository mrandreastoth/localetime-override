#!/bin/bash

# Execute using "sh enable_debug.sh" OR ". enable_debug.sh"

echo "🔹 Enabling debug mode for localetime override..."
export LOCALETIME_OVERRIDE_DEBUG=1
echo "✅ Debug mode enabled. Debug messages will be printed."