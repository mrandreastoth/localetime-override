#!/bin/bash

LIB_NAME="liblocaletime_override.so"
LIB_PATH="/usr/local/lib/$LIB_NAME"
LD_PRELOAD_FILE="/etc/ld.so.preload"

echo "🔹 Installing $LIB_NAME..."

# Ensure script is run as root
if [[ $EUID -ne 0 ]]; then
    echo "❌ Error: This script must be run as root (use sudo)."
    exit 1
fi

# Check if the compiled library exists in the current directory
if [[ ! -f "$LIB_NAME" ]]; then
    echo "❌ Error: $LIB_NAME not found in the current directory."
    exit 1
fi

# Copy the library to /usr/local/lib/
echo "📂 Copying $LIB_NAME to $LIB_PATH..."
cp "$LIB_NAME" "$LIB_PATH"
chmod 755 "$LIB_PATH"

# Ensure library is added to ld.so.preload
if ! grep -q "$LIB_PATH" "$LD_PRELOAD_FILE" 2>/dev/null; then
    echo "🔄 Adding $LIB_NAME to $LD_PRELOAD_FILE..."
    echo "$LIB_PATH" >> "$LD_PRELOAD_FILE"
else
    echo "✅ $LIB_NAME is already in $LD_PRELOAD_FILE."
fi

# Refresh the dynamic linker cache
echo "🔄 Running ldconfig..."
ldconfig

echo "✅ Installation complete. $LIB_NAME is now preloaded system-wide."