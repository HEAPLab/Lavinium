#!/usr/bin/env python

from pathlib import Path
import argparse
from re import split
from typing import List
from modules.subprocess_helper import run
from modules.pretty_print import bold, color_print, green, red, yellow
import os
import modules.debug as debug
import pandas as pd


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
SECOND_COMPILATION_FLAG = f"{COMMON_FLAG} -mllvm {TA_MEMORY} -mllvm {TA_MUARCH} -mllvm -lavinium-enable -mllvm -lavinium-file={LAVINIUMPASSES} -mllvm --ta-strict=false -mllvm --ta-lpsolver-effort=minimal" 



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
    compiled = []
    results = []
    
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
        if b"unable to find library -lclang_rt.builtins-arm" in err:
            if b"18446744073709551615" in err:
                color_print(b"\tLavinium    %b " % yellow("Unbounded :'("))
                write_to_file(directory + "/lavinium_res",err)
                results.extend(extract_value(stem,err))
                compiled.append(f"{stem}")
            else:
                color_print(b"\tLavinium    %b " % green("OK!"))
                write_to_file(directory + "/lavinium_res",err)
                results.extend(extract_value(stem,err))
                compiled.append(f"{stem}")
        else: 
            color_print(b"\tLavinium    %b " % red("ERR"))
            write_to_file(directory + "/lavinium_error",err)
    return (compiled, results)

def extract_value(name, data):
    splitted_data = data.splitlines()
    start = 0 
    end = 0
    res = []
    for i, line in enumerate(splitted_data):
        if line.startswith(b"WCET Result"):
            start = i+1
        if line.startswith(b"ld.lld: error:"):
            end = i
            break
    if start == 0 or end == 0 or start == end:
        raise Exception(f"Malformed result of {name}")
    splitted_data = splitted_data[start:end]

    for line in splitted_data:
        split_line = line.split(b":")
        chosed_pass = split_line[0]
        wcet = split_line[1]
        wcet = int(wcet)
        res.append((name, chosed_pass, wcet))
    return res



def clean(benchmarks : List[Path], keep_extensions : tuple[str]):
    for benchmark in benchmarks:
        files = [x for x in benchmark.glob("*") if not x.name.endswith(keep_extensions)]
        for file in files:
            file.unlink()


if __name__ == '__main__':
    check_env()
    parser = argparse.ArgumentParser(description='Lavinium runner')
    parser.add_argument('-only', metavar='name[,name]*', type=str, help='List of comma separated benchmark')
    parser.add_argument('-skip', metavar='name[,name]*', type=str, help='List of comma separated benchmarks')
    parser.add_argument('-clean-all', metavar='bool', type=bool, default=False, nargs='?', help='Clean All Lavinium Output' , const=True)    
    parser.add_argument('-clean', metavar='bool', type=bool, default=False, nargs='?', help='Clean All Lavinium Output except .lav files' , const=True)
    parser.add_argument('-compile', metavar='bool', type=bool, default=False, nargs='?', help='Compile Benchmarks' , const=True)
    parser.add_argument('-debug', metavar='bool', type=bool, default=False, nargs='?', help='Enable Debug' , const=True)
    args = parser.parse_args()

    if args.debug:
        debug.DEBUG = args.debug


    if args.only is None:
        benchmarks = [x for x in Path(C_DIR).glob("*/") if x.is_dir()]
    else:
        benchmarks = [C_DIR / x for x in args.only.split(",")]

    if args.skip :
        to_skip = [C_DIR / x for x in args.skip.split(",")]
        benchmarks = [x for x in benchmarks if x  not in to_skip]

    if args.clean_all:
        clean(benchmarks, (".c", ".h"))
    
    if args.clean:
        clean(benchmarks, (".c", ".h", ".lav"))
        
    if args.compile:
        compiled, results = compile(benchmarks)
        results = pd.DataFrame(results)
        results.to_csv("results.csv")
        print(f"List of successfully compiled benchmark {compiled}")
    exit(0)
    


    


    
