import os
import subprocess
import platform
import threading
import sys
import shutil
import psutil
import time
from enum import Enum
from pprint import pprint
from typing import Union


class BuildTypes(Enum):
    Release = "Release"
    Debug = "Debug"
    RelWithDebInfo = "RelWithDebInfo"
    MinSizeRel = "MinSizeRel"

    def __str__(self):
        return self.value


from automator.AyonCiCd.Project import Project, Func, Stage, Cmd_StoreDictKeyPair
from automator.AyonCiCd import Cmake

AyonUsdResolverPRJ = Project("Ayon_Usd_Resolver")
AyonUsdResolverPRJ.add_pip_package("PyYAML")
AyonUsdResolverPRJ.add_pip_package("jsonschema")

AyonUsdResolverPRJ.add_cmd_arg("--GenerateArgs", action=Cmd_StoreDictKeyPair)
AyonUsdResolverPRJ.add_cmd_arg(
    "--Clean",
    action="store_true",
    help="Deletes the specified Build Folder for the targeted item during compilation to ensure the element builds from a clean state.",
)
AyonUsdResolverPRJ.add_cmd_arg(
    "--CleanAll",
    action="store_true",
    help="Clears all build and install directories and files prior to building to guarantee a pristine build.",
)
AyonUsdResolverPRJ.add_cmd_arg(
    "--BuildType", type=BuildTypes, choices=list(BuildTypes), default=BuildTypes.Release
)
AyonUsdResolverPRJ.add_cmd_arg(
    "--Target",
    type=str,
    default="all",
    help="Choose a solitary target for construction. Selection is made by entering the Target Name",
)
AyonUsdResolverPRJ.add_cmd_arg(
    "--Parallel",
    "-p",
    type=int,
    default=int(os.cpu_count() / 2),
    help="Limit compile targets processed concurrently with this option. Useful for systems with limited resources.",
)

AyonUsdResolverPRJ.add_cmd_arg(
    "--MinMem",
    type=int,
    default=20,
    help="Set the minimum available memory required to initiate a task.",
)

cmd_args = AyonUsdResolverPRJ.setup_prj()
import yaml
import jsonschema
from jsonschema import validate

parrallel_limit = threading.Semaphore(cmd_args.Parallel)
MEMORY_THRESHOLD = cmd_args.MinMem
build_dir = os.path.join(os.getcwd(), "build")
resolvers_dir = os.path.join(os.getcwd(), "Resolvers")

build_config_schema = {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "patternProperties": {
        "^.*$": {
            "type": "object",
            "properties": {"COMPILEPLUGIN": {"type": "string"}},
            "patternProperties": {"^((?!COMPILEPLUGIN).)*$": {"type": "string"}},
            "required": ["COMPILEPLUGIN"],
            "additionalProperties": False,
        }
    },
    "additionalProperties": False,
}


def remove_all_build_foulders():
    if os.path.exists(AyonUsdResolverPRJ._build_artefacts_out_path):
        shutil.rmtree(AyonUsdResolverPRJ._build_artefacts_out_path)
        os.makedirs(AyonUsdResolverPRJ._build_artefacts_out_path, exist_ok=True)

    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)

    if os.path.exists(resolvers_dir):
        shutil.rmtree(resolvers_dir)


def generate_named_zip(resolver_source_path: str, compile_plugin):
    bin_package_path = os.path.join(resolvers_dir, "AyonUsdResolverBin")
    zip_path = os.path.join(
        os.getcwd(),
        bin_package_path,
        f"{str(compile_plugin)}_{platform.system()}_{platform.machine()}",
    )

    temp_dir_for_zip = os.path.abspath(
        os.path.join(resolvers_dir, "temp", compile_plugin)
    )
    zip_inner_foulder_name = os.path.basename(zip_path)
    zip_inner_foulder_path = os.path.join(temp_dir_for_zip, zip_inner_foulder_name)

    if os.path.exists(temp_dir_for_zip):
        shutil.rmtree(temp_dir_for_zip)
    os.makedirs(zip_inner_foulder_path)

    shutil.copytree(resolver_source_path, zip_inner_foulder_path, dirs_exist_ok=True)
    os.makedirs(bin_package_path, exist_ok=True)
    shutil.make_archive(zip_path, "zip", temp_dir_for_zip)

    shutil.rmtree(temp_dir_for_zip)


def _get_os_path(src_path: str) -> str:
    if platform.system() == "Windows":
        path = os.path.abspath(src_path)
    else:
        path = os.path.expanduser(os.path.expandvars(src_path))
    return path


def _generate_env(change_keys: dict) -> os._Environ:
    test_env = os.environ.copy()

    for key, val in change_keys.items():
        path = _get_os_path(val)
        if os.path.exists(path):
            val = path
        test_env[key] = val
    return test_env


def cmake_compile(name, cmake_args, env_data, clean_build: bool, build_type):
    try:
        parrallel_limit.acquire()
        abs_build_path = os.path.abspath(f"build/{name}")
        if clean_build and os.path.exists(abs_build_path):
            shutil.rmtree(abs_build_path)

        build_env = _generate_env(env_data)
        Cmake.cmake_command(
            AyonUsdResolverPRJ,
            build_env,
            ".",
            "-B",
            f"build/{name}",
            "-DJTRACE=0",
            f"-DCMAKE_BUILD_TYPE={str(build_type)}",
            f"-DCMAKE_INSTALL_PREFIX=Resolvers/{name}",
        )
        Cmake.cmake_command(
            AyonUsdResolverPRJ,
            build_env,
            "--build",
            f"build/{name}",
            "--config",
            str(build_type),
            f"-j{os.cpu_count()}",
        )
        Cmake.cmake_command(
            AyonUsdResolverPRJ,
            build_env,
            "--install",
            f"build/{name}",
        )
    finally:
        parrallel_limit.release()


def validate_build_conf(build_targets: Union[str, dict]):
    build_config = build_targets

    if isinstance(build_targets, str):
        conf_path = _get_os_path(build_targets)
        if build_targets.endswith("yaml") and os.path.isfile(conf_path):
            with open(conf_path, "r") as file:
                build_config = yaml.safe_load(file)
    try:

        validate(instance=build_config, schema=build_config_schema)
    except jsonschema.exceptions.ValidationError as e:
        raise ValueError(f"Config not Valid {e}, data:{build_config}")

    return build_config


def _cmake_multi_build(
    build_targets: Union[str, dict],
    cmake_conf: dict,
    clean_build: bool = False,
    clean_all: bool = False,
    build_type=BuildTypes.Release,
):
    remove_all_build_foulders()
    build_config = validate_build_conf(build_targets)

    threads = []
    for name, conf in build_config.items():
        if not cmd_args.Target == name and not cmd_args.Target == "all":
            continue
        if not platform.system() in name:
            continue

        thread = threading.Thread(
            target=cmake_compile, args=(name, cmake_conf, conf, clean_build, build_type)
        )
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()


def _zip_resolvers(
    build_targets: Union[str, dict], cmake_conf: dict, clean_build: bool = False
):
    build_config = validate_build_conf(build_targets)

    for name, conf in build_config.items():
        if not cmd_args.Target == name and not cmd_args.Target == "all":
            continue
        if not platform.system() in name:
            continue
        generate_named_zip(os.path.abspath(f"Resolvers/{name}"), conf["COMPILEPLUGIN"])


build_stage = Stage("Build Resolvers")
build_stage.add_funcs(
    Func(
        "Cmake Build",
        _cmake_multi_build,
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "BuildConf"),
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "CMakeArgs"),
        cmd_args.Clean,
        cmd_args.CleanAll,
        cmd_args.BuildType,
    )
)
AyonUsdResolverPRJ.add_stage(build_stage)

build_release = Stage("Build Release")
build_release.add_funcs(
    Func(
        "Cmake Build",
        _cmake_multi_build,
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "BuildConf"),
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "CMakeArgs"),
        True,
        True,
        BuildTypes.Release,
    )
)
AyonUsdResolverPRJ.add_stage(build_release)

zip_stage = Stage("Zip Resolvers")
zip_stage.add_funcs(
    Func(
        "zip",
        _zip_resolvers,
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "BuildConf"),
        Func("Get BuildConf", AyonUsdResolverPRJ.getVar, "CMakeArgs"),
        cmd_args.Clean,
    )
)
AyonUsdResolverPRJ.add_stage(zip_stage)

clean_stage = Stage("Clean")
clean_stage.add_funcs(Func("Clean", remove_all_build_foulders))
AyonUsdResolverPRJ.add_stage(clean_stage)


def _zip_bin_package():
    resolvers_dir = os.path.join(os.getcwd(), "Resolvers")
    bin_package_dir = os.path.join(os.getcwd(), "Resolvers", "AyonUsdResolverBin")
    current_dir = os.getcwd()
    os.chdir(resolvers_dir)
    shutil.make_archive(
        base_name="AyonUsdResolverBin",
        format="zip",
        base_dir=bin_package_dir,
        root_dir=resolvers_dir,
    )
    temp_dir = os.path.join(resolvers_dir, "temp")
    if os.path.exists(temp_dir):
        shutil.rmtree(temp_dir)
    os.chdir(current_dir)


zip_package_stage = Stage("Zip_Bin_Package")
zip_package_stage.add_funcs(Func("zip", _zip_bin_package))
zip_package_stage.addArtefactFoulder(
    os.path.join(os.getcwd(), "Resolvers", "AyonUsdResolverBin.zip")
)
AyonUsdResolverPRJ.add_stage(zip_package_stage)

AyonUsdResolverPRJ.creat_stage_group(
    "Build and Zip",
    build_stage,
    zip_stage,
)

AyonUsdResolverPRJ.creat_stage_group(
    "Generate Release",
    build_release,
    zip_stage,
    zip_package_stage,
)

with AyonUsdResolverPRJ as prj:
    prj.make_project_cli_available()
