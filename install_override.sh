#!/bin/bash
set -e

LIB_PATH="/usr/lib/liblocaletime_override.so"

echo "Compiling override library..."
g++ -shared -fPIC -o liblocaletime_override.so override.cpp -ldl

echo "Installing override library..."
sudo mv liblocaletime_override.so "$LIB_PATH"
echo "export LD_PRELOAD=$LIB_PATH" | sudo tee -a /etc/profile

echo "Reloading environment..."
source /etc/profile

echo "Installation complete! Test with:"
echo '  date +"%A, %B %d %Y"'