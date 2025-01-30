#!/bin/bash

# Define paths
USR_CONF="$HOME/.config/localetime-override.conf"
SYS_CONF="/etc/localetime-override.conf"
USR_BACKUP="$USR_CONF.bak"
SYS_BACKUP="$SYS_CONF.bak"

# Remove test configurations and restore original ones
echo "🔹 Restoring original configuration files..."

if [[ -f "$USR_BACKUP" ]]; then
    mv "$USR_BACKUP" "$USR_CONF"
    echo "✅ Restored: $USR_CONF"
else
    rm -f "$USR_CONF"
    echo "🗑️ Removed: $USR_CONF (No backup found)"
fi

if [[ -f "$SYS_BACKUP" ]]; then
    sudo mv "$SYS_BACKUP" "$SYS_CONF"
    echo "✅ Restored: $SYS_CONF"
else
    sudo rm -f "$SYS_CONF"
    echo "🗑️ Removed: $SYS_CONF (No backup found)"
fi

echo "✅ Test configs uninstalled successfully."