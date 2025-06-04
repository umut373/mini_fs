# mini_fs File System

## Overview
This project is a simple file system that operates on a 1MB disk image (`disk.img`). It supports basic file and directory operations such as creating, reading, writing, and deleting files and directories.

## Features

- Operates on a virtual 1MB disk (`disk.img`) with 1KB block size
- Basic file operations: `create_fs`, `write_fs`, `read_fs`, `delete_fs`
- Basic directory operations: `mkdir_fs`, `ls_fs`, `rmdir_fs`

## Project Build and Execution Guide

### 1. Prerequisites
Ensure you have `gcc`, and `make`.

### 2. Build and Run
To build the project, use:
```sh
make release   # For release build
make debug     # For debug build
make all       # To build both versions
```

These will create:

- `build/` directory for release builds, with the executable in `/mini_fs`.
- `debug/` directory for debug builds, with the executable in `debug/bin/program`.
- Object files stored under `build/obj/` and `debug/obj/`.

### To run the program
```sh
make run
```
This will:
- Build the project if not already built.
- Execute the compiled binary located in `./mini_fs`. It runs a self test with predefined operations.

Alternatively, you can run the program directly using the compiled executable with command line arguments:
```sh
./mini_fs <command> <args>
```

### To check the program
```sh
make check
```
This will:
- Run commands in tests/commands.txt
- Compare result stored in tests/output.txt with tests/expected_output.txt

### To clean all build files
```sh
make clean
```

## File Structure
```
.
├── src/               # Source code files
├── include/           # Header files
├── build/             # Compiled files (release mode)
│ └── obj/             # Object files
├── debug/             # Compiled files (debug mode)
│ ├── bin/             # Debug executable
│ └── obj/             # Debug object files
├── tests/             # Automated tests
├── Makefile           # Build instructions
└── mini_fs            # Compiled executable
```
