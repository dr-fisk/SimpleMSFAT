#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SUPERBLOCK_INDEX 0
#define FAT_EOC 0xFFFF
#define FD_AMT 32
#define SIGNATURE "ECS150FS"

struct SuperBlock {
        uint8_t signature[8];
        uint16_t totalBlocks;
        uint16_t rootIndex;
        uint16_t dataBlockIndex;
        uint16_t dataBlockAmt;
        uint8_t fatAmt;
        uint8_t padding[4079];
};

struct FAT {
        uint16_t entry;
};

struct Root {
        uint8_t fileName[FS_FILENAME_LEN];
        uint32_t fileSize;
        uint16_t dataIndex;
        uint8_t padding[10];
};

enum status {OPEN, CLOSED};

struct FileDescriptor {
        uint8_t fileName[FS_FILENAME_LEN];
        int index;
        int offset;
        enum status fileStatus;
};

static struct SuperBlock *superBlock;
static struct FAT *fat;
static struct Root *root;
static struct FileDescriptor *FD;

int mount_error_handling() {
        if (memcmp(superBlock->signature, SIGNATURE, 8) != 0)
                return -1;

        if (fat == NULL || superBlock == NULL || root == NULL || FD == NULL)
                return -1;

        if (superBlock->totalBlocks != block_disk_count())
                return -1;

        return 0;
}

int num_empty_root(void) {
        int freeRootEntries = 0;

        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (root[i].fileName[0] == 0)
                        freeRootEntries ++;
        }

        return freeRootEntries;
}

int num_empty_fat(void) {
        int freeFatEntries = 0;

        for (int i = 0; i < superBlock->dataBlockAmt; i ++) {
                if (fat[i].entry == 0)
                        freeFatEntries ++;
        }

        return freeFatEntries;
}

int initializeDataStructures(void) {
        int fatSize;

        superBlock = malloc(sizeof(struct SuperBlock) * BLOCK_SIZE);

        if (superBlock == NULL)
                return -1;

        if (block_read(SUPERBLOCK_INDEX, superBlock) == -1)
                return -1;

        fatSize =  BLOCK_SIZE * superBlock->fatAmt;
        fat = malloc(sizeof(struct FAT) * fatSize);
        root = malloc(sizeof(struct Root) * FS_FILE_MAX_COUNT);
        FD = malloc(sizeof(struct FileDescriptor) * FD_AMT);

        for(int i = 0; i < FD_AMT; i++) {
                memset(FD[i].fileName, 0, FS_FILENAME_LEN);
                FD[i].index = -1;
                FD[i].fileStatus = CLOSED;
                FD[i].offset = 0;
        }

        return 0;
}


void clean_fat_entries(int fileIndex) {
        int fatIndex = root[fileIndex].dataIndex, prevIndex;

        while(fatIndex != FAT_EOC) {
                prevIndex = fatIndex;
                fatIndex = fat[prevIndex].entry;
                fat[prevIndex].entry = 0;
        } 
}

int find_file_in_root(const char *filename) {
        char *rootFileName = malloc(sizeof(char) * FS_FILENAME_LEN);

        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                memcpy(rootFileName, root[i].fileName, FS_FILENAME_LEN);
                if (strcmp(rootFileName, filename) == 0) {
                        if (rootFileName)
                                free(rootFileName);

                        return i;
                }
        }

        if (rootFileName)
                free(rootFileName);

        return -1;
}

int fd_error_handling(int fd) {
        if (FD == NULL)
                return -1;

        if (fd < 0 || fd > 31)
                return -1;

        if (FD[fd].fileStatus == CLOSED)
                return -1;

        return 0;
}

int get_fat_index(int fd) {
        int offsetIndexes = FD[fd].offset / BLOCK_SIZE;
        int index = root[FD[fd].index].dataIndex;

        if (fd_error_handling(fd) == -1)
                return -1;

        for (int i = 0; i < offsetIndexes; i++)
                index = fat[index].entry;

        return index;
}

int allocate_data_block(int fd, int fatEOCIndex) {
        for (int i = 1; i < superBlock->dataBlockAmt; i ++) {
                if (fat[i].entry == 0) {
                        fat[i].entry = FAT_EOC;

                        if (fatEOCIndex != FAT_EOC)
                                fat[fatEOCIndex].entry = i;
                        else {
                                root[FD[fd].index].dataIndex = i;
                        }

                        return 0;
                }
        }

        return -1;
}

int fs_mount(const char *diskname) {
        int fatIndex = 0;

        if (block_disk_open(diskname) == -1)
                return -1;

        if (initializeDataStructures() == -1)
                return -1;

        if(mount_error_handling() == -1)
                return -1;

        for (int i = 0; i < superBlock->fatAmt; i++) {
                fatIndex += i * BLOCK_SIZE;

                if (block_read(i + 1, fat + fatIndex) == -1)
                        return -1;
        }

        if (block_read(superBlock->rootIndex, root) == -1)
                return -1;

        return 0;
}

int fs_umount(void) {
        int fatIndex = 0;

        if (block_write(SUPERBLOCK_INDEX, superBlock) == -1)
                return -1;

        for (int i = 0; i < superBlock->fatAmt; i++) {
                fatIndex += i * BLOCK_SIZE;

                if (block_write(i + 1, fat + fatIndex) == -1)
                        return -1;
        }

        if (block_write(superBlock->rootIndex, root) == -1)
                return -1;
        
        if (block_disk_close() == -1)
                return -1;

        if (superBlock == NULL || fat == NULL || root == NULL)
                return -1;

        free(superBlock);
        free(fat);
        free(root);
        free(FD);

        return 0;
}

int fs_info(void) {
        if (superBlock == NULL || fat == NULL || root == NULL)
                return -1;

        printf("FS Info:\n");
        printf("total_blk_count=%d\n", superBlock->totalBlocks);
        printf("fat_blk_count=%d\n", superBlock->fatAmt);
        printf("rdir_blk=%d\n", superBlock->rootIndex);
        printf("data_blk=%d\n", superBlock->dataBlockIndex);
        printf("data_blk_count=%d\n", superBlock->dataBlockAmt);
        printf("fat_free_ratio=%d/%d\n", num_empty_fat(), superBlock->dataBlockAmt);
        printf("rdir_free_ratio=%d/%d\n", num_empty_root(), FS_FILE_MAX_COUNT);
        return 0;
}

int fs_create(const char *filename) {
        if (find_file_in_root(filename) != -1)
                return -1;

        if (strlen(filename) - 1 == 0 || (int) strlen(filename) >= 16
                || filename[strlen(filename)] != 0)
                return -1;


        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (root[i].fileName[0] == 0) {
                        memcpy(root[i].fileName, filename, FS_FILENAME_LEN);
                        root[i].fileSize = 0;
                        root[i].dataIndex = FAT_EOC;
                        return 0;
                }
        }

        return -1;
}

int fs_delete(const char *filename) {
        int rootIndex;

        rootIndex = find_file_in_root(filename);

        if (rootIndex == -1)
                return -1;

        clean_fat_entries(rootIndex);
        memset(root[rootIndex].fileName, 0, FS_FILENAME_LEN);
        root[rootIndex].fileSize = 0;
        root[rootIndex].dataIndex = 0;
        return 0;
}

int fs_ls(void) {
        if (superBlock == NULL || fat == NULL || root == NULL)
                return -1;

        printf("FS Ls:\n");

        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (root[i].fileName[0] != 0) {
                        printf("file: %s, ", root[i].fileName);
                        printf("size: %d, ", root[i].fileSize);
                        printf("data_blk: %d\n", root[i].dataIndex);
                }
        }

        return 0;
}

int fs_open(const char *filename) {
        int rootIndex;

        if (FD == NULL)
                return -1;

        rootIndex = find_file_in_root(filename);

        if (rootIndex == -1)
                return -1;

        for (int i = 0; i < FD_AMT; i ++) {
                if (FD[i].fileStatus == CLOSED) {
                        struct FileDescriptor *fd = (struct FileDescriptor *) filename;
                        fd->index = rootIndex;
                        fd->fileStatus = OPEN;
                        fd->offset = 0;
                        FD[i] = *fd;
                        return i;
                }
        }

        return -1;
}

int fs_close(int fd) {
        if (fd_error_handling(fd) == -1)
                return -1;

        FD[fd].index = -1;
        FD[fd].fileStatus = CLOSED;
        FD[fd].offset = 0;
        memset(FD[fd].fileName, 0, FS_FILENAME_LEN);

        return 0;
}

int fs_stat(int fd) {
        int rootIndex;

        if (fd_error_handling(fd) == -1)
                return -1;

        char *filename = malloc(sizeof(char) * FS_FILENAME_LEN);
        memcpy(filename, FD[fd].fileName, FS_FILENAME_LEN);
        rootIndex = find_file_in_root(filename);

        if (filename)
                free(filename);

        if (rootIndex == -1)
                return -1;

        return root[rootIndex].fileSize;
}

int fs_lseek(int fd, size_t offset) {
        if (fd_error_handling(fd) == -1)
                return -1;

        if (offset > root[FD[fd].index].fileSize)
                return -1;

        FD[fd].offset = offset;

        return 0;
}

int fs_write(int fd, void *buf, size_t count) {
        int bytesWrote = 0, fatIndex;
        uint8_t bounceBuf[BLOCK_SIZE], data[count];

        if (count == 0)
                return bytesWrote;

        memcpy(data, buf, count);

        if (fd_error_handling(fd) == -1)
                return -1;

        fatIndex = get_fat_index(fd);

        /* When file is newly opened, root index == FAT EOC
        therefore this allocates the first data block*/
        if (fatIndex == FAT_EOC) {
                if (allocate_data_block(fd, fatIndex) == -1)
                        return bytesWrote;

                fatIndex = get_fat_index(fd);
        }

        if (block_read(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                return -1;

        for (bytesWrote = 0; bytesWrote < (int) count; bytesWrote++) {
                if ((FD[fd].offset + bytesWrote) % BLOCK_SIZE ==  0
                                        && FD[fd].offset + bytesWrote != 0) {

                        if (fat[fatIndex].entry == FAT_EOC)
                                if (allocate_data_block(fd, fatIndex) == -1)
                                        break;

                        if (block_write(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                                return -1;

                        fatIndex = fat[fatIndex].entry;

                        if (block_read(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                                return -1;
                }

                bounceBuf[(FD[fd].offset + bytesWrote) % BLOCK_SIZE] = data[bytesWrote];
        }

        if (block_write(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                return -1;

        if (FD[fd].offset + bytesWrote > root[FD[fd].index].fileSize)
                root[FD[fd].index].fileSize += ((bytesWrote + FD[fd].offset) - root[FD[fd].index].fileSize);

        fs_lseek(fd, FD[fd].offset += bytesWrote);

        return bytesWrote;
}

int fs_read(int fd, void *buf, size_t count) {
        uint8_t bounceBuf[BLOCK_SIZE], fileData[count];
        int bytesRead, fatIndex;

        memset(fileData, 0, count);

        if (fd_error_handling(fd) == -1)
                return -1;

        fatIndex = get_fat_index(fd);

        if (fatIndex == FAT_EOC)
                return -1;

        if (block_read(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                return -1;

        // Read bytes up to size count
        for (bytesRead = 0; bytesRead < (int) count; bytesRead ++) {
                // Handles condition when we've reached the end of the file
                if (FD[fd].offset + bytesRead >= (int) root[FD[fd].index].fileSize)
                       break;

                // Grabs next datablock when we've read all 4096 bytes of a data block
                if ((FD[fd].offset + bytesRead) % BLOCK_SIZE ==  0
                                        && FD[fd].offset + bytesRead != 0) {

                        fatIndex = fat[fatIndex].entry;

                        if (block_read(superBlock->dataBlockIndex + fatIndex, bounceBuf) == -1)
                                return -1;
                }

                fileData[bytesRead] = bounceBuf[(FD[fd].offset + bytesRead) % BLOCK_SIZE];
        }

        fs_lseek(fd, FD[fd].offset + bytesRead);
        memcpy(buf, fileData, bytesRead);

        return bytesRead;
}

