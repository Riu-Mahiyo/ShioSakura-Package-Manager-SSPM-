#!/bin/bash
# build.sh — builds hello-sspm.spk from this source directory
# Usage: bash build.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PKG_NAME="hello-sspm"
PKG_VERSION="1.0.0"
OUTPUT="${SCRIPT_DIR}/../../${PKG_NAME}-${PKG_VERSION}.spk"

echo "[build] Building $PKG_NAME $PKG_VERSION..."

# Create payload directory with the program
mkdir -p "$SCRIPT_DIR/payload/usr/bin"
cat > "$SCRIPT_DIR/payload/usr/bin/hello-sspm" << 'EOF'
#!/bin/sh
echo "Hello from SSPM! 🌸"
EOF
chmod +x "$SCRIPT_DIR/payload/usr/bin/hello-sspm"

# Delegate to sspm build
sspm build "$SCRIPT_DIR" --output "$OUTPUT"

echo "[build] Output: $OUTPUT"
