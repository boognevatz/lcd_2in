#!/bin/bash

# DESCRIPTION variable - specify your text here
DESCRIPTION=$(cat <<EOF
# Build Question

This is a dummy markdown-like text for the build question script.

## Overview

The script processes an array of items and generates a buildquestion.txt file.

## Features

- Outputs description to file
- Loops through array elements
- Appends cat commands and contents

## Usage

Run the script with: ./buildquestion.sh

## Array Items

The array contains:
- FIRST
- SECOND
- THIRD

## Output Format

The output includes the description followed by cat commands for each item.

## Notes

Ensure the files in the array exist before running the script.

## End

This concludes the dummy markdown text.
EOF
)

# Array of items to process
array=("FIRST" "SECOND" "THIRD")

# Output to buildquestion.txt
echo "$DESCRIPTION" > buildquestion.txt

# Loop through the array
for item in "${array[@]}"; do
    echo "cat $item" >> buildquestion.txt
    cat "$item" >> buildquestion.txt
done