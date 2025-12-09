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


def run(cmd, cwd=None, env=None):
    """Run a shell command with logging."""
    print(f"\n>>> {cmd}")
    result = subprocess.run(cmd, shell=True, cwd=cwd, env=env)
    if result.returncode != 0:
        sys.exit(result.returncode)


def detect_houdini_env(root):
    """Detect paths for Houdini installations."""
    usd_root = root
    houdini_cmake_path = f"{usd_root}/toolkit/cmake"
    python_exec = os.path.join(root, "python", "bin", "python")
    return [
        "-DBUILD_TARGET=houdini",
        f"-DUSD_ROOT={usd_root}",
        f"-DCMAKE_PREFIX_PATH={houdini_cmake_path}",
        f"-DPYTHON_EXECUTABLE={python_exec}",
    ]

def detect_maya_env(root, devkit_path=None, usd_root=None):
    """Detect paths for Maya installations."""
    if devkit_path is None:
        raise ValueError("Maya devkit path must be provided for Maya builds.")
    if usd_root is None:
        raise ValueError("Maya USD root path must be provided for Maya builds.")
    python_exec = os.path.join(root, "bin", "python")
    return [
        "-DBUILD_TARGET=maya",
        f"-DMAYA_ROOT={root}",
        f"-DMAYA_USD_DEVKIT_PATH={devkit_path}",
        f"-DUSD_ROOT={usd_root}",
        f"-DPYTHON_EXECUTABLE={python_exec}",
    ]


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
    install_dir = os.path.join(
        project_root, "install", f"{args.dcc}_{platform.system().lower()}_{args.build_type.lower()}"
    )

    # Clean build
    if args.clean and os.path.exists(build_dir):
        print(f"Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(install_dir, exist_ok=True)

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
    print(f"Install dir: {install_dir}")
    print(f"Parallel jobs: {args.jobs}")
    print(f"Zip after build: {args.zip}")
    print("\n==================================================")

    cmake_args = [
        # f"-DAYON_CPPTOOLS_BUILD_LOGGER=OFF"
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DCMAKE_INSTALL_PREFIX={install_dir}",
    ]
    cmake_args.extend(dcc_args)

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
                f"ayon-usd-resolver_{args.dcc}_{sys_name}"
            )

        archive_path = shutil.make_archive(
            base_name=base_name,
            format="zip",
            root_dir=os.path.dirname(install_dir),
            base_dir=os.path.basename(install_dir)
        )
        print(f"\nCreated archive: {archive_path}")

    print("\n============================")
    print(f"\nBuild finished successfully!\nArtifacts installed to: {install_dir}\n")


if __name__ == "__main__":
    main()
