#!/bin/sh

# Get version
echo "Please enter the MCUS version (e.g. 0.1.1):"
read version

# Build for Windows
cd ..
./configure --host=i586-mingw32msvc || exit 1
make clean
make || exit 1

# Remove the old packages
rm -f mcus-$version.zip
rm -f mcus-$version.exe

# Copy the compiled files into our Windows directory tree
cp src/.libs/mcus.exe packaging/GTK2-Runtime/lib
cp data/mcus.ui packaging/GTK2-Runtime/share/mcus/
cp data/ocr-assembly.lang packaging/GTK2-Runtime/share/mcus/
cp help/pdf/mcus-pdf.pdf packaging/GTK2-Runtime/share/mcus/help.pdf
cp data/icons/16x16/mcus.png packaging/GTK2-Runtime/share/icons/hicolor/16x16/apps
cp data/icons/22x22/mcus.png packaging/GTK2-Runtime/share/icons/hicolor/22x22/apps
cp data/icons/32x32/mcus.png packaging/GTK2-Runtime/share/icons/hicolor/32x32/apps
cp data/icons/48x48/mcus.png packaging/GTK2-Runtime/share/icons/hicolor/48x48/apps

cd packaging

# Building zip archive
mv GTK2-Runtime mcus-$version
zip -r mcus-$version.zip mcus-$version -x \*~ || exit 1
mv mcus-$version GTK2-Runtime

# Building NSIS installer
makensis mcus.nsi || exit 1
