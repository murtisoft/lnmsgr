#!/bin/bash
#================================================================================================
#	This script is for creating a deployment zip archive out of the compiled executable.
#	This is for windows, but it should be easily applicable to other systems.
#	Edit the variables given at the top to your liking.
#	The rest of the script should work fine on any system.
#================================================================================================
SevenZip="/c/Program Files/7-Zip/7z.exe"
WinDeployQt="/c/Qt/6.10.1/mingw_64/bin/windeployqt.exe"
OpenSSL="/c/Qt/Tools/OpenSSL-Win64"
ProjectDir="../../"
#================================================================================================

if [ ! -f "$ProjectDir/build/Release/LanMessenger.exe" ]; then
    echo "Error: Source executable not found."
    read -p "Press enter to exit..."
    exit 1
fi

rm -rf "$ProjectDir/build/Deployment"
mkdir -p "$ProjectDir/build/Deployment"

cp "$ProjectDir/build/Release/LanMessenger.exe" "$ProjectDir/build/Deployment/"
"$WinDeployQt" "$ProjectDir/build/Deployment/LanMessenger.exe"

rm -rf "$ProjectDir/build/Deployment/translations"
rm -f "$ProjectDir/build/Win64.zip"

cp "$OpenSSL/libcrypto-3-x64.dll" "$ProjectDir/build/Deployment/"
cp "$OpenSSL/libssl-3-x64.dll" "$ProjectDir/build/Deployment/"

"$SevenZip" a "$ProjectDir/build/Win64.zip" "$ProjectDir/build/Deployment/*"
cp "$ProjectDir/build/Win64.zip" ~/Desktop/

read -p "Done. Press enter to continue..."