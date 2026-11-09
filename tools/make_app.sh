#!/bin/sh
# Maakt "Europe Racer.app" in de projectmap: dubbelklikken = het menu.
# Het menu start de race (eracer). Eerst de HD-plaatjes uit je eigen CD
# (alleen wat ontbreekt; de eerste keer lang op de GPU): race-textures
# (tools/make_texture_pack.py) en, als het script er is, het menu
# (tools/upscale_menu.py). ERACER_SKIP_UPSCALE=1 slaat dat over.
# Het icoon (build/EuropeRacer.icns, uit ER.dat van je eigen CD via
# tools/exe_icon.py) staat niet in git.
set -e
cd "$(dirname "$0")/.."
python3 -I tools/make_texture_pack.py
[ -f tools/upscale_menu.py ] && python3 -I tools/upscale_menu.py
if [ ! -f build/EuropeRacer.icns ]; then
  mkdir -p build/icon.iconset
  python3 -I tools/exe_icon.py iso/EuropeRacer/ER.dat build/icon.png
  for s in 16 32 64 128 256 512; do
    sips -z $s $s build/icon.png --out build/icon.iconset/icon_${s}x${s}.png >/dev/null
  done
  iconutil -c icns build/icon.iconset -o build/EuropeRacer.icns || true
fi
APP="Europe Racer.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cat > "$APP/Contents/MacOS/EuropeRacer" <<'SH'
#!/bin/sh
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT" || exit 1
BIN="$ROOT/out/build/release/eracer-menu"
if [ ! -x "$BIN" ] || [ ! -x "$ROOT/out/build/release/eracer" ]; then
  osascript -e 'display alert "Europe Racer" message "Nog niet gebouwd. Bouw eerst eracer en eracer-menu (zie CLAUDE.md)."'
  exit 1
fi
export XDG_DATA_HOME="$ROOT/userdata"
export ERACER_FULLSCREEN="${ERACER_FULLSCREEN:-window}"
exec "$BIN" --game-dir "$ROOT/iso/EuropeRacer" --mods "$ROOT/mods/menu" > "$ROOT/app.log" 2>&1
SH
chmod +x "$APP/Contents/MacOS/EuropeRacer"
[ -f build/EuropeRacer.icns ] && cp build/EuropeRacer.icns "$APP/Contents/Resources/EuropeRacer.icns"
cat > "$APP/Contents/Info.plist" <<'PL'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key><string>Europe Racer</string>
	<key>CFBundleDisplayName</key><string>Europe Racer</string>
	<key>CFBundleIdentifier</key><string>local.europeracer.mac</string>
	<key>CFBundleExecutable</key><string>EuropeRacer</string>
	<key>CFBundleIconFile</key><string>EuropeRacer</string>
	<key>CFBundlePackageType</key><string>APPL</string>
	<key>CFBundleShortVersionString</key><string>0.1</string>
	<key>LSMinimumSystemVersion</key><string>11.0</string>
	<key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PL
codesign --force -s - "$APP" >/dev/null 2>&1 || true
touch "$APP"
echo "OK: $APP"
