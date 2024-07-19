#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "./utils.h"

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
    buffer = malloc(len);
    fread(buffer, len + 1, 1, f);
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
