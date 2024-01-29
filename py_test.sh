#!/bin/bash

VENV_DIR=".venv"
REQUIREMENTS_FILE="requirements.txt"

# Check if the virtual environment exists
if [ -d "$VENV_DIR" ]; then
    echo "Virtual environment exists. Activating..."
    source "$VENV_DIR/bin/activate"
else
    echo "Virtual environment doesn't exist. Creating..."
    python -m venv "$VENV_DIR"
    source "$VENV_DIR/bin/activate"

    # Install dependencies
    if [ -f "$REQUIREMENTS_FILE" ]; then
        echo "Installing dependencies from $REQUIREMENTS_FILE..."
        pip install -r "$REQUIREMENTS_FILE"
    else
        echo "No requirements file found."
    fi
fi

# move into the testing dir 
cd test

PY_TEST_ENTY_FILE="test_main.py"

echo "Starting testing, entry py-file: $PYTHON_FILE..."
python "$PYTHON_FILE"

# Deactivate the virtual environment when done
deactivate

