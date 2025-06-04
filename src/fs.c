#include "fs.h"
#include "disk.h"
#include "fs_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mkfs(const char *diskfile) {
    FILE *fp = fopen(diskfile, "wb+");
    if (!fp) {
        fprintf(stderr, "Error: mkfs: cannot open disk file\n");
        return;
    }

    strcpy(disk_image, diskfile);
    sprintf(backup_image, "%s.backup", disk_image);

    char zero[BLOCK_SIZE];
    memset(zero, 0, sizeof(zero));
    fwrite(zero, sizeof(zero), 1024, fp);

    SuperBlock sb;
    sb.magic_number = 0xDEADBEEF;
    sb.num_blocks = 1024;
    sb.num_inodes = 1;
    sb.bitmap_start = 1;
    sb.inode_start = 2;
    sb.data_start = 11;

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, BLOCK_SIZE, 1, fp);

    Inode root_inode;
    root_inode.is_valid = 1;
    root_inode.is_directory = 1;
    root_inode.owner_id = 150230736;
    root_inode.size = 0;
    memset(root_inode.direct_blocks, -1, sizeof(root_inode.direct_blocks));

    int offset = BLOCK_SIZE * sb.inode_start;
    fseek(fp, offset, SEEK_SET);
    fwrite(&root_inode, BLOCK_SIZE, 1, fp);

    fclose(fp);
}

int mkdir_fs(const char *path) {
    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("mkdir_fs", path, ERR_PATH);
        return -1;
    }

    int token_len = strlen(tokens[depth-1]);
    if (tokens[depth-1][token_len-1] != '/') {
        tokens[depth-1][token_len] = '/';
        tokens[depth-1][token_len+1] = '\0';
    }

    begin_transaction();

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("mkdir_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("mkdir_fs", path, ERR_FORMAT);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int parent_inode = find_inode_by_path(disk, sb.inode_start, tokens, depth-1);
    if (parent_inode == -1) {
        print_error("mkdir_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    if (is_dir_exist(disk, sb.inode_start, parent_inode, tokens[depth-1])) {
        print_error("mkdir_fs", path, ERR_DIR_EXISTS);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    Inode parent;
    read_inode(disk, sb.inode_start, parent_inode, &parent);

    DirectoryEntry entries[MAX_ENTRIES];
    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            DirectoryEntry *entry = &entries[j];
            
            if (entry->inode_number == 0) {
                int new_inode = create_inode(disk, &sb, TYPE_DIR);
                if (new_inode == -1) {
                    print_error("mkdir_fs", path, ERR_NO_SPACE);
                    return -1;
                }

                entry->inode_number = new_inode;
                memset(entry->name, 0, sizeof(entry->name));
                strcpy(entry->name, tokens[depth-1]);

                write_block(disk, block_number, entries);

                parent.size++;
                write_inode(disk, sb.inode_start, parent_inode, &parent);

                fclose(disk);
                commit_transaction();
                return 0;
            }
        }
    }

    char bitmap[BLOCK_SIZE];
    read_block(disk, sb.bitmap_start, bitmap);

    int bitmap_size = 1024 - sb.data_start;

    for (int i = 0; i < 4; ++i) {
        if (parent.direct_blocks[i] != -1) {
            continue;
        }

        for (int j = 0; j < bitmap_size; ++j) {
            if (bitmap[j] != 0) {
                continue;
            }

            bitmap[j] = 1;

            int new_inode = create_inode(disk, &sb, TYPE_DIR);
            if (new_inode == -1) {
                print_error("mkdir_fs", path, ERR_NO_SPACE);
                fclose(disk);
                rollback_transaction();
                return -1;
            }

            write_block(disk, sb.bitmap_start, bitmap);

            memset(entries, 0, sizeof(entries));
            entries[0].inode_number = new_inode;
            strcpy(entries[0].name, tokens[depth-1]);

            int new_block = sb.data_start + j;
            write_block(disk, new_block, entries);

            parent.direct_blocks[i] = new_block;
            parent.size++;

            write_inode(disk, sb.inode_start, parent_inode, &parent);

            fclose(disk);
            commit_transaction();
            return 0;
        }
    }

    print_error("mkdir_fs", path, ERR_NO_SPACE); 
    fclose(disk);
    rollback_transaction();
    return -1;
}

int create_fs(const char *path) {
    if (path[strlen(path)-1] == '/') {
        print_error("create_fs", path, ERR_NO_SUCH_FILE);
        return -1;
    }

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("create_fs", path, ERR_PATH);
        return -1;
    }

    begin_transaction();

    FILE *disk = fopen(disk_image, "rb+");
    if (disk == NULL) {
        print_error("create_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("create_fs", path, ERR_FORMAT);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int parent_inode = find_inode_by_path(disk, sb.inode_start, tokens, depth - 1);
    if (parent_inode == -1) {
        print_error("create_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    if (is_file_exist(disk, sb.inode_start, parent_inode, tokens[depth - 1])) {
        print_error("create_fs", path, ERR_FILE_EXISTS);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    Inode parent;
    read_inode(disk, sb.inode_start, parent_inode, &parent);

    DirectoryEntry entries[MAX_ENTRIES];
    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            DirectoryEntry *entry = &entries[j];
            if (entry->inode_number == 0) {
                int new_inode = create_inode(disk, &sb, TYPE_FILE);
                if (new_inode == -1) {
                    print_error("mkdir_fs", path, ERR_NO_SPACE);
                    return -1;
                }

                entry->inode_number = new_inode;
                memset(entry->name, 0, sizeof(entry->name));
                strcpy(entry->name, tokens[depth - 1]);

                write_block(disk, block_number, entries);

                parent.size++;
                write_inode(disk, sb.inode_start, parent_inode, &parent);

                fclose(disk);
                commit_transaction();
                return 0;
            }
        }
    }

    char bitmap[BLOCK_SIZE];
    read_block(disk, sb.bitmap_start, bitmap);

    int bitmap_size = 1024 - sb.data_start;

    for (int i = 0; i < 4; ++i) {
        if (parent.direct_blocks[i] != -1) {
            continue;
        }

        for (int j = 0; j < bitmap_size; ++j) {
            if (bitmap[j] != 0) {
                continue;
            }

            bitmap[j] = 1;

            int new_inode = create_inode(disk, &sb, TYPE_FILE);
            if (new_inode == -1) {
                print_error("mkdir_fs", path, ERR_NO_SPACE);
                fclose(disk);
                rollback_transaction();
                return -1;
            }

            write_block(disk, sb.bitmap_start, bitmap);

            memset(entries, 0, sizeof(entries));
            entries[0].inode_number = new_inode;
            strcpy(entries[0].name, tokens[depth - 1]);

            int new_block = sb.data_start + j;
            write_block(disk, new_block, entries);

            parent.direct_blocks[i] = new_block;
            parent.size++;

            write_inode(disk, sb.inode_start, parent_inode, &parent);

            fclose(disk);
            commit_transaction();
            return 0;
        }
    }

    print_error("mkdir_fs", path, ERR_NO_SPACE);
    fclose(disk);
    rollback_transaction();
    return -1;
}

int write_fs(const char *path, const char *data) {

    if (path[strlen(path)-1] == '/') {
        print_error("create_fs", path, ERR_NO_SUCH_FILE);
        return -1;
    }

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("write_fs", path, ERR_PATH);
        return -1;
    }

    begin_transaction();

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("write_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("write_fs", path, ERR_FORMAT);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int inode_number = find_inode_by_path(disk, sb.inode_start, tokens, depth);
    if (inode_number == -1) {
        print_error("write_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    Inode inode;
    read_inode(disk, sb.inode_start, inode_number, &inode);

    if (inode.is_directory != 0) {
        print_error("write_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int data_size = strlen(data);
    if (inode.size + data_size > 4 * BLOCK_SIZE) {
        print_error("write_fs", path, ERR_NO_SPACE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    char bitmap[BLOCK_SIZE];
    read_block(disk, sb.bitmap_start, bitmap);

    int remaining = data_size;

    int block_index = inode.size / BLOCK_SIZE;
    int block_offset = inode.size % BLOCK_SIZE;

    while (remaining > 0 && block_index < 4) {
        int block_number = -1;
        char block[BLOCK_SIZE];

        if (inode.direct_blocks[block_index] == -1) {
            int bitmap_size = sb.num_blocks - sb.data_start;

            for (int i = 0; i < bitmap_size; ++i) {
                if (bitmap[i] == 0) {
                    bitmap[i] = 1;
                    block_number = sb.data_start + i;
                    inode.direct_blocks[block_index] = block_number;

                    memset(block, 0, BLOCK_SIZE);

                    write_block(disk, sb.bitmap_start, bitmap);
                    break;
                }
            }

            if (block_number == -1) {
                print_error("mkdir_fs", path, ERR_NO_SPACE);
                fclose(disk);
                rollback_transaction();
                return -1;
            }
        } else {

            block_number = inode.direct_blocks[block_index];
            read_block(disk, block_number, block);
        }

        int space_in_block = BLOCK_SIZE - block_offset;
        int to_write = remaining < space_in_block ? remaining : space_in_block;

        memcpy(block+block_offset, data, to_write);

        write_block(disk, block_number, block);

        data += to_write;
        remaining -= to_write;
        inode.size += to_write;

        block_index++;
        block_offset = 0;
    }

    if (remaining > 0) {
        print_error("mkdir_fs", path, ERR_NO_SPACE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    write_inode(disk, sb.inode_start, inode_number, &inode);

    fclose(disk);
    commit_transaction();
    return data_size;
}

int read_fs(const char *path, char *buf, int bufsize) {

    if (path[strlen(path)-1] == '/') {
        print_error("create_fs", path, ERR_NO_SUCH_FILE);
        return -1;
    }

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("read_fs", path, ERR_PATH);
        return -1;
    }

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("read_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("read_fs", path, ERR_FORMAT);
        fclose(disk);
        return -1;
    }

    int inode_number = find_inode_by_path(disk, sb.inode_start, tokens, depth);
    if (inode_number == -1) {
        print_error("read_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        return -1;
    }

    Inode inode;
    read_inode(disk, sb.inode_start, inode_number, &inode);

    if (inode.is_directory != 0) {
        print_error("read_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        return -1;
    }

    int to_read = inode.size < bufsize ? inode.size : bufsize;

    int read_total = 0;
    int block_index = 0;

    while (to_read > 0 && block_index < 4) {
        int block_number = inode.direct_blocks[block_index];

        if (block_number == -1) {
            break;
        }

        char block[BLOCK_SIZE];
        read_block(disk, block_number, block);

        int chunk = to_read < BLOCK_SIZE ? to_read : BLOCK_SIZE;

        memcpy(buf + read_total, block, chunk);
        read_total += chunk;
        to_read -= chunk;

        block_index++;
    }

    buf[read_total] = '\0';

    fclose(disk);
    return read_total;
}

int delete_fs(const char *path) {

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("delete_fs", path, ERR_PATH);
        return -1;
    }

    begin_transaction();

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("delete_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("delete_fs", path, ERR_FORMAT);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int parent_inode = find_inode_by_path(disk, sb.inode_start, tokens, depth-1);
    if (parent_inode == -1) {
        print_error("delete_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    Inode parent;
    read_inode(disk, sb.inode_start, parent_inode, &parent);

    Inode inode;
    int inode_number = -1;

    DirectoryEntry entries[MAX_ENTRIES];

    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            DirectoryEntry *entry = &entries[j];
            if (entry->inode_number != 0 && strcmp(entry->name, tokens[depth-1]) == 0) {
                inode_number = entry->inode_number;
                read_inode(disk, sb.inode_start, inode_number, &inode);

                if (inode.is_directory != 0) {
                    inode_number = -1;
                    continue;
                }

                entry->inode_number = 0;
                break;
            }
        }

        if (inode_number != -1) {

            write_block(disk, block_number, entries);
            break;
        }
    }

    if (inode_number == -1) {
        print_error("delete_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    char bitmap[BLOCK_SIZE];
    read_block(disk, sb.bitmap_start, &bitmap);

    for (int i = 0; i < 4; ++i) {
        int block_number = inode.direct_blocks[i];
        if (block_number != -1) {
            bitmap[block_number - sb.data_start] = 0;
        }
    }

    write_block(disk, sb.bitmap_start, bitmap);

    inode.is_valid = 0;
    inode.size = 0;
    memset(inode.direct_blocks, -1, sizeof(inode.direct_blocks));

    write_inode(disk, sb.inode_start, inode_number, &inode);

    parent.size--;
    write_inode(disk, sb.inode_start, parent_inode, &parent);

    sb.num_inodes--;
    write_superblock(disk, &sb);

    fclose(disk);

    commit_transaction();
    return 0;
}

int rmdir_fs(const char *path) {

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("rmdir_fs", path, ERR_PATH);
        return -1;
    }

    int token_len = strlen(tokens[depth-1]);
    if (tokens[depth-1][token_len-1] != '/') {
        tokens[depth-1][token_len] = '/';
        tokens[depth-1][token_len+1] = '\0';
    }

    begin_transaction();

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("rmdir_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("rmdir_fs", path, ERR_FORMAT);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    int parent_inode = find_inode_by_path(disk, sb.inode_start, tokens, depth-1);
    if (parent_inode == -1) {
        print_error("rmdir_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    Inode parent;
    read_inode(disk, sb.inode_start, parent_inode, &parent);

    Inode inode;
    int inode_number = -1;

    DirectoryEntry entries[MAX_ENTRIES];

    for (int i = 0; i < 4; ++i) {
        int block_number = parent.direct_blocks[i];
        if (block_number == -1) {
            continue;
        }

        read_block(disk, block_number, entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            DirectoryEntry *entry = &entries[j];
            if (entry->inode_number != 0 && strcmp(entry->name, tokens[depth-1]) == 0) {
                inode_number = entry->inode_number;
                read_inode(disk, sb.inode_start, inode_number, &inode);

                if (inode.is_directory != 1) {
                    inode_number = -1;
                    continue;
                }

                entry->inode_number = 0;
                break;
            }
        }

        if (inode_number != -1) {

            write_block(disk, block_number, entries);
            break;
        }
    }

    if (inode_number == -1) {
        print_error("rmdir_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    if (inode.size > 0) {
        print_error("rmdir_fs", path, ERR_DIR_NOT_EMPTY);
        fclose(disk);
        rollback_transaction();
        return -1;
    }

    char bitmap[BLOCK_SIZE];
    read_block(disk, sb.bitmap_start, &bitmap);

    for (int i = 0; i < 4; ++i) {
        int block_number = inode.direct_blocks[i];
        if (block_number != -1) {
            bitmap[block_number - sb.data_start] = 0;
        }
    }

    write_block(disk, sb.bitmap_start, bitmap);

    inode.is_valid = 0;
    inode.size = 0;
    memset(inode.direct_blocks, -1, sizeof(inode.direct_blocks));

    write_inode(disk, sb.inode_start, inode_number, &inode);

    parent.size--;
    write_inode(disk, sb.inode_start, parent_inode, &parent);

    sb.num_inodes--;
    write_superblock(disk, &sb);

    fclose(disk);

    commit_transaction();
    return 0;
}

int ls_fs(const char *path, DirectoryEntry *entries, int max_entries) {

    char tokens[MAX_DEPTH][TOKEN_LEN];
    int depth = tokenize_path(path, tokens);
    if (depth == -1) {
        print_error("ls_fs", path, ERR_PATH);
        return -1;
    }

    int token_len = strlen(tokens[depth-1]);
    if (tokens[depth-1][token_len-1] != '/') {
        tokens[depth-1][token_len] = '/';
        tokens[depth-1][token_len+1] = '\0';
    }

    FILE *disk = fopen(disk_image ,"rb+");
    if (disk == NULL) {
        print_error("ls_fs", path, ERR_DISK);
        return -1;
    }

    SuperBlock sb;
    if (read_superblock(disk, &sb) != 0) {
        print_error("ls_fs", path, ERR_FORMAT);
        fclose(disk);
        return -1;
    }

    int inode_number = find_inode_by_path(disk, sb.inode_start, tokens, depth);
    if (inode_number == -1) {
        print_error("ls_fs", path, ERR_NO_SUCH_FILE);
        fclose(disk);
        return -1;
    }

    Inode inode;
    read_inode(disk, sb.inode_start, inode_number, &inode);

    int num_entries = 0;
    DirectoryEntry block_entries[MAX_ENTRIES];

    for (int i = 0; i < 4 && num_entries < max_entries; ++i) {
        int block_num = inode.direct_blocks[i];
        if (block_num == -1) {
            continue;
        }

        read_block(disk, block_num, block_entries);

        for (int j = 0; j < MAX_ENTRIES; ++j) {
            if (block_entries[j].inode_number != 0) {
                entries[num_entries++] = block_entries[j];

                if (num_entries == max_entries) {
                    break;
                }
            }
        }
    }

    fclose(disk);
    return num_entries;
}
