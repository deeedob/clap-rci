#!/bin/bash

# Get list of files using git ls-files
files=$(git ls-files "*.h" "*.hp" "*.cpp")

# Loop through files and apply clang-format
for file in $files; do
    clang-format -i "$file"
done

