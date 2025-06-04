#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    ERR_NONE = 0,
    ERR_PATH,
    ERR_DISK,
    ERR_FORMAT,
    ERR_NO_SUCH_FILE,
    ERR_FILE_EXISTS,
    ERR_DIR_EXISTS,
    ERR_DIR_NOT_EMPTY,
    ERR_NO_SPACE
} ErrorCode;

void print_error(const char *command, const char* path, ErrorCode code);

#endif // ERRORS_H
