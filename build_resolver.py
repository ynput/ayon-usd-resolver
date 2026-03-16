"""
Simple build helper for AYON USD Resolver.
Automatically detects USD/DCC environment and runs CMake build.
Adds optional zipping of the install directory.
"""

import argparse
import os
import sys
import subprocess
import platform
import shutil
import tempfile


def run(cmd, cwd=None, env=None):
    """Run a shell command with logging."""
    print(f"\n>>> {cmd}")
    result = subprocess.run(cmd, shell=True, cwd=cwd, env=env)
    if result.returncode != 0:
        sys.exit(result.returncode)


def detect_houdini_env(root):
    """Detect paths for Houdini installations."""
    houdini_cmake_path = os.path.join(root, "toolkit", "cmake")

    if platform.system() == "Windows":
        python_exec = os.path.join(root, "python311", "python.exe")
    else:
        python_exec = os.path.join(root, "python", "bin", "python")

    return [
        "-DBUILD_TARGET=houdini",
        "-DUSE_OPENSSL3=ON",
        f"-DUSD_ROOT=\"{root}\"",
        f"-DCMAKE_PREFIX_PATH=\"{houdini_cmake_path}\"",
        f"-DPython_EXECUTABLE=\"{python_exec}\"",
    ]


def detect_usd_env(root):
    """Detect paths for standalone USD SDK installs."""
    python_exec = shutil.which("python3") or sys.executable
    return [
        "-DBUILD_TARGET=usd",
        f"-DUSD_ROOT={root}",
        f"-DPython_EXECUTABLE={python_exec}",
    ]


def detect_maya_env(root, devkit_path=None, usd_root=None):
    """Detect paths for Maya installations."""
    if devkit_path is None:
        raise ValueError("Maya devkit path must be provided for Maya builds.")
    if usd_root is None:
        raise ValueError("Maya USD root path must be provided for Maya builds.")

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
            [python_exec, "-c", "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')"],
            text=True
        ).strip()
    except Exception:
        pyver_out = "310"  # fallback for Maya 2024
    pyver_dotted = f"{pyver_out[0]}.{pyver_out[1:]}"

    if platform.system() == "Windows":
        python_lib = os.path.join(root, "lib", f"python{pyver_out}.lib")
        python_include = os.path.join(root, "include", f"Python{pyver_out}", "Python")
    else:
        python_lib = os.path.join(root, "lib", f"libpython{pyver_dotted}.so")
        python_include = os.path.join(root, "include", f"python{pyver_dotted}")

    return [
        "-DBUILD_TARGET=maya",
        "-DUSE_OPENSSL3=ON",
        f"-DMAYA_ROOT=\"{root}\"",
        f"-DMAYA_USD_DEVKIT_PATH=\"{devkit_path}\"",
        f"-DUSD_ROOT=\"{usd_root}\"",

        # Only specify what CMake actually uses
        f"-DPython_EXECUTABLE=\"{python_exec}\"",
        f"-DPython_INCLUDE_DIR=\"{python_include}\"",
        f"-DPython_LIBRARIES=\"{python_lib}\"",
    ]


def detect_dcc_env(dcc, root, **kwargs):
    """Dispatch environment detection based on DCC name."""
    if dcc == "houdini":
        return detect_houdini_env(root)
    elif dcc == "maya":
        return detect_maya_env(root, **kwargs)
    elif dcc == "usd":
        return detect_usd_env(root)
    else:
        raise ValueError(f"Unsupported DCC: {dcc}")


def main():
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
        help="Custom installation directory path (defaults to install/{dcc}_{platform}_{build_type})"
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

    args = parser.parse_args()

    project_root = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(project_root, f"build_{args.dcc}_{args.build_type.lower()}")

    if not args.install_dir:
        args.install_dir = os.path.join(
            project_root, "install", f"{args.dcc}_{platform.system().lower()}_{args.build_type.lower()}"
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
    print("\n=== AYON USD Resolver Build Configuration ===")
    print(f"Platform: {platform.system()} {platform.release()}")
    print(f"DCC: {args.dcc}")
    print(f"Build type: {args.build_type}")
    print(f"Build dir: {build_dir}")
    print(f"Install dir: {args.install_dir}")
    print(f"Parallel jobs: {args.jobs}")
    print("\n==================================================")

    cmake_args = [
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DCMAKE_INSTALL_PREFIX={args.install_dir}",
    ]
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
    run(f'cmake --build "{build_dir}" --target install --config {args.build_type} -j {args.jobs}')

    print("\n============================")
    print(f"\nBuild finished successfully!\nArtifacts installed to: {args.install_dir}\n")


if __name__ == "__main__":
    main()
