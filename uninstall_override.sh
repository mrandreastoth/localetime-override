#!/bin/bash
set -e

LIB_PATH="/usr/lib/liblocaletime_override.so"

echo "Removing override library..."
sudo rm -f "$LIB_PATH"

echo "Removing LD_PRELOAD entry..."
sudo sed -i '/liblocaletime_override.so/d' /etc/profile

echo "Reloading environment..."
source /etc/profile

echo "Uninstallation complete!"