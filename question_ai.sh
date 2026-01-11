#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

input_file="$1"
output_file="${input_file%.*}_ai.txt"

if [ ! -f "$input_file" ]; then
    echo "Input file $input_file not found"
    exit 1
fi

# Read description until ---
description=""
reading_desc=true
filelist=()
while IFS= read -r line; do
    if [ "$line" = "---" ]; then
        reading_desc=false
        continue
    fi
    if $reading_desc; then
        description+="$line\n"
    else
        if [[ "$line" == FILELIST:* ]]; then
            continue
        fi
        if [ -n "$line" ]; then
            filelist+=("$line")
        fi
    fi
done < "$input_file"

# Output to file
echo -e "$description" > "$output_file"

for file in "${filelist[@]}"; do
    echo "cat $file" >> "$output_file"
    if [ -f "$file" ]; then
        cat "$file" >> "$output_file"
    else
        echo "Warning: $file not found" >> "$output_file"
    fi
done
