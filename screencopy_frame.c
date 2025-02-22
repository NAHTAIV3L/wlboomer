#include "./state.h"
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

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

void screencopy_frame_buffer(void *data, struct zwlr_screencopy_frame_v1 *frame,
                             uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
    client_state* state = data;

    size_t size = width * height * 4;
    int fd = allocate_shm_file("/wlscreenshot", size);
    state->screen_data = mmap(NULL, size,
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    state->pool = wl_shm_create_pool(state->shm, fd, size);
    state->screen_buffer = wl_shm_pool_create_buffer(state->pool, 0, width, height,
                                                     stride, format);

    state->screen_width = width;
    state->screen_height = height;
    state->screen_format = format;
}

void screencopy_frame_flags(void *data,
                            struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                            uint32_t flags) {
}

void screencopy_frame_ready(void *data, struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                            uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec) {
}

void screencopy_frame_failed(void *data, struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1) {
    printf("failed to copy output\n");
}

void screencopy_frame_linux_dmabuf(void * data, struct zwlr_screencopy_frame_v1 * zwlr_screencopy_frame_v1,
                                   uint32_t format, uint32_t width, uint32_t height) {
}


void screencopy_frame_buffer_done(void * data, struct zwlr_screencopy_frame_v1 * zwlr_screencopy_frame_v1) {
    client_state* state = data;
    state->buffer_done = true;
}

struct zwlr_screencopy_frame_v1_listener screencopy_listener = {
    .buffer = screencopy_frame_buffer,
    .flags  = screencopy_frame_flags,
    .ready  = screencopy_frame_ready,
    .failed = screencopy_frame_failed,
    .linux_dmabuf = screencopy_frame_linux_dmabuf,
    .buffer_done = screencopy_frame_buffer_done,
};
