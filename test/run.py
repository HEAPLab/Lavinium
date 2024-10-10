#!/usr/bin/env python

from pathlib import Path
import argparse
from typing import List
from modules.subprocess_helper import run
from modules.pretty_print import bold, color_print, green, red
import os
import modules.debug as debug


C_DIR = Path("./C")
LAVINIUM_PATH = os.getenv('LAVINIUM_PATH') 
SYSROOT = os.getenv('SYSROOT')
LAVINIUMPASSES = "../../passes"
CLANG =  LAVINIUM_PATH + "/clang" if LAVINIUM_PATH is not None else None
OPT =  LAVINIUM_PATH + "/opt" if LAVINIUM_PATH is not None else None
COMMON_FLAG = f"--target=arm-none-eabi --sysroot={SYSROOT} -march=armv4t -mfloat-abi=hard" if SYSROOT is not None else None
PASSES = "-passes=\"function(mem2reg,loop-simplify),loop-annota\""
TA_MUARCH = "--ta-muarch-type=fixedlatency" 
TA_MEMORY = "--ta-memory-type=none"
FIRST_COMPILATION_FLAG = f"-S -emit-llvm -Xclang -disable-O0-optnone {COMMON_FLAG} " 
SECOND_COMPILATION_FLAG = f"{COMMON_FLAG} -mllvm {TA_MEMORY} -mllvm {TA_MUARCH} -mllvm -lavinium-enable -mllvm -lavinium-file={LAVINIUMPASSES}" 



def check_env():
    if LAVINIUM_PATH is None:
        color_print(b"%b is not set\nPlease set it to the correct path" %
                    bold(red('LAVINIUM_PATH')))
        exit(1)
    if SYSROOT is None:
        color_print(b"%b is not set\nPlease set it to the correct path" %
                    bold(red('SYSROOT')))
        exit(1)
        
def write_to_file(name, content):
    with open(name, "wb") as f:
        f.write(content)

        
def compile(benchmarks : List[Path]):
    for benchmark in benchmarks:
        # Only one C file in the entry directory of the benchmark 

        directory = benchmark.as_posix()
        name = [x for x in benchmark.glob("*.c")][0].name
        stem = [x for x in benchmark.glob("*.c")][0].stem
        color_print(b"Processing:\t%b" %  bold(stem))
        
        code, out, err = run(f"cd {directory}; {CLANG} {name} -o {stem}-base.ll {FIRST_COMPILATION_FLAG} ")
        if code == 0:
            color_print(b"\tCompilation %b " % green("OK!"))
        else: 
            color_print(b"\tCompilation %b " % red("ERR"))
            write_to_file(directory + "/compilation_error",err)

        code,out,err = run(f"cd {directory}; {OPT} {PASSES} {stem}-base.ll -o /dev/null -S {TA_MUARCH} {TA_MEMORY}", True)
        if code == 0:
            color_print(b"\tPre  Annota %b " % green("OK!"))
        else: 
            color_print(b"\tPre  Annota %b " % red("ERR"))
            write_to_file(directory + "/pre_annota_error",err)
        
        code,out,err = run(f"cd {directory}; {OPT} {PASSES} {stem}-base.ll -o {stem}-post-annota.ll -S {TA_MUARCH} {TA_MEMORY}", True)
        if code == 0:
            color_print(b"\tPost Annota %b " % green("OK!"))
        else: 
            color_print(b"\tPost Annota %b " % red("ERR"))
            write_to_file(directory + "/post_annota_error",err)
        
        code,out,err = run(f"cd {directory}; {CLANG} {stem}-post-annota.ll -o {stem}-lavinium.ll {SECOND_COMPILATION_FLAG} ", True)
        if code == 0:
            color_print(b"\tLavinium    %b " % green("OK!"))
            write_to_file(directory + "/lavinium_res",out)
        else: 
            color_print(b"\tLavinium    %b " % red("ERR"))
            write_to_file(directory + "/lavinium_error",err)

def clean(benchmarks : List[Path]):
    for benchmark in benchmarks:
        files = [x for x in benchmark.glob("*") if not x.name.endswith((".c", ".h"))]
        for file in files:
            file.unlink()


if __name__ == '__main__':
    check_env()
    parser = argparse.ArgumentParser(description='Lavinium runner')
    parser.add_argument('-only', metavar='name[,name]*', type=str, help='List of comma separated names\'s names')
    parser.add_argument('-clean', metavar='bool', type=bool, default=False, nargs='?', help='Clean Benchmarks' , const=True)
    parser.add_argument('-compile', metavar='bool', type=bool, default=False, nargs='?', help='Compile Benchmarks' , const=True)
    parser.add_argument('-debug', metavar='bool', type=bool, default=False, nargs='?', help='Enable Debug' , const=True)
    args = parser.parse_args()

    if args.debug:
        debug.DEBUG = args.debug


    if args.only is None:
        benchmarks = [x for x in Path(C_DIR).glob("*/") if x.is_dir()]
    else:
        benchmarks = [C_DIR / x for x in args.only.split(",")]

    if args.clean:
        clean(benchmarks)

    if args.compile:
        compile(benchmarks)

    exit(0)
    


    


    
