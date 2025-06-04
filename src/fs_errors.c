#include "fs_errors.h"
#include <stdio.h>

void print_error(const char *command, const char* path, ErrorCode code) {
    switch (code)
    {
    case ERR_PATH:
        fprintf(stderr, "Error: %s %s: paths must be absolute (start with '/')\n", command, path);
        break;

    case ERR_DISK:
        fprintf(stderr, "Error: %s %s: cannot open disk file\n", command, path);
        break;

    case ERR_FORMAT:
        fprintf(stderr, "Error: %s %s: wrong filesystem format\n", command, path);
        break;

    case ERR_NO_SUCH_FILE:
        fprintf(stderr, "Error: %s %s: no such file or directory\n", command, path);
        break;

    case ERR_FILE_EXISTS:
        fprintf(stderr, "Error: %s %s: file already exists\n", command, path);
        break;

    case ERR_DIR_EXISTS:
        fprintf(stderr, "Error: %s %s: directory already exists\n", command, path);
        break;

    case ERR_DIR_NOT_EMPTY:
        fprintf(stderr, "Error: %s %s: directory not empty\n", command, path);
        break;

    case ERR_NO_SPACE:
        fprintf(stderr, "Error: %s %s: no enough space\n", command, path);
        break;
    
    default:
        break;
    }
}