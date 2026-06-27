"""Setup for tests."""
import os
import sys
# from pathlib import Path

# we need USD to run tests
usd_path = os.getenv("USD_ROOT")
sys.path.append(usd_path)
