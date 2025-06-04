#ifndef DISK_H_
#define DISK_H_

#include "fs_types.h"
#include <stdio.h>

#define MAX_DEPTH 20
#define TOKEN_LEN 28

#define MAX_ENTRIES BLOCK_SIZE/sizeof(DirectoryEntry)
#define MAX_INODES BLOCK_SIZE/sizeof(Inode)

static char disk_image[28] = "disk.img";
static char backup_image[35] = "disk.img.backup";

void begin_transaction();
void rollback_transaction();
void commit_transaction();

int read_superblock(FILE *disk, SuperBlock *sb);
void read_inode(FILE *disk, int inode_start, int inode_number, Inode *inode);
void read_block(FILE *disk, int block_number, void *bock);

void write_superblock(FILE *disk, const SuperBlock *sb);
void write_inode(FILE *disk, int inode_start, int inode_number, const Inode *inode);
void write_block(FILE *disk, int block_number, const void *bock);

int create_inode(FILE *disk, SuperBlock *sb, Type type);

int tokenize_path(const char *path, char tokens[MAX_DEPTH][TOKEN_LEN]);

int find_inode_by_path(FILE *disk, int inode_start, const char tokens[MAX_DEPTH][TOKEN_LEN], int depth);

int is_dir_exist(FILE *disk, int inode_start, int parent_inode, const char *name);
int is_file_exist(FILE *disk, int inode_start, int parent_inode, const char *name);

#endif // DISK_H_