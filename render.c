#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "./glad/glad.h"
#include "./render.h"
#include "./utils.h"

void gl_render_init(Render* glr, const char* vert_file, const char* frag_file) {
    glEnable(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &glr->vao);
    glBindVertexArray(glr->vao);

    glGenBuffers(1, &glr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, glr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glr->buffer), glr->buffer, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
}

void gl_render_vertex(Render* glr, Vec2f p, Vec2f uv) {
    assert(glr->buffer_count < BUFFER_SIZE);
    glr->buffer[glr->buffer_count++] = (Vertex) {
        .pos = p,
        .uv = uv,
    };
}

void gl_render_triangle(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3,
    Vec2f uv1, Vec2f uv2, Vec2f uv3) {
    gl_render_vertex(glr, p1, uv1);
    gl_render_vertex(glr, p2, uv2);
    gl_render_vertex(glr, p3, uv3);
}

void gl_render_quad(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4,
    Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4) {
    gl_render_triangle(glr, p1, p2, p3, uv1, uv2, uv3);
    gl_render_triangle(glr, p2, p3, p4, uv2, uv3, uv4);
}

void gl_render_rec(Render* glr, Vec2f p, Vec2f s) {
    Vec2f uv = vec2fs(0);
    gl_render_quad(
        glr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        uv, uv, uv, uv);
}

void gl_render_img(Render* glr, Vec2f p, Vec2f s, Vec2f uv1, Vec2f uv2) {
    gl_render_quad(
        glr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        uv1, vec2f(uv2.x, uv1.y), vec2f(uv1.x, uv2.y), uv2);
}

void gl_clear(Render* glr) {
    glr->buffer_count = 0;
}

void gl_sync(Render* glr) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * glr->buffer_count, glr->buffer);
}
