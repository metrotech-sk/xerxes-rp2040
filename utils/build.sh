#!/bin/bash

# Define the green color code
GREEN='\033[0;32m'
# Define the no color code to reset the color
NC='\033[0m'

# Check if exactly two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 start_address end_address"
    exit 1
fi

# Assign the arguments to variables
start_address=$1
end_address=$2

# Check if the start and end addresses are valid numbers
if ! [[ "$start_address" =~ ^[0-9]+$ ]] || ! [[ "$end_address" =~ ^[0-9]+$ ]]; then
    echo "Both start and end addresses must be valid numbers."
    exit 1
fi

# Iterate over the range of addresses
for i in $(seq $start_address $end_address); do
    cmake .. -DDEVICE_ADDRESS=$i -DCMAKE_BUILD_TYPE=Release -GNinja
    ninja

    # Wait for user input before proceeding
    echo "Press Enter to continue with 'ninja install' for DEVICE_ADDRESS=$i"
    read

    ninja install

    # Run picocom and ensure it exits cleanly
    while true; do
        picocom /dev/ttyACM0 -b 115200
        if [ $? -eq 0 ]; then
        
            echo -e "${GREEN}NUMBER $i is"
            cat << "EOF"
██████╗  ██████╗ ███╗   ██╗███████╗
██╔══██╗██╔═══██╗████╗  ██║██╔════╝
██║  ██║██║   ██║██╔██╗ ██║█████╗  
██║  ██║██║   ██║██║╚██╗██║██╔══╝  
██████╔╝╚██████╔╝██║ ╚████║███████╗
╚═════╝  ╚═════╝ ╚═╝  ╚═══╝╚══════╝
EOF
            echo -e "${NC}"
            break
        else
            echo "picocom failed to exit cleanly. Retrying..."
            sleep 1
        fi
    done
done