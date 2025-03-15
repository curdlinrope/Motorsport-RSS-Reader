#!/bin/bash

# Build the application
qmake
make

# Create package directory structure
mkdir -p packaging/deb/usr/bin
mkdir -p packaging/deb/usr/share/applications
mkdir -p packaging/deb/usr/share/icons/hicolor/256x256/apps

# Copy files
cp MotorsportRSS packaging/deb/usr/bin/
cp MotorsportRSS.desktop packaging/deb/usr/share/applications/
cp resources/icons/motorsportrss.png packaging/deb/usr/share/icons/hicolor/256x256/apps/

# Build the package
dpkg-deb --build packaging/deb motorsportrss_1.0.0_amd64.deb

echo "Package created: motorsportrss_1.0.0_amd64.deb" 