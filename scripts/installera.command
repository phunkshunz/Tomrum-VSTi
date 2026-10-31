#!/bin/bash
# Installerar TOMRUM. Kör från Terminal:  bash installera.command
set -e
cd "$(dirname "$0")"

VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"

rm -rf "$VST3/TOMRUM.vst3" "$AU/TOMRUM.component"
cp -R TOMRUM.vst3 "$VST3/"
cp -R TOMRUM.component "$AU/"

# Pluginet är osignerat, så macOS spärrar det. Detta tar bort spärren.
xattr -dr com.apple.quarantine "$VST3/TOMRUM.vst3" "$AU/TOMRUM.component" TOMRUM.app 2>/dev/null || true

echo ""
echo "Klart! TOMRUM ligger nu i:"
echo "  $VST3"
echo "Stäng Bitwig helt och starta igen. TOMRUM finns bland instrumenten."
echo "Lägg TOMRUM på ett instrumentspår och tryck play. Den låter direkt."
