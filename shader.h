#ifndef SHADER_H_
#define SHADER_H_

#include "./glad/glad.h"

typedef struct _Shader {
    char* name;
    GLuint program;
    char* vert_file;
    char *frag_file;
    struct _Shader* next;
} Shader;

void shader_create(Shader** shdr, const char* name, const char* vert, const char* frag);
void shader_free(Shader** shdr);
void shader_destroy(Shader** shdr, const char* name);
void shader_use(Shader* shdr, const char* name);
void shader_print_files(Shader* shdr, const char* name);
Shader* shader_get(Shader* shdr, const char* name);
GLuint shader_get_program(Shader* shdr, const char* name);

#endif // SHADER_H_
