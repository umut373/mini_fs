#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "fs.h"
#include "fs_types.h"


void print_commands() {
    printf("Usage:\n");
    printf("  ./mini_fs mkfs\n");
    printf("  ./mini_fs mkdir_fs <path>\n");
    printf("  ./mini_fs create_fs <path>\n");
    printf("  ./mini_fs write_fs <path> <data>\n");
    printf("  ./mini_fs read_fs <path>\n");
    printf("  ./mini_fs delete_fs <path>\n");
    printf("  ./mini_fs rmdir_fs <path>\n");
    printf("  ./mini_fs ls_fs <path>\n");
}


void self_test() {
    printf("$ ./mini_fs mkfs\n");
    mkfs("disk.img");

    printf("$ ./mini_fs mkdir_fs /dir1/\n");
    mkdir_fs("/dir1/");

    printf("$ ./mini_fs create_fs /file1.txt\n");
    create_fs("/file1.txt");

    printf("$ ./mini_fs create_fs /dir1/file2.txt\n");
    create_fs("/dir1/file2.txt");

    printf("$ ./mini_fs write_fs \"hello world1\"\n");
    int size = write_fs("/file1.txt", "hello world1");
    printf("%d\n", size);

    printf("$ ./mini_fs write_fs /dir1/file2.txt \"hello world2\"\n");
    size = write_fs("/dir1/file2.txt", "hello world2");
    size = printf("%d\n", size);

    printf("$ ./mini_fs read_fs /file1.txt\n");
    char buffer[100];
    size = read_fs("/file1.txt", buffer, 100);
    buffer[size] = '\0';
    printf("%s\n", buffer);

    printf("$ ./mini_fs read_fs /dir1/file2.txt\n");
    size = read_fs("/dir1/file2.txt", buffer, 100);
    buffer[size] = '\0';
    printf("%s\n", buffer);

    printf("$ ./mini_fs ls_fs /dir1/\n");
    DirectoryEntry entries[10];
    int count = ls_fs("/dir1/", entries, 10);
    for (int i = 0; i < count; ++i) {
        printf("%s\n", entries[i].name);
    }

    printf("$ ./mini_fs delete_fs /file1.txt\n");
    delete_fs("/file1.txt");

    printf("$ ./mini_fs ls_fs /\n");
    count = ls_fs("/", entries, 10);
    for (int i = 0; i < count; ++i) {
        printf("%s\n", entries[i].name);
    }

    printf("$ ./mini_fs delete_fs /dir1/file2.txt\n");
    delete_fs("/dir1/file2.txt");

    printf("$ ./mini_fs ls_fs /dir1\n");
    count = ls_fs("/dir1", entries, 10);
    for (int i = 0; i < count; ++i) {
        printf("%s\n", entries[i].name);
    }

    printf("$ ./mini_fs rmdir_fs /dir1/\n");
    rmdir_fs("/dir1/");

    printf("$ ./mini_fs ls_fs /\n");
    count = ls_fs("/", entries, 10);
    for (int i = 0; i < count; ++i) {
        printf("%s\n", entries[i].name);
    }
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        self_test();
        return 0;
    }


    if (strcmp(argv[1], "mkfs") == 0) {
        mkfs("disk.img");
    }


    else if (strcmp(argv[1], "mkdir_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: mkdir_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        mkdir_fs(argv[2]);
    } 


    else if (strcmp(argv[1], "create_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: create_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        create_fs(argv[2]);
    }


    else if (strcmp(argv[1], "write_fs") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: write_fs requires <path> <data>.\n");
            print_commands();
            return 1;
        }

        int size = write_fs(argv[2], argv[3]);
        if (size != -1) {
            printf("%d\n", size);
        }
    }


    else if (strcmp(argv[1], "read_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: read_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        char buf[100]; 
        int bytes = read_fs(argv[2], buf, sizeof(buf)-1);
        if (bytes > 0) {
            buf[bytes] = '\0';
            printf("%s\n", buf);
        }
    }


    else if (strcmp(argv[1], "delete_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: delete_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        delete_fs(argv[2]);
    }


    else if (strcmp(argv[1], "rmdir_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: rmdir_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        rmdir_fs(argv[2]);
    }


    else if (strcmp(argv[1], "ls_fs") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: ls_fs requires <path>.\n");
            print_commands();
            return 1;
        }
        DirectoryEntry entries[10];
        int count = ls_fs(argv[2], entries, 128);
        for (int i = 0; i < count; ++i) {
            printf("%s\n", entries[i].name);
        }
    }


    else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", argv[1]);
        print_commands();
        return 1;
    }

    return 0;
}
