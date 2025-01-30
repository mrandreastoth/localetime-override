#!/bin/bash

LIB_NAME="liblocaletime_override.so"
LIB_PATH="/usr/local/lib/$LIB_NAME"
LD_PRELOAD_FILE="/etc/ld.so.preload"
CACHE_FILE="/tmp/localetime_cache.dat"

echo "🔹 Uninstalling $LIB_NAME..."

# Ensure script is run as root
if [[ $EUID -ne 0 ]]; then
    echo "❌ Error: This script must be run as root (use sudo)."
    exit 1
fi

# Remove the library from /usr/local/lib/
if [[ -f "$LIB_PATH" ]]; then
    echo "🗑️ Removing $LIB_PATH..."
    rm -f "$LIB_PATH"
else
    echo "⚠️ $LIB_PATH not found, skipping."
fi

# Remove the entry from ld.so.preload if it exists
if grep -q "$LIB_PATH" "$LD_PRELOAD_FILE" 2>/dev/null; then
    echo "🔄 Removing $LIB_NAME from $LD_PRELOAD_FILE..."
    sed -i "\|$LIB_PATH|d" "$LD_PRELOAD_FILE"
else
    echo "✅ $LIB_NAME was not in $LD_PRELOAD_FILE, skipping."
fi

# Remove the cache file
if [[ -f "$CACHE_FILE" ]]; then
    echo "🗑️ Removing cache file: $CACHE_FILE..."
    rm -f "$CACHE_FILE"
else
    echo "⚠️ Cache file not found, skipping."
fi

# Refresh the dynamic linker cache
echo "🔄 Running ldconfig..."
ldconfig

echo "✅ Uninstallation complete. $LIB_NAME has been removed."