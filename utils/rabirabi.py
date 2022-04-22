import argparse
import pathlib
import re

# Meant to be DAG
class Dep_Tree:
    def __init__(self, path):
        self.path    = path
        self.depends = []
        self.lines   = []

    @property
    def name(self):
        return self.path.name

    def __str__(self):
        return "<Dep_Tree: {}: {}>".format(self.name, ",".join([ str(i) for i in self.depends ]))

    def __repr__(self):
        return "<Dep_Tree: {}: {}>".format(self.name, ",".join([ str(i) for i in self.depends ]))

def recursively_create_dependency_tree(files):
    expand_include_pattern = re.compile("^\#include \"(?P<name>.*\.h)\"")
    define_pattern = re.compile("^\#define (?P<name>)$")
    possible_guard_pattern = re.compile("^\#ifndef (?P<name>)$")

    expanding = set()
    total_file_line = []

    def expand(f):
        if f.name in expanding:
            return

        expanding.add(f.name)
        total_file_line.append("//@@ Rabirabi! merged: {}".format(f.name))
        total_file_line.append("#ifndef RABIRABI_{}_INCLUDED".format(f.name.split('.')[0]))
        total_file_line.append("#define RABIRABI_{}_INCLUDED".format(f.name.split('.')[0]))

        with open(f, "r") as file:
            line = file.readlines()
            defined_ident = set()

            for i in line:
                include_match = expand_include_pattern.match(i)
                if include_match:
                    path = f.parent / include_match.group("name")
                    expand(path)
                    continue

    for f in files:
        expand(f)

    print(total_file_line)


def do_join_operation(headers, sources, output, is_single = False, impl_name = ""):
    current_dir  = pathlib.Path('.')
    header_files = current_dir.glob(headers)
    source_files = current_dir.glob(sources)

    if is_single:
        if not output.endswith(".h"):
            if output.endswith("/"): # @Robustness
                output += "output"
            output += ".h"
    else:
        assert 0

    tree = recursively_create_dependency_tree(header_files)

def main():
    parser = argparse.ArgumentParser()
    subparser = parser.add_subparsers(dest="oper", help="operation help", required=True)
 
    p_join = subparser.add_parser("join", help="Join operation -- joins multiple .h and .c files into one .h / .c file respectively.")

    p_join.add_argument("header_glob",     help="e.g.: src/*.h -- specifies the header file that you intend to include in amalgamated header.")
    p_join.add_argument("source_glob",     help="e.g.: src/*.c -- specifies the source file that you intend to include in amalgamated source.")

    p_join.add_argument("-o", "--output",  help="specify filename/path for output. if you specify a path that ends with / (slash), it will interpret that as a directory and produce file inside of it.", type=str, required=True)

    p_join.add_argument("-s", "--single", help="make it single header instead of one .h and one.c.", action="store_true")
    p_join.add_argument("--impl-name",    help="when used with --single, you can specify #define directive for creating implementation of .C part.", type=str)

    result = parser.parse_args()

    if result.oper == "join":
        do_join_operation(result.header_glob, result.source_glob, result.output, result.single, result.impl_name)


if __name__ == "__main__":
    main()
