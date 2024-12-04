#include "./utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

char *read_file(const char *file)
{
    char* buffer = NULL;
    FILE *f = fopen(file, "r");
    if (!f) {
        die("Failed to open file: %s", file);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(len + 1);
    fread(buffer, len, 1, f);
    fclose(f);
    buffer[len] = 0x00;
    return buffer;
}

void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}


#define BUFFER_SIZE 256

void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (size_t i = 0; i < strlen(buf); ++i) {
        if (buf[i] != 'X') continue;
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

int create_shm_file(const char* filename) {
    int retries = 100;
    char name[BUFFER_SIZE];
    size_t len = strlen(filename);
    memcpy(name, filename, len);
    const char* randomstr = "-XXXXXX";
    do {
        memcpy(&name[len], randomstr, strlen(randomstr));
        randname(name);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

int allocate_shm_file(const char* filename, size_t size) {
    int fd = create_shm_file(filename);
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}
