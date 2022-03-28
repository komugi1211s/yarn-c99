import pathlib
import subprocess
import os

TEST_FILE_FOLDER = "./tests/yarn-files/"
TEST_C_FOLDER    = "./tests/yarn-c"
filepath = []

def create_tests():
    print("[TESTER] Making test files.")
    files = pathlib.Path(TEST_FILE_FOLDER)

    ysc_path = "dist/ysc.exe" if os.name == "nt" else "dist/ysc"

    if not pathlib.Path(TEST_C_FOLDER).exists():
        pathlib.Path(TEST_C_FOLDER).mkdir()
    if not pathlib.Path(ysc_path).exists():
        print("[TESTER] Cannot create tests: ysc does not exist inside distribution folder")
        return

    for file in files.glob("./*.yarn"):
        output_name = file.name.split(".")[0]
        output_dir  = "%s/%s" % (TEST_C_FOLDER, output_name)
        actual_file_path = "%s/%s" % (TEST_FILE_FOLDER, file.name)
        path = pathlib.Path(output_dir)
        if path.exists():
            print("[TESTER]  folder %s already exists. skipping now." % path.name)
            filepath.append((output_dir, output_name))
            continue
        else:
            path.mkdir()

        output = subprocess.run([ysc_path, "compile", "-o", output_dir, "-n", output_name + ".yarnc", "-t", output_name + ".csv", actual_file_path], stdout=subprocess.DEVNULL)
        if output.returncode < 0:
            print("[TESTER]   FAILED: folder creation for %s. deleting folder now." % output_name)
            path.rmdir()

        else:
            filepath.append((output_dir, output_name))
            print("  Folder made for %s" % output_name)

def actually_do_tests():
    print("[TESTER] Actually trying to perform tests.")
    executable_path = "dist/main.exe" if os.name == "nt" else "dist/compiled"

    for (dirname, filename) in filepath:
        print("[TESTER] Trying to do test for file: %s" % (filename))
        yarnc_path = "%s/%s.yarnc" % (dirname, filename)
        csv_path = "%s/%s.csv" % (dirname, filename)
        output = subprocess.run([executable_path, yarnc_path, csv_path])


print("[TESTER] Commencing tests...")
create_tests()
actually_do_tests()
