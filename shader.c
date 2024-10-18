#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./shader.h"
#include "./utils.h"

#ifndef INSTALL_PATH
#define INSTALL_PATH "./"
#endif

void shader_create(Shader** shdr, const char* name, const char* vert, const char* frag) {
    Shader* new_shdr = calloc(1, sizeof(Shader));

    new_shdr->name = malloc(strlen(name));
    strncpy(new_shdr->name, name, strlen(name));

    new_shdr->vert_file = malloc(strlen(vert));
    strncpy(new_shdr->vert_file, vert, strlen(vert));

    new_shdr->frag_file = malloc(strlen(frag));
    strncpy(new_shdr->frag_file, frag, strlen(frag));
    {
        char buffer[256];
        sprintf(buffer, "%s/%s", INSTALL_PATH, vert);
        char *vert_code = read_file(buffer);
        sprintf(buffer, "%s/%s", INSTALL_PATH, frag);
        char *frag_code = read_file(buffer);

        GLuint vertex, fragment;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, (const GLchar* const*)&vert_code, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            die("ERROR: Failed to Compile %s shader file: %s\n", new_shdr->vert_file, infoLog);
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, (const GLchar* const*)&frag_code, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            die("ERROR: Failed to Compile %s shader file: %s\n", new_shdr->frag_file, infoLog);
        }

        new_shdr->program = glCreateProgram();
        glAttachShader(new_shdr->program, vertex);
        glAttachShader(new_shdr->program, fragment);
        glLinkProgram(new_shdr->program);

        glGetProgramiv(new_shdr->program, GL_LINK_STATUS, &success);
        if (!success)
        {
            die("Shader Link Failed: %s\n", new_shdr->name);
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        free(vert_code);
        free(frag_code);
    }
    new_shdr->next = *shdr;
    *shdr = new_shdr;
}

void shader_free(Shader** shdr) {
    Shader* next = NULL;
    for (; *shdr; *shdr = next) {
        next = (*shdr)->next;
        free((*shdr)->name);
        free((*shdr)->vert_file);
        free((*shdr)->frag_file);
        free(*shdr);
    }
}

void shader_destroy(Shader** shdr, const char* name) {
    Shader* temp = *shdr;
    Shader* prev;
    if (!strncmp(temp->name, name, strlen(name))) {
        *shdr = temp->next;
        goto ret;
    }
    while (temp && strncmp(temp->name, name, strlen(name))) {
        prev = temp;
        temp = temp->next;
    }

    if (!temp) return;
    prev->next = temp->next;

ret:
    free(temp->name);
    free(temp->vert_file);
    free(temp->frag_file);
    free(temp);
}

void shader_use(Shader* shdr, const char* name) {
    for (; shdr; shdr = shdr->next) {
        int len = MIN(strlen(shdr->name), strlen(name));
        if (!strncmp(shdr->name, name, len)) {
            glUseProgram(shdr->program);
        }
    }
}

void shader_print_files(Shader* shdr, const char* name) {
    for (; shdr; shdr = shdr->next) {
        if (!strncmp(shdr->name, name, strlen(name))) {
            printf("Vertex Shader: %s\n"
                   "Fragment Shader: %s\n",
                shdr->vert_file, shdr->frag_file);
            return;
        }
    }
    printf("Shader not found: %s\n", name);
}

Shader* shader_get(Shader* shdr, const char* name) {
    for (; shdr; shdr = shdr->next) {
        if (!strncmp(shdr->name, name, strlen(name))) {
            return shdr;
        }
    }
    return NULL;
}

GLuint shader_get_program(Shader* shdr, const char* name) {
    for (; shdr; shdr = shdr->next) {
        if (!strncmp(shdr->name, name, strlen(name))) {
            return shdr->program;
        }
    }
    return -1;
}
