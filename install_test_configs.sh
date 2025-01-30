#!/bin/bash

# Define paths
USR_TEST_CONF="$PWD/test_usr_localetime-override.conf"
SYS_TEST_CONF="$PWD/test_sys_localetime-override.conf"
USR_CONF="$HOME/.config/localetime-override.conf"
SYS_CONF="/etc/localetime-override.conf"

# Backup paths
USR_BACKUP="$USR_CONF.bak"
SYS_BACKUP="$SYS_CONF.bak"

# Backup existing conf files
echo "🔹 Backing up existing configuration files..."
[[ -f "$USR_CONF" ]] && cp "$USR_CONF" "$USR_BACKUP"
[[ -f "$SYS_CONF" ]] && sudo cp "$SYS_CONF" "$SYS_BACKUP"

# Copy test config files
echo "🔹 Applying test configurations..."
mkdir -p "$HOME/.config"
cp "$USR_TEST_CONF" "$USR_CONF"
sudo cp "$SYS_TEST_CONF" "$SYS_CONF"

# Verify files
echo "🔹 Verifying installation..."
ls -l "$USR_CONF" "$SYS_CONF"
echo "🔹 Contents of user configuration file ($USR_CONF):"
cat "$USR_CONF"
echo "🔹 Contents of system configuration file ($SYS_CONF):"
sudo cat "$SYS_CONF"

echo "✅ Test configs installed successfully."