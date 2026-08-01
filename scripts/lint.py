import glob
import sys

cmake_files  = glob.glob("src/**/CMakeLists.txt", recursive=True)
source_files = glob.glob("src/**/*.cpp", recursive=True)
header_files = glob.glob("src/**/*.h", recursive=True)

# For every call to target_sources in cmake_file, return the list of arguments
# to target_sources.
# Example: []
def extract_target_sources(cmake_file):
    return_value = []
    with open(cmake_file, "r") as f:
        remaining_contents = f.read()
    while "target_sources" in remaining_contents:
        pos = remaining_contents.find("target_sources")
        paren_start = remaining_contents.find("(", pos)
        paren_end = remaining_contents.find(")", pos)
        paren_contents = remaining_contents[paren_start + 1:paren_end]
        paren_elements = paren_contents.split()
        return_value += paren_elements
        remaining_contents = remaining_contents[pos + 1:]
    return return_value

# Check for cases where a C++ file is in the source tree, but not mentioned in
# any CMakeLists.txt file.
def lint_files_not_in_cmakelists():
    all_files = sorted(source_files + header_files)
    all_files = list(map(lambda x: x.split("/")[-1], all_files))
    remaining_files = set(all_files)
    for cmake_file in cmake_files:
        cmake_sources = extract_target_sources(cmake_file)
        for source in cmake_sources:
            file_name = source.split("/")[-1]
            if file_name in remaining_files:
                remaining_files.remove(file_name)
    for file in list(remaining_files):
        print(f"{file}: warning: file is not added in CMakeLists.txt (files-not-in-cmakelists)")
    return len(remaining_files)

num_warnings = 0
num_warnings += lint_files_not_in_cmakelists()

if num_warnings != 0:
    print(f"\n{num_warnings} warnings generated.")
    sys.exit(1)
else:
    print("No problems found")