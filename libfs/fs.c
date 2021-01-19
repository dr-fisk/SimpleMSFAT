#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

struct SuperBlock {
        unint16_t signature[4];
};

struct FAT {
};

struct rootDirectory {
};

int fs_mount(const char *diskname) {
        block_disk_open(diskname);
}

int fs_umount(void) {
        block_disk_close();
}

int fs_info(void) {
        /* TODO: Phase 1 */
}

int fs_create(const char *filename) {
        /* TODO: Phase 2 */
}

int fs_delete(const char *filename) {
        /* TODO: Phase 2 */
}

int fs_ls(void) {
        /* TODO: Phase 2 */
}

int fs_open(const char *filename) {
        /* TODO: Phase 3 */
}

int fs_close(int fd) {
        /* TODO: Phase 3 */
}

int fs_stat(int fd) {
        /* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset) {
        /* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count) {
        /* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

