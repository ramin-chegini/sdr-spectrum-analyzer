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

echo "[1/3] Stopping sdr_app..."
ssh "$TARGET" "killall sdr_app 2>/dev/null || true"

echo "[2/3] Copying sdr_app..."
scp "$LOCAL_PATH" "$TARGET:$REMOTE_PATH"

if [ $? -ne 0 ]; then
    echo "ERROR: Deploy failed!"
    exit 1
fi

echo "[3/3] Setting executable permission..."
ssh "$TARGET" "chmod +x $REMOTE_PATH"

echo ""
echo "======================================"
echo "       Deploy successful! 🚀"
echo "======================================"