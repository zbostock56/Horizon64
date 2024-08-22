#!/bin/bash

# Port to check
PORT=1234

# Find the PIDs of processes using the specified port
pids=$(lsof -t -i :$PORT)

# Check if any PIDs were found
if [ -z "$pids" ]; then
  echo "No processes are using port $PORT."
  exit 0
fi

# Iterate over each PID and terminate the process
for pid in $pids; do
  echo "Terminating process with PID $pid using port $PORT."
  kill -9 $pid
done

echo "All processes using port $PORT have been terminated."
