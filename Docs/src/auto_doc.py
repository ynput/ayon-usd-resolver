import os
import time
import subprocess
import threading

script_dir = os.path.dirname(__file__)
docs_dir = os.path.join(os.path.dirname(script_dir), "docs")
os.chdir(script_dir)


def get_file_mod_times(path):
    return {f: os.path.getmtime(os.path.join(path, f)) for f in os.listdir(path)}


def start_server():
    return subprocess.run(
        ["python3 -m http.server 8009"],
        shell=True,
        cwd=os.path.join(os.path.dirname(script_dir), "docs", "0.1.0", "html"),
    )


if __name__ == "__main__":
    path = "."
    
    if not os.path.exists(docs_dir):
        os.mkdir(docs_dir)
    print(os.listdir(docs_dir))
    subprocess.run(
        ["doxygen", "Doxyfile"],
    )

    prev_mod_times = get_file_mod_times(path)
    server = threading.Thread(target=start_server)
    server.start()

    while True:
        time.sleep(1)
        curr_mod_times = get_file_mod_times(path)

        changed_files = [
            f
            for f in curr_mod_times
            if f not in prev_mod_times or curr_mod_times[f] != prev_mod_times[f]
        ]

        if changed_files:
            print(os.listdir())
            subprocess.run(
                ["doxygen", "Doxyfile"],
            )

            prev_mod_times = curr_mod_times
