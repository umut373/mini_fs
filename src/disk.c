#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void begin_transaction() {
    char command[100];
    sprintf(command, "cp %s %s", disk_image, backup_image);

    system(command);
}

void rollback_transaction() {
    char command[100];
    sprintf(command, "cp %s %s", backup_image, disk_image);
    system(command);

    sprintf(command, "rm %s", backup_image);
    system(command);
}

void commit_transaction() {
    char command[100];
    sprintf(command, "rm %s", backup_image);

    system(command);
}

int read_superblock(FILE *disk, SuperBlock *sb) {
    char block[BLOCK_SIZE];
    fread(block, BLOCK_SIZE, 1, disk);

    memcpy(sb, block, sizeof(SuperBlock));

    if (sb->magic_number != 0xDEADBEEF) {
        return -1;
    }

    return 0;
}

void read_inode(FILE *disk, int inode_start, int inode_number, Inode *inode) {
    int block_number = inode_start + (inode_number / MAX_INODES);

    Inode inodes[MAX_INODES];
    read_block(disk, block_number, inodes);

    int index = inode_number % (MAX_INODES);
    memcpy(inode, &inodes[index], sizeof(Inode));
}

void read_block(FILE *disk, int block_number, void *bock) {
    int block_offset = block_number * BLOCK_SIZE;

    fseek(disk, block_offset, SEEK_SET);
    fread(bock, BLOCK_SIZE, 1, disk);
}

void write_superblock(FILE *disk, const SuperBlock *sb) {
    char block[BLOCK_SIZE];
    memset(block, 0, BLOCK_SIZE);

    memcpy(block, sb, sizeof(SuperBlock));
    fwrite(block, BLOCK_SIZE, 1, disk);
}

void write_inode(FILE *disk, int inode_start, int inode_number, const Inode *inode) {
    int block_number = inode_start + (inode_number / MAX_INODES);

    Inode inodes[MAX_INODES];
    read_block(disk, block_number, inodes);

    int index = inode_number % (MAX_INODES);
    inodes[index] = *inode;
    write_block(disk, block_number, inodes);
}

void write_block(FILE *disk, int block_number, const void *block) {
    int block_offset = block_number * BLOCK_SIZE;

    fseek(disk, block_offset, SEEK_SET);
    fwrite(block, BLOCK_SIZE, 1, disk);
}

int create_inode(FILE* disk, SuperBlock *sb, Type type) {
    int num_blocks = sb->data_start - sb->inode_start;

    Inode inodes[MAX_INODES];
    for (int i = 0; i < num_blocks; ++i) {
        read_block(disk, sb->inode_start+i, inodes);

        for (int j = 0; j < MAX_INODES; ++j) {
            if (inodes[j].is_valid == 0) {
                inodes[j].is_valid = 1;
                inodes[j].owner_id = 150230736;
                inodes[j].is_directory = type;
                inodes[j].size = 0;
                memset(inodes[j].direct_blocks, -1, sizeof(inodes[j].direct_blocks));

                write_block(disk, sb->inode_start+i, inodes);

                sb->num_inodes++;
                write_superblock(disk, sb);

                return i*MAX_INODES + j;
            }
        }
    }
    return -1;
}

int tokenize_path(const char *path, char tokens[MAX_DEPTH][TOKEN_LEN]) {
    if (path[0] != '/') {
        return -1;
    }

   int depth = 0;
    size_t path_len = strlen(path);
    char path_copy[500];

    strcpy(path_copy, path);

    char *buffer;
    char *token = strtok_r(path_copy, "/", &buffer);

    while (token != NULL && depth < MAX_DEPTH) {
        char *next_token = strtok_r(NULL, "/", &buffer);

        if (next_token == NULL && path[path_len - 1] != '/') {
            snprintf(tokens[depth], TOKEN_LEN, "%s", token);
        } else {
            snprintf(tokens[depth], TOKEN_LEN, "%s/", token);
        }

        depth++;
        token = next_token;
    }

    return depth;
}

int find_inode_by_path(FILE *disk, int inode_start, const char tokens[MAX_DEPTH][TOKEN_LEN], int depth) {
    int current_inode = 0;

    for (int i = 0; i < depth; ++i) {
        Inode dir_inode;
        read_inode(disk, inode_start, current_inode, &dir_inode);

        if (dir_inode.is_directory != 1) {
            return -1;
        }

        int found = 0;
        DirectoryEntry entries[MAX_ENTRIES];

        for (int j = 0; j < 4; ++j) {
            int block_number = dir_inode.direct_blocks[j];
            if (block_number == -1) {
                continue;
            }

            read_block(disk, block_number, entries);

            for (int k = 0; k < MAX_ENTRIES; ++k) {
                DirectoryEntry *entry = &entries[k];
                if (entry->inode_number != 0 && strcmp(entry->name, tokens[i]) == 0) {
                    current_inode = entry->inode_number;
                    found = 1;
                    break;
                }
            }

            if (found) {
                break;
            }
        }

        if (!found) {
            return -1;
        }
    }

    return current_inode;
}

int is_file_exist(FILE *disk, int inode_start, int parent_inode, const char *name) {
    Inode parent;
    read_inode(disk, inode_start, parent_inode, &parent);

    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        DirectoryEntry entries[MAX_ENTRIES];
        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            int inode_num = entries[j].inode_number;

            if (inode_num == 0) {
                continue;
            }

            Inode child;
            read_inode(disk, inode_start, inode_num, &child);

            if (child.is_directory == 0 && strcmp(entries[j].name, name) == 0) {
                return 1;
            }
        }
    }

    return 0;
}


int is_dir_exist(FILE *disk, int inode_start, int parent_inode, const char *name) {
    Inode parent;
    read_inode(disk, inode_start, parent_inode, &parent);

    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        DirectoryEntry entries[MAX_ENTRIES];
        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            int inode_num = entries[j].inode_number;

            if (inode_num == 0) {
                continue;
            }

            Inode child;
            read_inode(disk, inode_start, inode_num, &child);

            if (child.is_directory == 1 && strcmp(entries[j].name, name) == 0) {
                return 1;
            }
        }
    }

    return 0;
}
