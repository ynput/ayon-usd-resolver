import subprocess
import argparse
import os
import shutil
import multiprocessing
import time
import platform

compile_plugin = os.environ.get("COMPILEPLUGIN")
script_dir = os.path.dirname(__file__)
prj_root_dir = os.path.dirname(script_dir)
build_dir = os.path.join(prj_root_dir, "build")
resolvers_dir = os.path.join(prj_root_dir,"Resolvers")
current_resolver_dir = os.path.join(resolvers_dir, str(compile_plugin))
max_cores = multiprocessing.cpu_count()

bin_package_path = os.path.join(resolvers_dir, "AyonUsdResolverBin")
temp_dir_for_zip = os.path.join(resolvers_dir, "temp")

zip_sub_path = f"{str(compile_plugin)}_{platform.system()}_{platform.machine()}"
zip_path = os.path.join(bin_package_path, zip_sub_path)

def generate_named_zip(resolver_source_path:str):
    zip_inner_foulder_name = os.path.basename(zip_path)
    zip_inner_foulder_path = os.path.join(temp_dir_for_zip, zip_inner_foulder_name)
    
    if os.path.exists(temp_dir_for_zip):
        shutil.rmtree(temp_dir_for_zip)
    os.makedirs(zip_inner_foulder_path)

    shutil.copytree(resolver_source_path, zip_inner_foulder_path, dirs_exist_ok=True)
    os.makedirs(bin_package_path, exist_ok=True)
    shutil.make_archive(zip_path,"zip", temp_dir_for_zip)

    shutil.rmtree(temp_dir_for_zip)
  
def cmake_get_vars():
    vars_dict = {}
    process = subprocess.run(["cmake", "-LAH", ".", "-B", "build"], stdout=subprocess.PIPE, text=True, universal_newlines=True)
    for i in process.stdout.split("//"):
        for x in i.split("\n"):
            if ":" in x:
                key_val = x.split(":")
                vars_dict[key_val[0]] = key_val[1]
    return vars_dict

def cmake(*args):
    command = ["cmake"]
    command.extend(*args)
    subprocess.run(command)

class _StoreDictKeyPair(argparse.Action):
     def __call__(self, parser, namespace, values, option_string=None):
         extra_env_var_dict = {}
         for kv in values.split(","):
             k,v = kv.split("=")
             extra_env_var_dict[k] = v
         setattr(namespace, self.dest, extra_env_var_dict)

def main():
    cmd_args = argparse.ArgumentParser(description="Ayon Usd Resolver main Build Script")
    cmd_args.add_argument("--DEV", help="Enable/Disable development build", type=int, default=0)
    cmd_args.add_argument("--JTRACE", help="Enable/Disable json tracing", type=int, default=0)
    cmd_args.add_argument("--Clean", action="store_true", help="delet build foulder for non cached build also adds --clean-first to cmake args")
    cmd_args.add_argument("--CompilePlugin", type=str, default=0, help="select a compile plugin, this can be scipped if you set COMPILEPLUGIN env variable ")
    cmd_args.add_argument("--extra_env_vars", dest="extra_env_var_dict", action=_StoreDictKeyPair, metavar="KEY1=VAL1,KEY2=VAL2...")
    cmd_args = cmd_args.parse_args()
    
    if cmd_args.extra_env_var_dict:
        for key in cmd_args.extra_env_var_dict:
            val = cmd_args.extra_env_var_dict[key]
            os.environ[key] = val
    
    if cmd_args.CompilePlugin:
        os.environ["COMPILEPLUGIN"] = cmd_args.CompilePlugin
        global compile_plugin
        global current_resolver_dir
        compile_plugin = os.environ.get("COMPILEPLUGIN")
        current_resolver_dir = os.path.join(resolvers_dir, compile_plugin)

    if not compile_plugin:
        raise RuntimeError("No Compile plugin selected")

    prior_compile_vars = cmake_get_vars()
    last_compile_plugin = str(prior_compile_vars.get("SelectedCompilePlugin")).split("=")[-1]
    if not compile_plugin == last_compile_plugin:
        print("Last Selected Compile plugin is not the same as the current, force clean", compile_plugin, last_compile_plugin)
        cmd_args.Clean = True

    if cmd_args.Clean:
        print("Running Clean Build")
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
        os.makedirs(build_dir, exist_ok=True)
        if os.path.exists(current_resolver_dir):
            shutil.rmtree(current_resolver_dir)


    construct_command = ['.', '-B', 'build',  f'-DDEV={cmd_args.DEV}', f'-DJTRACE={cmd_args.JTRACE}' ]
    cmake(construct_command)

    build_comand = ["--build" ,"build"]
    if cmd_args.DEV:
        build_comand.extend(["--config", "Debug"])
    else:
        build_comand.extend(["--config", "Release"])
    if cmd_args.Clean:
        build_comand.extend(["--clean-first"])
    build_comand.extend(["--parallel", str(max_cores)])
    cmake(build_comand)

    install_command = ["--install", "build"]
    cmake(install_command)


if __name__ == "__main__":
    os.chdir(prj_root_dir)
    start_time = time.time()
    main()
    generate_named_zip(current_resolver_dir)
    end_time = time.time()
    build_time = end_time - start_time
    print("build took", build_time, "seconds")

