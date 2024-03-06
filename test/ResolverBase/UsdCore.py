import os
from pxr import Ar, Usd
#from usdAssetResolver import AyonUsdResolver


def setup_custom_usd_resolver():
    # Path to your resolver directory
    script_dir = "/path/to/your/resolver/directory"

    # Set environment variables
    os.environ["USD_ASSET_RESOLVER"] = script_dir
    os.environ["TF_DEBUG"] = "AYONUSDRESOLVER_RESOLVER_CONTEXT"
    os.environ["LD_LIBRARY_PATH"] = f"{script_dir}/ayonUsdResolver/lib:{os.environ.get('LD_LIBRARY_PATH', '')}"
    os.environ["PXR_PLUGINPATH_NAME"] = f"{script_dir}/ayonUsdResolver/resources:{os.environ.get('PXR_PLUGINPATH_NAME', '')}"
    os.environ["PYTHONPATH"] = f"{script_dir}/ayonUsdResolver/lib/python:{os.environ.get('PYTHONPATH', '')}"


def main():
    # Setup custom USD resolver
    #setup_custom_usd_resolver()

    # Now you can proceed to work with USD using the custom resolver
    # For example, opening a USD file
    usd_file_path = "/home/workh/Ynput/dev/ayon-usd-resolver/test/ResolverBase/TestUsd.usda"
    stage = Usd.Stage.Open(usd_file_path)
    
    stage_string = stage.ExportToString()
    print(stage_string)

    
if __name__ == "__main__":
    main()
