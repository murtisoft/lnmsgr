#!/bin/bash
#================================================================================================
#	This script is for creating a deployment zip archive out of the compiled executable.
#	This is for windows, but it should be easily applicable to other systems.
#	Edit the variables given at the top to your liking.
#	The rest of the script should work fine on any system.
#================================================================================================
SEVENZIP="/c/Program Files/7-Zip/7z.exe"
WINDEPLOYQT="/c/Qt/6.10.1/mingw_64/bin/windeployqt.exe"
OPENSSL="/c/Qt/Tools/OpenSSL-Win64"
PROJECTDIR="../../"
#================================================================================================

if [ ! -f "$PROJECTDIR/build/Release/LanMessenger.exe" ]; then
    echo "Error: Source executable not found."
    read -p "Press enter to exit..."
    exit 1
fi

rm -rf "$PROJECTDIR/build/Deployment"
mkdir -p "$PROJECTDIR/build/Deployment"

cp "$PROJECTDIR/build/Release/LanMessenger.exe" "$PROJECTDIR/build/Deployment/"
"$WINDEPLOYQT" "$PROJECTDIR/build/Deployment/LanMessenger.exe"

rm -rf "$PROJECTDIR/build/Deployment/translations"
rm -f "$PROJECTDIR/build/Win64.zip"

cp "$OPENSSL/libcrypto-3-x64.dll" "$PROJECTDIR/build/Deployment/"
cp "$OPENSSL/libssl-3-x64.dll" "$PROJECTDIR/build/Deployment/"

"$SEVENZIP" a "$PROJECTDIR/build/Win64.zip" "$PROJECTDIR/build/Deployment/*"
cp "$PROJECTDIR/build/Win64.zip" ~/Desktop/

read -p "Done. Press enter to continue..."