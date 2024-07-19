#ifndef RENDER_H_
#define RENDER_H_

#define BUFFER_SIZE 1024 * 10

#include "./la.h"
#include <stdlib.h>
#include <GL/gl.h>


typedef struct {
    Vec2f pos;
    Vec2f uv;
} Vertex;

typedef struct {
    GLuint vao;
    GLuint vbo;

    GLuint program;

    size_t buffer_count;
    Vertex buffer[BUFFER_SIZE];
} Render;

void gl_render_init(Render* glr, const char* vert_file, const char* frag_file);
void gl_render_vertex(Render* glr, Vec2f p, Vec2f uv);
void gl_render_triangle(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3,
    Vec2f uv1, Vec2f uv2, Vec2f uv3);

void gl_render_quad(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4,
    Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4);

void gl_render_rec(Render* glr, Vec2f p, Vec2f s);
void gl_render_img(Render* glr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs);
    
void gl_clear(Render* glr);
void gl_sync(Render* glr);

#endif // RENDER_H_
