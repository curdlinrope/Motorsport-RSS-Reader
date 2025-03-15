#!/bin/bash

# Build the application
qmake
make

# Create AppDir structure
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps

# Copy files
cp MotorsportRSS AppDir/usr/bin/
cp MotorsportRSS.desktop AppDir/usr/share/applications/
cp resources/icons/motorsportrss.png AppDir/usr/share/icons/hicolor/256x256/apps/

# Create AppRun file
cat > AppDir/AppRun << 'EOF'
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$APPDIR/usr/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$APPDIR/usr/lib/qt5/plugins"
export QML2_IMPORT_PATH="$APPDIR/usr/lib/qml"
"$APPDIR/usr/bin/MotorsportRSS" "$@"
EOF

chmod +x AppDir/AppRun

# Download linuxdeployqt if not present
if [ ! -f linuxdeployqt-continuous-x86_64.AppImage ]; then
    wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod +x linuxdeployqt-continuous-x86_64.AppImage
fi

# Build AppImage
./linuxdeployqt-continuous-x86_64.AppImage AppDir/usr/share/applications/MotorsportRSS.desktop -appimage

echo "AppImage created!" 