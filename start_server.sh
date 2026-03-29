#!/bin/bash
# Start Audio Server on macOS

echo "🎤 Audio Server Startup Script"
echo "=============================="
echo ""

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 not found. Please install Python 3.8+"
    exit 1
fi

echo "✅ Python3 found"

# Check if requirements installed
echo ""
echo "📦 Checking dependencies..."

python3 -c "import flask" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "⚠️  Missing dependencies. Installing..."
    pip3 install -r requirements.txt
    if [ $? -ne 0 ]; then
        echo "❌ Failed to install requirements"
        exit 1
    fi
fi

echo "✅ All dependencies ready"

# Show IP configuration
echo ""
echo "🌐 Getting server configuration..."
python3 config_esp32.py

# Start server
echo ""
echo "🚀 Starting Audio Server..."
echo ""
python3 audio_server.py
