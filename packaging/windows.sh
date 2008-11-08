#!/bin/sh

# Get version
print "Please enter the MCUS version (e.g. 0.1.1):"
read version

# Build for Windows
cd ../
./autogen.sh --target=i586-mingw32msvc --host=i586-mingw32msvc
make clean
make

# Copy the compiled files into our Windows directory tree
cp src/.libs/mcus.exe packaging/GTK2-Runtime/lib
cp data/mcus.ui packaging/GTK2-Runtime/lib/data
cp data/ocr-assembly.lang packaging/GTK2-Runtime/lib/data
cp help/pdf/mcus-pdf.pdf packaging/GTK2-Runtime/lib/data/help.pdf

cd packaging

# Building zip archive
zip -r mcus-$version.zip GTK2-Runtime

# Building NSIS installer
makensis mcus.nsi
