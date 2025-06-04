#ifndef FS_TYPES_H_
#define FS_TYPES_H_

#define BLOCK_SIZE 1024
#define MAX_NAME_SIZE 28

typedef enum {
    TYPE_FILE = 0,
    TYPE_DIR
} Type;


typedef struct SuperBlock {
    int magic_number; // filesystem identifier
    int num_blocks;   // total blocks(1024)
    int num_inodes;   // total inodes(e.g., 128)
    int bitmap_start; // block index of free-block bitmap
    int inode_start;  // block index of inode table
    int data_start;   // block index of first data block
} SuperBlock;


typedef struct Inode {
    int is_valid;         // 0=free, 1= used
    int size;             // bytes(file) or entry count(directory)
    int direct_blocks[4]; // direct block pointers
    int is_directory;     // 0=file, 1= directory
    int owner_id;         // your student id number
} Inode;


typedef struct DirectoryEntry {
    int inode_number;
    char name[MAX_NAME_SIZE]; // 27 ASCII chars + null terminator (\0)
} DirectoryEntry;

#endif // FS_TYPES_H_