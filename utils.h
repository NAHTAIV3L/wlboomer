#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>

char *read_file(const char *file);
void die(const char *fmt, ...);
int allocate_shm_file(const char* filename, size_t size);
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define CLAMP(x, min, max) (x > max ? max : (x < min ? min : x))
#endif //UTILS_H_
