#!/bin/bash

echo "=== Checking Build Status ==="
echo "Build cache directories exist: $(test -d 'Intermediate' && echo 'YES' || echo 'NO')"
echo "Binary directories exist: $(test -d 'Binaries' && echo 'YES' || echo 'NO')"

echo "=== Checking for Built Binaries ==="
if find Binaries -name "*.dylib" -type f 2>/dev/null | grep -q .; then
    echo "Found built libraries:"
    find Binaries -name "*.dylib" -type f 2>/dev/null
else
    echo "No built libraries found"
fi

echo "=== Checking UnrealBuildTool Processes ==="
ps aux | grep -i unreal | grep -v grep || echo "No UBT processes running"

echo "=== Attempting Quick Build Check ==="
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" BackRooomsUE57Editor Mac Development -Project="/Users/agus/repo/BackRooomsUE57/BackRooomsUE57.uproject" -WaitMutex -NoAction 2>&1 | head -20