#!/usr/bin/env bash
# Run once from this directory to pull header-only dependencies.
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Fetching tiny_gltf.h ..."
curl -fsSL "https://raw.githubusercontent.com/syoyo/tinygltf/v2.8.21/tiny_gltf.h" \
     -o tiny_gltf.h

echo "Fetching stb_image.h ..."
curl -fsSL "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
     -o stb_image.h

echo "Fetching nlohmann/json.hpp ..."
mkdir -p nlohmann
curl -fsSL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" \
     -o nlohmann/json.hpp

echo "All dependencies fetched."
