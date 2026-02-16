#!/bin/bash
# Launcher script for Greenhouse GUI

echo "🌱 Greenhouse IoT - GUI Launcher"
echo "================================"

# Check Python version
python_version=$(python3 --version 2>&1 | awk '{print $2}')
echo "Python version: $python_version"

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
source venv/bin/activate

# Install/upgrade dependencies
echo "Checking dependencies..."
pip install -q --upgrade pip
pip install -q -r requirements.txt

# Run GUI
echo ""
echo "Starting Greenhouse GUI..."
echo "Press Ctrl+C to exit"
echo ""

python greenhouse_gui.py

# Deactivate on exit
deactivate
