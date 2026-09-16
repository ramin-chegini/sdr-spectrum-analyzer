#!/bin/bash

TARGET="root@192.168.2.10"
REMOTE_PATH="/usr/bin/sdr_app"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_PATH="$SCRIPT_DIR/sdr_app"

echo "======================================"
echo "       Deploying sdr_app"
echo "======================================"

if [ ! -f "$LOCAL_PATH" ]; then
    echo "ERROR: $LOCAL_PATH not found!"
    echo "Run 'make' first."
    exit 1
fi

echo "[1/4] Stopping sdr-app..."
ssh "$TARGET" "sdr-app stop"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to stop sdr-app!"
    exit 1
fi

echo "[2/4] Copying sdr_app..."
scp "$LOCAL_PATH" "$TARGET:$REMOTE_PATH"

if [ $? -ne 0 ]; then
    echo "ERROR: Deploy failed!"
    echo "Starting previous sdr_app instance..."
    ssh "$TARGET" "sdr-app start"
    exit 1
fi

echo "[3/4] Setting executable permission..."
ssh "$TARGET" "chmod +x $REMOTE_PATH"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to set executable permission!"
    ssh "$TARGET" "sdr-app start"
    exit 1
fi

echo "[4/4] Starting sdr-app..."
ssh "$TARGET" "sdr-app start"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to start sdr-app!"
    exit 1
fi

echo ""
echo "======================================"
echo "       Deploy successful! 🚀"
echo "======================================"