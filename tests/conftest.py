"""Setup for tests."""

import os
import sys

import pytest

# We need USD to run tests.
usd_root = os.getenv("USD_ROOT")
if not usd_root:
    pytest.skip("USD_ROOT is not set; skipping USD-dependent tests", allow_module_level=True)

sys.path.append(os.path.join(usd_root, "lib", "python"))
