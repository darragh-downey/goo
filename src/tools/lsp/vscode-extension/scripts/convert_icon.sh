#!/bin/bash
# convert_icon.sh - Script to convert SVG icon to PNG

# Ensure we're in the scripts directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

# Create images directory if it doesn't exist
mkdir -p images

# Check if ImageMagick's convert is available
if command -v convert >/dev/null 2>&1; then
    echo "Converting SVG to PNG..."
    convert -background none -density 300 images/icon.svg -resize 128x128 images/icon.png
    echo "Icon converted successfully!"
    echo "PNG icon saved at images/icon.png"
elif command -v rsvg-convert >/dev/null 2>&1; then
    echo "Converting SVG to PNG using rsvg-convert..."
    rsvg-convert -w 128 -h 128 images/icon.svg > images/icon.png
    echo "Icon converted successfully!"
    echo "PNG icon saved at images/icon.png"
else
    echo "Error: Neither ImageMagick's convert nor rsvg-convert found."
    echo "Please install ImageMagick or librsvg to convert the icon."
    echo ""
    echo "For macOS:"
    echo "  brew install imagemagick"
    echo "For Ubuntu/Debian:"
    echo "  sudo apt-get install imagemagick"
    echo "For Fedora/RHEL:"
    echo "  sudo dnf install imagemagick"
    exit 1
fi

# Check if the PNG was created
if [ -f "images/icon.png" ]; then
    echo "Icon conversion successful!"
else
    echo "Error: Icon conversion failed."
    exit 1
fi 