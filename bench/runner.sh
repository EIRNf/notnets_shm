#!/bin/bash


if [ $# -eq 0 ]; then
    echo "Usage: $0 <argument>"
    exit 1
fi

argument="$1"
current_directory=$(pwd)
output_file="$current_directory/bench_output.txt"

# List all directories in the current directory
directories=$(find . -maxdepth 1 -type d -not -name ".")

# Initialize the output file
echo "Output from benchmarks in each directory using compilation '$argument':" > "$output_file"

# Loop through each directory and run a script if it exists
for directory in $directories; do
    script_path="$current_directory/$directory/run.sh"  # Change 'your_script_name.sh' to the actual script name

    if [ -f "$script_path" ]; then
        echo "Running script in directory: $directory" >> "$output_file"
        echo "------------------------------------" >> "$output_file"
        cd $directory
        bash "$script_path" "$argument" >> "$output_file" #2>&1
        cd $current_directory
        echo "" >> "$output_file"  # Add a blank line between outputs
        echo "------------------------------------" >> "$output_file"
        echo "Finished running script in directory: $directory" >> "$output_file"
    else
        echo "No script found in directory: $directory" >> "$output_file"
    fi
done

echo "All output has been saved to $output_file"
