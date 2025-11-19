#!/usr/bin/env python3
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


# ------------------------------------------------------------
# Utility helpers
# ------------------------------------------------------------

def run(cmd, cwd=None, env=None):
    """Run a shell command with logging."""
    print(f"\n>>> {cmd}")
    result = subprocess.run(cmd, shell=True, cwd=cwd, env=env)
    if result.returncode != 0:
        sys.exit(result.returncode)


def detect_houdini_env(root):
    """Detect paths for Houdini installations (19-21)."""
    usd_root = root  # Houdini 21 has USD directly under /opt/hfsXX
    python_exec = os.path.join(root, "python", "bin", "python3.11")
    return {
        "DCC": "houdini",
        "USD_ROOT": usd_root,
        "PYTHON_EXECUTABLE": python_exec,
        "PYTHON_VERSION": "3.11",
    }


def detect_maya_env(root):
    """Detect paths for Maya installations."""
    usd_root = os.path.join(root, "lib", "usd")
    python_exec = os.path.join(root, "bin", "mayapy")
    return {
        "DCC": "maya",
        "USD_ROOT": usd_root,
        "PYTHON_EXECUTABLE": python_exec,
        "PYTHON_VERSION": "3.10",
    }


def detect_usd_env(root):
    """Detect paths for standalone USD SDK installs."""
    python_exec = shutil.which("python3") or sys.executable
    pyver = f"{sys.version_info.major}.{sys.version_info.minor}"
    return {
        "DCC": "usd",
        "USD_ROOT": root,
        "PYTHON_EXECUTABLE": python_exec,
        "PYTHON_VERSION": pyver,
    }


def detect_dcc_env(dcc, root):
    """Dispatch environment detection based on DCC name."""
    if dcc == "houdini":
        return detect_houdini_env(root)
    elif dcc == "maya":
        return detect_maya_env(root)
    elif dcc == "usd":
        return detect_usd_env(root)
    else:
        raise ValueError(f"Unsupported DCC: {dcc}")


# ------------------------------------------------------------
# Main build logic
# ------------------------------------------------------------

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
        "--python",
        default=None,
        help="Python version override (e.g., 3.11)"
    )
    parser.add_argument(
        "--build-type",
        default="Release",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="CMake build type"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove build directory before building"
    )
    parser.add_argument(
        "--zip",
        action="store_true",
        help="Create a zip archive of the install directory after building"
    )
    parser.add_argument(
        "--zip-name",
        default=None,
        help="Output zip path (.zip). If omitted, ./dist/<auto-name>.zip is used"
    )

    # Cap parallelism at 4 for safety
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
    install_dir = os.path.join(project_root, "install")

    # Clean build
    if args.clean and os.path.exists(build_dir):
        print(f"Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(install_dir, exist_ok=True)

    # Detect environment
    env_info = detect_dcc_env(args.dcc, args.dcc_root)
    if args.python:
        env_info["PYTHON_VERSION"] = args.python

    # Print configuration summary
    print("\n=== AYON USD Resolver Build Configuration ===")
    print(f"Platform: {platform.system()} {platform.release()}")
    print(f"DCC: {env_info['DCC']}")
    print(f"USD_ROOT: {env_info['USD_ROOT']}")
    print(f"PYTHON_EXECUTABLE: {env_info['PYTHON_EXECUTABLE']}")
    print(f"PYTHON_VERSION: {env_info['PYTHON_VERSION']}")
    print(f"Build type: {args.build_type}")
    print(f"Build dir: {build_dir}")
    print(f"Install dir: {install_dir}")
    print(f"Parallel jobs: {args.jobs}")
    print(f"Zip after build: {args.zip}")

    # Build command
    # cmake_args = [
    #     f"-DCMAKE_BUILD_TYPE={args.build_type}",
    #     f"-DUSD_ROOT={env_info['USD_ROOT']}",
    #     f"-DPYTHON_EXECUTABLE={env_info['PYTHON_EXECUTABLE']}",
    #     f"-DPYTHON_VERSION={env_info['PYTHON_VERSION']}",
    #     f"-DCMAKE_INSTALL_PREFIX={install_dir}",
    # ]
    houdini_cmake_path = f"{env_info['USD_ROOT']}/toolkit/cmake"

    cmake_args = [
        "-DBUILD_TARGET=Houdini", # Ensure this is set for Houdini builds
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DUSD_ROOT={env_info['USD_ROOT']}",
        f"-DPYTHON_EXECUTABLE={env_info['PYTHON_EXECUTABLE']}",
        f"-DPYTHON_VERSION={env_info['PYTHON_VERSION']}",
        f"-DCMAKE_INSTALL_PREFIX={install_dir}",
        
        # --- ADD THIS LINE ---
        f"-DCMAKE_PREFIX_PATH={houdini_cmake_path}" 
    ]

    generator = "Ninja" if shutil.which("ninja") else "Unix Makefiles"

    # Configure step
    run(f'cmake -S . -B "{build_dir}" -G "{generator}" {" ".join(cmake_args)}')

    # Build + Install
    run(f'cmake --build "{build_dir}" --target install -j {args.jobs}')

    # Optional zip step
    if args.zip:
        # Decide base name (without .zip) and ensure destination dir exists
        if args.zip_name:
            base_name = args.zip_name[:-4] if args.zip_name.lower().endswith(".zip") else args.zip_name
            base_name = os.path.abspath(base_name)
            out_dir = os.path.dirname(base_name)
            if out_dir and not os.path.exists(out_dir):
                os.makedirs(out_dir, exist_ok=True)
        else:
            dist_dir = os.path.join(project_root, "dist")
            os.makedirs(dist_dir, exist_ok=True)
            sys_name = platform.system().lower()
            base_name = os.path.join(
                dist_dir,
                f"ayon-usd-resolver_{args.dcc}_{sys_name}_py{env_info['PYTHON_VERSION']}"
            )

        archive_path = shutil.make_archive(
            base_name=base_name,
            format="zip",
            root_dir=os.path.dirname(install_dir),
            base_dir=os.path.basename(install_dir)
        )
        print(f"\n📦 Created archive: {archive_path}")

    print(f"\n✅ Build finished successfully!\nArtifacts installed to: {install_dir}\n")


if __name__ == "__main__":
    main()