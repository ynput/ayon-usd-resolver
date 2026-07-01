"""Simple build helper for AYON USD Resolver.

Automatically detects USD/DCC environment and runs CMake build.
Adds optional zipping of the install directory.
"""
from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys


def run(
        cmd: str | list[str],
        cwd: str | None = None,
        env: dict[str, str] | None = None) -> None:
    """Run a shell command with logging."""
    print(f">>> {cmd}, cwd={cwd}")
    result = subprocess.run(
        cmd, cwd=cwd, env=env, check=False, text=True)
    if result.returncode != 0:
        sys.exit(result.returncode)


def get_vcpkg_triplet() -> None:
    """Determine appropriate vcpkg triplet for current architecture.

    Uses the -static-md variant: static library, dynamic CRT (/MD).
    This is required for static libmemcached in DCC plugin builds.

    """
    machine = platform.machine().lower()

    if machine in {"amd64", "x86_64"}:
        return "x64-windows-static-md"
    if machine in {"x86", "i386", "i686"}:
        return "x86-windows-static-md"
    if machine.startswith("arm64"):
        return "arm64-windows-static-md"

    print(f"[WARNING] Unknown architecture: {machine}, defaulting to x64-windows-static-md")
    return "x64-windows-static-md"


def find_libmemcached_vcpkg(vcpkg_root, triplet):
    """Find libmemcached-awesome in vcpkg installation."""
    packages_dir = os.path.join(vcpkg_root, "installed", triplet)
    include_path = os.path.join(packages_dir, "include", "libmemcached-1.0")
    lib_path = os.path.join(packages_dir, "lib")
    
    if os.path.exists(include_path) and os.path.exists(lib_path):
        print(f"[INFO] Found libmemcached via vcpkg: {packages_dir}")
        return packages_dir
    
    return None


def try_install_via_vcpkg(vcpkg_root, triplet):
    """Install libmemcached using vcpkg."""
    print(f"[INFO] Installing libmemcached-awesome via vcpkg (triplet: {triplet})...")
    
    vcpkg_exe = os.path.join(vcpkg_root, "vcpkg.exe")
    cmd = [vcpkg_exe, "install", f"libmemcached-awesome:{triplet}"]
    
    try:
        result = subprocess.run(cmd, check=False, capture_output=False, text=True)
        if result.returncode == 0:
            print("[INFO] Successfully installed libmemcached via vcpkg")
            # Return the vcpkg path for memcached
            packages_dir = os.path.join(vcpkg_root, "installed", triplet)
            if os.path.exists(packages_dir):
                return packages_dir
    except Exception as e:
        print(f"[WARNING] Failed to install libmemcached via vcpkg: {e}")
    
    return None


def find_libmemcached():
    """Try to find libmemcached installation path."""
    system = platform.system()
    
    # Try pkg-config first (works on Linux and macOS)
    try:
        output = subprocess.check_output(
            ["pkg-config", "--variable=prefix", "libmemcached"],
            stderr=subprocess.DEVNULL,
            text=True
        ).strip()
        if output and os.path.exists(output):
            print(f"[INFO] Found libmemcached via pkg-config: {output}")
            return output
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    
    # Common installation paths to check
    search_paths = []
    
    if system == "Linux":
        search_paths = [
            "/usr",
            "/usr/local",
            "/opt/libmemcached",
            os.path.expanduser("~/.local"),
        ]
    elif system == "Darwin":  # macOS
        search_paths = [
            "/usr/local/opt/libmemcached",  # Homebrew
            "/usr/local",
            "/opt/homebrew/opt/libmemcached",  # Apple Silicon Homebrew
        ]
    elif system == "Windows":
        search_paths = [
            "C:\\Program Files\\libmemcached",
            "C:\\Program Files (x86)\\libmemcached",
        ]        
        # Also check vcpkg installations
        vcpkg_root = find_vcpkg()
        if vcpkg_root:
            triplet = get_vcpkg_triplet()
            memcached_path = find_libmemcached_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path    
    # Check standard paths
    for base_path in search_paths:
        include_path = os.path.join(base_path, "include", "libmemcached.h")
        lib_path = os.path.join(base_path, "lib")
        
        if os.path.exists(include_path) and os.path.exists(lib_path):
            print(f"[INFO] Found libmemcached at: {base_path}")
            return base_path
    
    return None


def find_vcpkg():
    """Find vcpkg installation on Windows."""
    if platform.system() != "Windows":
        return None
    
    # Check environment variable first
    vcpkg_root = os.environ.get("VCPKG_ROOT")
    if vcpkg_root and os.path.exists(os.path.join(vcpkg_root, "vcpkg.exe")):
        print(f"[INFO] Found vcpkg via VCPKG_ROOT: {vcpkg_root}")
        return vcpkg_root
    
    # Common vcpkg installation paths
    search_paths = [
        "C:\\vcpkg",
        os.path.expanduser("~/vcpkg"),
        os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "vcpkg"),
    ]
    
    for vcpkg_path in search_paths:
        vcpkg_exe = os.path.join(vcpkg_path, "vcpkg.exe")
        if os.path.exists(vcpkg_exe):
            print(f"[INFO] Found vcpkg at: {vcpkg_path}")
            return vcpkg_path
    
    return None





def try_install_via_vcpkg(vcpkg_root, triplet):
    """Install libmemcached using vcpkg."""
    print(f"[INFO] Installing libmemcached via vcpkg (triplet: {triplet})...")
    
    vcpkg_exe = os.path.join(vcpkg_root, "vcpkg.exe")
    cmd = [vcpkg_exe, "install", f"libmemcached:{triplet}"]
    
    try:
        result = subprocess.run(cmd, check=False, capture_output=False, text=True)
        if result.returncode == 0:
            print("[INFO] Successfully installed libmemcached via vcpkg")
            # Return the vcpkg path for memcached
            packages_dir = os.path.join(vcpkg_root, "installed", triplet)
            if os.path.exists(packages_dir):
                return packages_dir
    except Exception as e:
        print(f"[WARNING] Failed to install libmemcached via vcpkg: {e}")
    
    return None


def try_install_libmemcached():
    """Attempt to install libmemcached using system package manager."""
    system = platform.system()
    
    print("\n[INFO] libmemcached not found. Attempting installation...")
    
    if system == "Linux":
        # Try apt-get (Debian/Ubuntu)
        if shutil.which("apt-get"):
            print("[INFO] Installing libmemcached-dev via apt-get...")
            result = subprocess.run(["sudo", "apt-get", "install", "-y", "libmemcached-dev"])
            if result.returncode == 0:
                return find_libmemcached()
        # Try yum (RHEL/CentOS)
        elif shutil.which("yum"):
            print("[INFO] Installing libmemcached-devel via yum...")
            result = subprocess.run(["sudo", "yum", "install", "-y", "libmemcached-devel"])
            if result.returncode == 0:
                return find_libmemcached()
        # Try dnf (Fedora)
        elif shutil.which("dnf"):
            print("[INFO] Installing libmemcached-devel via dnf...")
            result = subprocess.run(["sudo", "dnf", "install", "-y", "libmemcached-devel"])
            if result.returncode == 0:
                return find_libmemcached()
    
    elif system == "Darwin":  # macOS
        if shutil.which("brew"):
            print("[INFO] Installing libmemcached via Homebrew...")
            result = subprocess.run(["brew", "install", "libmemcached"])
            if result.returncode == 0:
                return find_libmemcached()
    
    elif system == "Windows":
        # Try vcpkg first
        vcpkg_root = find_vcpkg()
        if vcpkg_root:
            triplet = get_vcpkg_triplet()
            # Check if already installed
            memcached_path = find_libmemcached_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path
            
            # Try to install via vcpkg
            memcached_path = try_install_via_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path
        else:
            print("[INFO] vcpkg not found. To use vcpkg:")
            print("       1. Clone: git clone https://github.com/Microsoft/vcpkg.git")
            print("       2. Bootstrap: .\\vcpkg\\bootstrap-vcpkg.bat")
            print("       3. Set VCPKG_ROOT environment variable")
            print("       4. Or install at C:\\vcpkg (default location)")
        
        print("[WARNING] Please install libmemcached-awesome from:")
        print("          - vcpkg: Set VCPKG_ROOT and use --with-memcached flag")
        print("          - OR manually: https://github.com/awesomized/libmemcached/releases")
    
    return None


def get_libmemcached_path(require=False):
    """Get libmemcached path, optionally install if not found."""
    memcached_path = find_libmemcached()
    
    if memcached_path:
        return memcached_path
    
    if require:
        memcached_path = try_install_libmemcached()
        if memcached_path:
            return memcached_path
        
        print("\n[WARNING] libmemcached not found and could not be installed automatically.")
        print("          Memcached support will be disabled.")
        print("          To enable memcached support, install libmemcached and try again.")
        return None
    
    print("[INFO] libmemcached not found - memcached support will be optional/disabled")
    return None


def get_vcpkg_triplet():
    """Determine appropriate vcpkg triplet for current architecture.
    Uses the -static-md variant: static library, dynamic CRT (/MD).
    This is required for static libmemcached in DCC plugin builds."""
    machine = platform.machine().lower()
    
    if machine in ("amd64", "x86_64"):
        return "x64-windows-static-md"
    elif machine in ("x86", "i386", "i686"):
        return "x86-windows-static-md"
    elif machine.startswith("arm64"):
        return "arm64-windows-static-md"
    else:
        print(f"[WARNING] Unknown architecture: {machine}, defaulting to x64-windows-static-md")
        return "x64-windows-static-md"


def find_libmemcached_vcpkg(vcpkg_root, triplet):
    """Find libmemcached-awesome in vcpkg installation."""
    packages_dir = os.path.join(vcpkg_root, "installed", triplet)
    include_path = os.path.join(packages_dir, "include", "libmemcached-1.0")
    lib_path = os.path.join(packages_dir, "lib")
    
    if os.path.exists(include_path) and os.path.exists(lib_path):
        print(f"[INFO] Found libmemcached via vcpkg: {packages_dir}")
        return packages_dir
    
    return None


def try_install_via_vcpkg(vcpkg_root, triplet):
    """Install libmemcached using vcpkg."""
    print(f"[INFO] Installing libmemcached-awesome via vcpkg (triplet: {triplet})...")
    
    vcpkg_exe = os.path.join(vcpkg_root, "vcpkg.exe")
    cmd = [vcpkg_exe, "install", f"libmemcached-awesome:{triplet}"]
    
    try:
        result = subprocess.run(cmd, check=False, capture_output=False, text=True)
        if result.returncode == 0:
            print("[INFO] Successfully installed libmemcached via vcpkg")
            # Return the vcpkg path for memcached
            packages_dir = os.path.join(vcpkg_root, "installed", triplet)
            if os.path.exists(packages_dir):
                return packages_dir
    except Exception as e:
        print(f"[WARNING] Failed to install libmemcached via vcpkg: {e}")
    
    return None


def find_libmemcached():
    """Try to find libmemcached installation path."""
    system = platform.system()
    
    # Try pkg-config first (works on Linux and macOS)
    try:
        output = subprocess.check_output(
            ["pkg-config", "--variable=prefix", "libmemcached"],
            stderr=subprocess.DEVNULL,
            text=True
        ).strip()
        if output and os.path.exists(output):
            print(f"[INFO] Found libmemcached via pkg-config: {output}")
            return output
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    
    # Common installation paths to check
    search_paths = []
    
    if system == "Linux":
        search_paths = [
            "/usr",
            "/usr/local",
            "/opt/libmemcached",
            os.path.expanduser("~/.local"),
        ]
    elif system == "Darwin":  # macOS
        search_paths = [
            "/usr/local/opt/libmemcached",  # Homebrew
            "/usr/local",
            "/opt/homebrew/opt/libmemcached",  # Apple Silicon Homebrew
        ]
    elif system == "Windows":
        search_paths = [
            "C:\\Program Files\\libmemcached",
            "C:\\Program Files (x86)\\libmemcached",
        ]        
        # Also check vcpkg installations
        vcpkg_root = find_vcpkg()
        if vcpkg_root:
            triplet = get_vcpkg_triplet()
            memcached_path = find_libmemcached_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path    
    # Check standard paths
    for base_path in search_paths:
        include_path = os.path.join(base_path, "include", "libmemcached.h")
        lib_path = os.path.join(base_path, "lib")
        
        if os.path.exists(include_path) and os.path.exists(lib_path):
            print(f"[INFO] Found libmemcached at: {base_path}")
            return base_path
    
    return None


def find_vcpkg():
    """Find vcpkg installation on Windows."""
    if platform.system() != "Windows":
        return None
    
    # Check environment variable first
    vcpkg_root = os.environ.get("VCPKG_ROOT")
    if vcpkg_root and os.path.exists(os.path.join(vcpkg_root, "vcpkg.exe")):
        print(f"[INFO] Found vcpkg via VCPKG_ROOT: {vcpkg_root}")
        return vcpkg_root
    
    # Common vcpkg installation paths
    search_paths = [
        "C:\\vcpkg",
        os.path.expanduser("~/vcpkg"),
        os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "vcpkg"),
    ]
    
    for vcpkg_path in search_paths:
        vcpkg_exe = os.path.join(vcpkg_path, "vcpkg.exe")
        if os.path.exists(vcpkg_exe):
            print(f"[INFO] Found vcpkg at: {vcpkg_path}")
            return vcpkg_path
    
    return None





def try_install_via_vcpkg(vcpkg_root, triplet):
    """Install libmemcached using vcpkg."""
    print(f"[INFO] Installing libmemcached via vcpkg (triplet: {triplet})...")
    
    vcpkg_exe = os.path.join(vcpkg_root, "vcpkg.exe")
    cmd = [vcpkg_exe, "install", f"libmemcached:{triplet}"]
    
    try:
        result = subprocess.run(cmd, check=False, capture_output=False, text=True)
        if result.returncode == 0:
            print("[INFO] Successfully installed libmemcached via vcpkg")
            # Return the vcpkg path for memcached
            packages_dir = os.path.join(vcpkg_root, "installed", triplet)
            if os.path.exists(packages_dir):
                return packages_dir
    except Exception as e:
        print(f"[WARNING] Failed to install libmemcached via vcpkg: {e}")
    
    return None


# DUPLICATE REMOVED - Already defined earlier


# DUPLICATE REMOVED - Already defined earlier


def try_install_libmemcached():
    """Attempt to install libmemcached using system package manager."""
    system = platform.system()
    
    print("\n[INFO] libmemcached not found. Attempting installation...")
    
    if system == "Linux":
        # Try apt-get (Debian/Ubuntu)
        if shutil.which("apt-get"):
            print("[INFO] Installing libmemcached-dev via apt-get...")
            result = subprocess.run(["sudo", "apt-get", "install", "-y", "libmemcached-dev"])
            if result.returncode == 0:
                return find_libmemcached()
        # Try yum (RHEL/CentOS)
        elif shutil.which("yum"):
            print("[INFO] Installing libmemcached-devel via yum...")
            result = subprocess.run(["sudo", "yum", "install", "-y", "libmemcached-devel"])
            if result.returncode == 0:
                return find_libmemcached()
        # Try dnf (Fedora)
        elif shutil.which("dnf"):
            print("[INFO] Installing libmemcached-devel via dnf...")
            result = subprocess.run(["sudo", "dnf", "install", "-y", "libmemcached-devel"])
            if result.returncode == 0:
                return find_libmemcached()
    
    elif system == "Darwin":  # macOS
        if shutil.which("brew"):
            print("[INFO] Installing libmemcached via Homebrew...")
            result = subprocess.run(["brew", "install", "libmemcached"])
            if result.returncode == 0:
                return find_libmemcached()
    
    elif system == "Windows":
        # Try vcpkg first
        vcpkg_root = find_vcpkg()
        if vcpkg_root:
            triplet = get_vcpkg_triplet()
            # Check if already installed
            memcached_path = find_libmemcached_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path
            
            # Try to install via vcpkg
            memcached_path = try_install_via_vcpkg(vcpkg_root, triplet)
            if memcached_path:
                return memcached_path
        else:
            print("[INFO] vcpkg not found. To use vcpkg:")
            print("       1. Clone: git clone https://github.com/Microsoft/vcpkg.git")
            print("       2. Bootstrap: .\\vcpkg\\bootstrap-vcpkg.bat")
            print("       3. Set VCPKG_ROOT environment variable")
            print("       4. Or install at C:\\vcpkg (default location)")
        
        print("[WARNING] Please install libmemcached-awesome from:")
        print("          - vcpkg: Set VCPKG_ROOT and use --with-memcached flag")
        print("          - OR manually: https://github.com/awesomized/libmemcached/releases")
    
    return None


def get_libmemcached_path(require=False):
    """Get libmemcached path, optionally install if not found."""
    memcached_path = find_libmemcached()
    
    if memcached_path:
        return memcached_path
    
    if require:
        memcached_path = try_install_libmemcached()
        if memcached_path:
            return memcached_path
        
        print("\n[WARNING] libmemcached not found and could not be installed automatically.")
        print("          Memcached support will be disabled.")
        print("          To enable memcached support, install libmemcached and try again.")
        return None
    
    print("[INFO] libmemcached not found - memcached support will be optional/disabled")
    return None


def detect_houdini_env(root: str) -> list[str]:
    """Detect paths for Houdini installations.

    Args:
        root (str): Path to Houdini installation root.

    Returns:
        list[str]: List of CMake arguments for Houdini build.

    """
    houdini_cmake_path = os.path.join(root, "toolkit", "cmake")

    if platform.system().lower() == "windows":
        # Auto-detect Python version from Houdini by checking
        # which python3X directory exists
        python_exec = None
        # Check in order of preference
        for pyver in ["313", "311", "310", "39", "37"]:
            candidate = os.path.join(root, f"python{pyver}", "python.exe")
            if os.path.exists(candidate):
                python_exec = candidate
                break

        # Fallback if none found (shouldn't happen with valid Houdini)
        if not python_exec:
            python_exec = os.path.join(root, "python311", "python.exe")
            print(
                f"Could not detect Python version, using fallback: {python_exec}"
            )
    else:
        python_exec = os.path.join(root, "python", "bin", "python")

    return [
        "-DBUILD_TARGET=houdini",
        "-DUSE_OPENSSL3=ON",
        f'-DUSD_ROOT="{root}"',
        f'-DCMAKE_PREFIX_PATH="{houdini_cmake_path}"',
        f'-DPython_EXECUTABLE="{python_exec}"',
    ]


def detect_usd_env(root: str) -> list[str]:
    """Detect paths for standalone USD SDK installs.

    Args:
        root (str): Path to USD installation root.

    Returns:
        list[str]: List of CMake arguments for USD build.

    """
    return [
        "-DBUILD_TARGET=usd",
        f"-DUSD_ROOT={root}",
        "-DUSE_OPENSSL3=ON",
    ]


def detect_maya_env(root: str,
                    devkit_path: str | None = None,
                    usd_root: str | None = None) -> list[str]:
    """Detect paths for Maya installations.

    Args:
        root (str): Path to Maya installation root.
        devkit_path (str | None): Path to Maya USD devkit.
        usd_root (str | None): Path to USD installation root.

    Returns:
        list[str]: List of CMake arguments for Maya build.

    Raises:
        ValueError: If devkit_path or usd_root is not provided.

    """
    msg = None
    if devkit_path is None:
        msg = "Maya devkit path must be provided for Maya builds."

    if usd_root is None:
        msg = "Maya USD root path must be provided for Maya builds."

    if msg:
        print(msg)
        raise ValueError(msg)

    # 1. Executable
    maya_bin = os.path.join(root, "bin")
    mayapy = os.path.join(maya_bin, "mayapy")

    if os.path.exists(mayapy + ".exe"):
        python_exec = mayapy + ".exe"
    elif os.path.exists(mayapy):
        python_exec = mayapy
    else:
        python_exec = os.path.join(maya_bin, "python")

    # 2. Library & Include Paths — auto-detect Python version from mayapy
    try:
        pyver_out = subprocess.check_output(
            [
                python_exec,
                "-c",
                (
                    "import sys; print(f'{sys.version_info.major}"
                    "{sys.version_info.minor}')"
                )
            ],
            text=True
        ).strip()
    except Exception:  # noqa: BLE001
        pyver_out = "310"  # fallback for Maya 2024
    pyver_dotted = f"{pyver_out[0]}.{pyver_out[1:]}"

    if platform.system().lower() == "windows":
        python_lib = os.path.join(
            root, "lib", f"python{pyver_out}.lib")
        python_include = os.path.join(
            root, "include", f"Python{pyver_out}", "Python")
    else:
        python_lib = os.path.join(root, "lib", f"libpython{pyver_dotted}.so")
        python_include = os.path.join(
            root, "include", f"python{pyver_dotted}")

    return [
        "-DBUILD_TARGET=maya",
        "-DUSE_OPENSSL3=ON",
        f'-DMAYA_ROOT="{root}"',
        f'-DMAYA_USD_DEVKIT_PATH="{devkit_path}"',
        f'-DUSD_ROOT="{usd_root}"',

        # Only specify what CMake actually uses
        f'-DPython_EXECUTABLE="{python_exec}"',
        f'-DPython_INCLUDE_DIR="{python_include}"',
        f'-DPython_LIBRARIES="{python_lib}"',
    ]


def detect_dcc_env(dcc: str, root: str, **kwargs) -> list[str]:  # noqa: ANN003
    """Dispatch environment detection based on DCC name.

    Args:
        dcc (str): DCC name ("houdini", "maya", or "usd").
        root (str): Path to DCC or USD SDK root directory.
        **kwargs: Additional keyword arguments for specific DCC detection.

    Returns:
        list[str]: List of CMake arguments for the specified DCC build.

    Raises:
        ValueError: If the DCC name is unsupported.
    """
    if dcc == "houdini":
        return detect_houdini_env(root)
    if dcc == "maya":
        return detect_maya_env(root, **kwargs)
    if dcc == "usd":
        return detect_usd_env(root)
    msg = (
        f"Unsupported DCC: {dcc}. Supported values are:"
        "'houdini', 'maya', 'usd'."
    )
    raise ValueError(msg)


def main() -> None:
    """Main entry point for the build script."""
    parser = argparse.ArgumentParser(
        description="Build AYON USD Resolver",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--dcc",
        choices=["usd", "houdini", "maya"],
        default="usd",
        help="Target DCC or SDK to build against"
    )
    parser.add_argument(
        "--dcc-root",
        required=True,
        help="Path to DCC or USD SDK root directory"
    )
    parser.add_argument(
        "--maya-devkit",
        default=None,
        help="(Maya only) Path to Maya devkit"
    )
    parser.add_argument(
        "--maya-usd-root",
        default=None,
        help="(Maya only) Path to Maya USD root"
    )
    parser.add_argument(
        "--build-type",
        default="Release",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="CMake build type"
    )
    parser.add_argument(
        "--install-dir",
        default=None,
        help=("Custom installation directory path "
              "(defaults to install/{dcc}_{platform}_{build_type})")
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove build directory before building"
    )

    max_jobs = min(os.cpu_count() or 4, 4)
    parser.add_argument(
        "--jobs",
        type=int,
        default=max_jobs,
        help=f"Number of parallel build jobs (capped default: {max_jobs})"
    )
    parser.add_argument(
        "--with-memcached",
        action="store_true",
        help="Attempt to install libmemcached if not found"
    )

    args = parser.parse_args()

    project_root = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(
        project_root, f"build_{args.dcc}_{args.build_type.lower()}")

    if not args.install_dir:
        args.install_dir = os.path.join(
            project_root, "install",
            f"{args.dcc}_{platform.system().lower()}_{args.build_type.lower()}"
        )

    # Clean build
    if args.clean and os.path.exists(build_dir):
        print(f"Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(args.install_dir, exist_ok=True)

    # Detect environment
    dcc_args = detect_dcc_env(
        args.dcc,
        args.dcc_root,
        devkit_path=args.maya_devkit,
        usd_root=args.maya_usd_root
    )

    # Print configuration summary
    print("=== AYON USD Resolver Build Configuration ===")
    print(f"Platform: {platform.system()} {platform.release()}")
    print(f"DCC: {args.dcc}")
    print(f"Build type: {args.build_type}")
    print(f"Build dir: {build_dir}")
    print(f"Install dir: {args.install_dir}")
    print(f"Parallel jobs: {args.jobs}")

    # Handle libmemcached detection and installation
    memcached_path = get_libmemcached_path(require=args.with_memcached)
    if memcached_path:
        print(f"Memcached: ENABLED ({memcached_path})")
    else:
        print("Memcached: DISABLED (optional, use --with-memcached to install)")
    
    print("\n==================================================")
    
    cmake_args = [
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DCMAKE_INSTALL_PREFIX={args.install_dir}",
    ]
    
    # Add libmemcached to CMAKE_PREFIX_PATH if found
    if memcached_path:
        # Build the full CMAKE_PREFIX_PATH with libmemcached
        cmake_prefix_paths = [memcached_path]
        
        # Add existing paths from dcc_args if they have CMAKE_PREFIX_PATH
        for arg in dcc_args:
            if arg.startswith("-DCMAKE_PREFIX_PATH="):
                existing_paths = arg.replace("-DCMAKE_PREFIX_PATH=", "").strip('"')
                if existing_paths:
                    cmake_prefix_paths.append(existing_paths)
        
        # Combine all paths
        full_prefix_path = ";".join(cmake_prefix_paths)
        cmake_args.append(f'-DCMAKE_PREFIX_PATH="{full_prefix_path}"')
        
        # Remove old CMAKE_PREFIX_PATH from dcc_args to avoid duplication
        dcc_args = [arg for arg in dcc_args if not arg.startswith("-DCMAKE_PREFIX_PATH=")]
        
        # Request static linking for memcached
        cmake_args.append("-DAYON_MEMCACHED_STATIC_LINK=ON")
    
    cmake_args.extend(dcc_args)

    if platform.system() == "Windows":
        if shutil.which("ninja"):
            generator = "Ninja"
        else:
            generator = "Visual Studio 17 2022"
    else:
        generator = "Ninja" if shutil.which("ninja") else "Unix Makefiles"

    # Configure step
    run(f'cmake -S . -B "{build_dir}" -G "{generator}" {" ".join(cmake_args)}')

    # Build + Install
    run(
        f'cmake --build "{build_dir}" --target install '
        f"--config {args.build_type} -j {args.jobs}"
    )

    print("============================")
    print("Build finished successfully")
    print(f"  Artifacts installed to: {args.install_dir}")


if __name__ == "__main__":
    main()
