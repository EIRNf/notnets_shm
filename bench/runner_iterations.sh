#!/bin/bash


if [ $# -eq 0 ]; then
    echo "Usage: $0 <argument>"
    exit 1
fi

argument="$1"
iterations="$2"
current_directory=$(pwd)
cpu_util_script="$current_directory/cpu.sh"
output_file="$current_directory/bench_output.txt"
output_cpu_file="$current_directory/bench_cpu_output.txt"


# List all directories in the current directory
directories=$(find . -maxdepth 1 -type d -not -name ".")

# Initialize the output file
echo "Output from benchmarks in each directory using compilation '$argument':" > "$output_file"

# Initialize the output file
echo "CPU utilization during execution '$argument':" > "$output_cpu_file"

# Loop through each directory and run a script if it exists
for directory in $directories; do
    script_path="$current_directory/$directory/run.sh" 

    if [ -f "$script_path" ]; then
        echo "Running script in directory: $directory" >> "$output_file"
        echo "------------------------------------" >> "$output_file"
        echo "" >> "$output_cpu_file"
        echo "Running script in directory: $directory" >> "$output_cpu_file"
        echo "------------------------------------" >> "$output_cpu_file"
        cd $directory
        for ((i=1; i<=$iterations; i++)); do
        echo -e "\nClients: $i\n" >> "$output_cpu_file"
        bash "$cpu_util_script" >> "$output_cpu_file" &
        PID=$!
        bash "$script_path" "$argument -DMAX_CLIENTS=$i">> "$output_file" #2>&1
        kill -9 $PID
        done
        cd $current_directory
        echo "" >> "$output_file" 
        echo "------------------------------------" >> "$output_file"
        echo "Finished running script in directory: $directory" >> "$output_file"
    else
        echo "No script found in directory: $directory" >> "$output_file"
    fi
done

echo "All output has been saved to $output_file"
