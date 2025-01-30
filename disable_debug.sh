#!/bin/bash

# Execute using "sh disable_debug.sh" OR ". disable_debug.sh"

echo "🔹 Disabling debug mode for localetime override..."
unset LOCALETIME_OVERRIDE_DEBUG
echo "✅ Debug mode disabled. Debug messages will no longer be printed."