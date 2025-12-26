#!/bin/bash
# Clean build script for BackRooms UE5.7 project

echo "Cleaning build artifacts..."
rm -rf Binaries/ Intermediate/ DerivedDataCache/ .vs/

echo "Opening project for clean rebuild..."
open "BackRooomsUE57.uproject"

echo "Clean build initiated. Project will rebuild automatically."