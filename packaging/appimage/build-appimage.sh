#!/usr/bin/env bash
# Build the Linux x86_64 player and package a Steam Deck / Linux AppImage.
#
# Build on the oldest glibc you intend to support. This project targets
# Ubuntu 22.04 (glibc 2.35), which is older than SteamOS 3.x (glibc 2.38),
# so the resulting AppImage runs on Steam Deck.
#
# Prerequisites: gcc, g++, cmake 3.21+, make, libsdl2-dev, wget, python3.
# Usage:
#   ./packaging/appimage/build-appimage.sh [ROM_DIR] [OUTPUT_APPIMAGE]
# Defaults: ROM_DIR=./roms
#           OUTPUT=./dist/steamdeck/Last Blade 2-SteamDeck-x86_64.AppImage
#
# No ROMs or BIOS files are copied into the AppImage. The player reads them
# from --rom-path, a roms/ folder beside the AppImage, or the usual Deck
# locations (see AppRun below).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ROMS="${1:-$ROOT/roms}"
OUT="${2:-$ROOT/dist/steamdeck/Last Blade 2-SteamDeck-x86_64.AppImage}"
BUILD="${BUILD_DIR:-$ROOT/build-linux}"
APPDIR="$BUILD/AppDir"

echo "== build (Release, glibc $(ldd --version | awk 'NR==1{print $NF}')) =="
if [ ! -f "$BUILD/CMakeCache.txt" ]; then
    cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release "-DLASTBLADE2_ROM_DIR=$ROMS"
fi
cmake --build "$BUILD" --target lastblade2 --parallel "${JOBS:-$(nproc)}"

echo "== assemble AppDir =="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/metainfo" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
         "$APPDIR/usr/share/doc/lastblade2" \
         "$APPDIR/usr/share/licenses/lastblade2/LICENSES"
cp "$BUILD/lastblade2" "$APPDIR/usr/bin/lastblade2"
strip --strip-unneeded "$APPDIR/usr/bin/lastblade2" 2>/dev/null || true

# Bundle the SDL2 the build linked against so the AppImage is portable.
SDL=$(ldd "$BUILD/lastblade2" | awk '/libSDL2-2\.0\.so/{print $3; exit}')
if [ -n "$SDL" ]; then
    cp -L "$SDL" "$APPDIR/usr/lib/$(basename "$SDL")"
    echo "bundled $(basename "$SDL")"
else
    echo "warning: libSDL2 was not found by ldd; the AppImage will use the system SDL2"
fi

python3 "$ROOT/packaging/appimage/make_icon.py"
cp "$ROOT/packaging/appimage/lastblade2.png" "$APPDIR/lastblade2.png"
cp "$ROOT/packaging/appimage/lastblade2.png" \
   "$APPDIR/usr/share/icons/hicolor/256x256/apps/lastblade2.png"
ln -sf lastblade2.png "$APPDIR/.DirIcon"

# License and documentation must travel with the binary.
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" \
   "$ROOT/LICENSE-neogeorecomp" "$ROOT/NOTICE-neogeorecomp" \
   "$APPDIR/usr/share/licenses/lastblade2/"
cp "$ROOT"/LICENSES/* "$APPDIR/usr/share/licenses/lastblade2/LICENSES/"
cp "$ROOT/README.md" "$ROOT/docs/game-notes.md" "$ROOT/docs/compatibility.md" \
   "$APPDIR/usr/share/doc/lastblade2/"

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Prefer fullscreen on a handheld; --windowed or --headless overrides it.
has_mode=0
for a in "$@"; do
    case "$a" in --fullscreen|--windowed|--headless) has_mode=1 ;; esac
done
if [ "$has_mode" = 0 ]; then
    set -- --fullscreen "$@"
fi

# If no --rom-path was given, look for the lastbld2 set beside the AppImage
# or in the usual Deck / desktop locations. A directory is only accepted if
# it actually contains 243-pg1.p1.
if [ -n "${APPIMAGE:-}" ]; then
    has_rompath=0
    for a in "$@"; do [ "$a" = "--rom-path" ] && has_rompath=1; done
    if [ "$has_rompath" = 0 ]; then
        for d in "$(dirname "$APPIMAGE")/roms" \
                 "$HOME/Emulation/roms/neogeo/lastbld2" \
                 "$HOME/Emulation/roms/arcade" \
                 "$HOME/roms/neogeo/lastbld2" \
                 "$HOME/roms/neogeo" \
                 "$HOME/roms/lastbld2" \
                 "$HOME/roms"; do
            if [ -f "$d/243-pg1.p1" ]; then
                set -- --rom-path "$d" "$@"
                break
            fi
        done
    fi
fi

exec "$HERE/usr/bin/lastblade2" "$@"
EOF
chmod +x "$APPDIR/AppRun"

cat > "$APPDIR/lastblade2.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=The Last Blade 2
Comment=Native static recompilation of The Last Blade 2 (Neo Geo)
Exec=lastblade2
Icon=lastblade2
Categories=Game;Emulator;
Terminal=false
X-AppImage-Name=The Last Blade 2
EOF
cp "$APPDIR/lastblade2.desktop" "$APPDIR/usr/share/applications/lastblade2.desktop"

cat > "$APPDIR/usr/share/metainfo/lastblade2.appdata.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>io.github.omegakatana92.lastblade2</id>
  <name>The Last Blade 2</name>
  <summary>Native static recompilation of The Last Blade 2</summary>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>LicenseRef-PolyForm-Noncommercial-1.0.0</project_license>
  <description>
    <p>The Last Blade 2 (Bakumatsu Rouman Dai Ni Maku, SNK 1998) with its
    Motorola 68000 program statically recompiled to C and run natively on the
    neogeorecomp Neo Geo board model. Requires your own legally obtained
    lastbld2 and neogeo ROM dumps.</p>
  </description>
  <launchable type="desktop-id">lastblade2.desktop</launchable>
  <provides><binary>lastblade2</binary></provides>
  <categories><category>Game</category></categories>
</component>
EOF

echo "== appimagetool =="
TOOL="${APPIMAGETOOL:-$BUILD/appimagetool.AppImage}"
if [ ! -x "$TOOL" ]; then
    wget -q -O "$TOOL" \
        https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$TOOL"
fi
mkdir -p "$(dirname "$OUT")"
ARCH=x86_64 "$TOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "$OUT"
chmod +x "$OUT"
echo "wrote $OUT"
