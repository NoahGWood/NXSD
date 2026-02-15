#!/bin/bash

# 1. Variables - Match your template exactly
OLD="NTemplate"
CORE="NTemplate"

read -p "Enter new project name: " NEW

if [ -z "$NEW" ]; then echo "Name required."; exit 1; fi

echo "Phase 1: Updating text inside files..."
# This fixes your CMakeLists.txt and #include statements
find . -type f -not -path '*/.*' -not -path '*/vendor/*' -exec sed -i "s/$OLD/$NEW/g" {} + 2>/dev/null || \
find . -type f -not -path '*/.*' -not -path '*/vendor/*' -exec sed -i '' "s/$OLD/$NEW/g" {} +

find . -type f -not -path '*/.*' -not -path '*/vendor/*' -exec sed -i "s/$CORE/$NEW/g" {} + 2>/dev/null || \
find . -type f -not -path '*/.*' -not -path '*/vendor/*' -exec sed -i '' "s/$CORE/$NEW/g" {} +

echo "Phase 2: Renaming folders and files (Deepest first)..."
# We use -depth to ensure we rename include/NTemplate/NTemplate.h 
# BEFORE we rename the include/NTemplate folder itself.
echo "Phase 2: Renaming folders (skipping vendor/)..."
# -depth ensures children are renamed before parents
# -prune is used here to prevent find from even entering the vendor directory
find . -depth \( -path "*/vendor/*" -prune \) -o \( -name "*$OLD*" -o -name "*$CORE*" \) -print | while read -r path; do
    dir=$(dirname "$path")
    base=$(basename "$path")
    
    new_base=$(echo "$base" | sed "s/$OLD/$NEW/g; s/$CORE/$NEW/g")
    
    if [ "$base" != "$new_base" ]; then
        mv "$path" "$dir/$new_base"
    fi
done
echo "Done! Your structure is now personalized to $NEW."