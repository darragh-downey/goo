#!/bin/bash

# build_demos.sh
# Script to build all messaging demos

echo "Building Goo messaging demos..."

# Compile all demos
gcc -o simple_messaging_demo simple_messaging_demo.c -pthread && \
gcc -o pubsub_demo pubsub_demo.c -pthread && \
gcc -o request_reply_demo request_reply_demo.c -pthread && \
gcc -o push_pull_demo push_pull_demo.c -pthread

if [ $? -eq 0 ]; then
    echo "All demos built successfully!"
    echo ""
    echo "Run the demos with:"
    echo "  ./simple_messaging_demo"
    echo "  ./pubsub_demo"
    echo "  ./request_reply_demo"
    echo "  ./push_pull_demo"
else
    echo "Error: Failed to build one or more demos"
    exit 1
fi 