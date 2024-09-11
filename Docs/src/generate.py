import os
import re
import subprocess
import argparse
import os

# VERSIONED_OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "docs")
# VERSION_FILE_MD = os.path.join(os.path.dirname(__file__), "md", "Versions.md")
DOXY_FILE = os.path.join(os.path.dirname(__file__), "Doxyfile")


def main():

    cmd_args = argparse.ArgumentParser(description="Ayon Cpp Dev Tools")
    cmd_args.add_argument(
        "--Version",
        help="Set the current Build Version",
        type=str,
        required=True,
    )
    cmd_args.add_argument(
        "--DontBuild",
        help="Enable only chaning the doxygen file",
        type=bool,
        default=False,
    )
    cmd_args = cmd_args.parse_args()

    os.chdir(os.path.dirname(__file__))

    output_dir = f"../docs/{cmd_args.Version}"
    os.makedirs(os.path.abspath(output_dir), exist_ok=True)
    if not os.path.exists(os.path.abspath(output_dir)):
        raise RuntimeError("the output foulder is missing")

    with open(DOXY_FILE, "r") as file:
        doxyfile_content = file.read()

    doxyfile_content = re.sub(
        r"^OUTPUT_DIRECTORY\s*=.*",
        f"OUTPUT_DIRECTORY = {output_dir}",
        doxyfile_content,
        flags=re.MULTILINE,
    )
    doxyfile_content = re.sub(
        r"^PROJECT_NUMBER\s*=.*",
        f"PROJECT_NUMBER = {cmd_args.Version}",
        doxyfile_content,
        flags=re.MULTILINE,
    )

    with open(DOXY_FILE, "w") as file:
        file.write(doxyfile_content)

    if not cmd_args.DontBuild:
        subprocess.run(
            ["doxygen", DOXY_FILE], check=True, cwd=os.path.dirname(__file__)
        )


if __name__ == "__main__":
    main()
