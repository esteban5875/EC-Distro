import ctypes
import subprocess
from pathlib import Path
lazy import shutil
lazy import os
from flask import Flask, jsonify, send_from_directory, abort

HOST = "0.0.0.0"
PORT = 8080

CURRENT_DIR = Path(__file__).resolve().parent
REPO_DIR = CURRENT_DIR.parents[3].joinpath("repo")

REPO_SOURCES  = REPO_DIR / "source"
REPO_BIN      = REPO_DIR / "bin"
REPO_TAR      = REPO_DIR / "tarball"

LIBTARBALL_SOURCE_NAME = "create_tar.c"
LIBTARBALL_BIN_NAME    = "libtarball.so"

LIBTARBALL_SOURCE_PATH = Path(__file__).parents[1].joinpath("tarball_lib")
LIBTARBALL_BIN_PATH = LIBTARBALL_SOURCE_PATH.joinpath("bin")

STAT_FAIL = -1  
ARCHIVE_OPEN_FAILURE = -2
WRITING_HEADER_FAILURE = -3
BIN_OPEN_FAILURE = -4
DATA_WRITE_FAILURE = -5
TAR_CREATE_SUCCESS = 0

def compile_libtarball():
    resultBin = LIBTARBALL_BIN_PATH.joinpath(LIBTARBALL_BIN_NAME) 
    
    resultBin.parent.mkdir(parents=True, exist_ok=True)
    
    command = [
        "gcc", 
        "-shared", 
        "-o", str(resultBin), 
        "-fPIC", str(LIBTARBALL_SOURCE_PATH.joinpath(LIBTARBALL_SOURCE_NAME)), 
        "-larchive"
    ]

    subprocess.run(command, check=True)
    return resultBin

def compile_sources():
    subprocess.run("make", cwd=str(REPO_DIR), check=True)

def create_tarballs():
    lib_path = compile_libtarball()
    lib = ctypes.CDLL(str(lib_path))
    
    lib.create_tarball.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
    lib.create_tarball.restype = ctypes.c_int
    
    REPO_TAR.mkdir(parents=True, exist_ok=True)
    result_tars = []
    
    if REPO_BIN.exists():
        for program_dir in REPO_BIN.iterdir():
            if not program_dir.is_dir():
                continue
                
            executable_path = program_dir / program_dir.name
            
            if executable_path.is_file() and os.access(executable_path, os.X_OK):
                tar_name = f"{program_dir.name}.tar.gz"
                out_tar_path = REPO_TAR / tar_name
                
                c_binary_path = str(executable_path).encode("utf-8")
                c_out_tar_path = str(out_tar_path).encode("utf-8")
                c_entry_name = executable_path.name.encode("utf-8")
                
                status = lib.create_tarball(c_binary_path, c_out_tar_path, c_entry_name)
                
                if status == TAR_CREATE_SUCCESS:
                    result_tars.append(out_tar_path)
                    
    return result_tars

def Clean():
    if LIBTARBALL_BIN_PATH.exists():
        shutil.rmtree(LIBTARBALL_BIN_PATH)
    
    if REPO_BIN.exists():
        shutil.rmtree(REPO_BIN)
        
    if REPO_TAR.exists():
        shutil.rmtree(REPO_TAR)

    LIBTARBALL_BIN_PATH.mkdir(parents=True, exist_ok=True)
    REPO_BIN.mkdir(parents=True, exist_ok=True)
    REPO_TAR.mkdir(parents=True, exist_ok=True)

def Proc():
    Clean()
    compile_sources()
    create_tarballs()
