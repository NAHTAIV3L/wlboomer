#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "./glad/glad.h"
#include "./state.h"
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

#ifndef SHADER_PATH
#define SHADER_PATH "./"
#endif

extern struct zwlr_screencopy_frame_v1_listener screencopy_listener;

extern struct wl_output_listener output_listener;

extern struct zwlr_layer_surface_v1_listener layer_surface_listener;

extern struct wl_keyboard_listener keyboard_listener;

extern struct wl_pointer_listener pointer_listener;

extern struct wl_seat_listener seat_listener;

extern struct wl_registry_listener registry_listener;

char *read_file(const char *file)
{
    char* buffer = NULL;
    FILE *f = fopen(file, "r");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s", file);
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

GLuint compile_shader(const char* vert_file, const char* frag_file) {
    char *vert_code = read_file(vert_file);
    char *frag_code = read_file(frag_file);

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
        fprintf(stderr, "ERROR: Failed to Compile %s shader file: %s\n", vert_file, infoLog);
        exit(1);
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const GLchar* const*)&frag_code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        fprintf(stderr, "ERROR: Failed to Compile %s shader file: %s\n", frag_file, infoLog);
        exit(1);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        fprintf(stderr, "Shader Link Failed\n");
        exit(1);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    free(vert_code);
    free(frag_code);
    return program;
}

int main() {
    client_state state = {0};

    state.buffer_done = false;
    state.running = true;
    state.focused = true;
    state.scale = 1.0;
    state.width = 400;
    state.height = 400;
    state.fl_radius = 200.0;
    state.fl_enabled = false;

    state.dpy = wl_display_connect(NULL);
    if (!state.dpy) {
        fprintf(stderr ,"Unable to create Wayland display.\n");
        return 1;
    }
    state.registry = wl_display_get_registry(state.dpy);
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(state.dpy);

    if (!state.screencopy_manager){
        fprintf(stderr, "screencopy_manager interface not found\n");
        exit(1);
    }

    state.screencopy_frame = zwlr_screencopy_manager_v1_capture_output(
        state.screencopy_manager, 0, state.output);
    zwlr_screencopy_frame_v1_add_listener(state.screencopy_frame, &screencopy_listener, &state);
    wl_display_roundtrip(state.dpy);

    while (!state.buffer_done)
        wl_display_dispatch_pending(state.dpy);
    zwlr_screencopy_frame_v1_copy(state.screencopy_frame, state.screen_buffer);

    state.surface = wl_compositor_create_surface(state.compositor);

    state.layer_surface = zwlr_layer_shell_v1_get_layer_surface(state.layer_shell, state.surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wlboomer");
    zwlr_layer_surface_v1_add_listener(state.layer_surface, &layer_surface_listener, &state);
    zwlr_layer_surface_v1_set_size(state.layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_anchor(state.layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(state.layer_surface, -1);
    zwlr_layer_surface_v1_set_margin(state.layer_surface, 0, 0, 0, 0);
    zwlr_layer_surface_v1_set_keyboard_interactivity(state.layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    wl_surface_commit(state.surface);
    wl_display_roundtrip(state.dpy);

    EGLint attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    state.ewindow = wl_egl_window_create(state.surface, state.width, state.height);
    if (!state.ewindow) {
        fprintf(stderr, "ewindow\n");
        exit(1);
    }


    state.edpy = eglGetDisplay(state.dpy);
    if (state.edpy == EGL_NO_DISPLAY)  {
        fprintf(stderr, "edpy\n");
        exit(1);
    }
    {
        EGLint major, minor;
        if (eglInitialize(state.edpy, &major, &minor) != EGL_TRUE) {
            fprintf(stderr, "eglinit\n");
            exit(1);
        }

        printf("EGL version %u.%u\n", major, minor);
    }
    eglBindAPI(EGL_OPENGL_API);

    EGLint num_configs;
    if (eglChooseConfig(state.edpy, attrs, &state.econfig, 1, &num_configs) != EGL_TRUE) {
        fprintf(stderr, "econfig: %d\n", eglGetError());
        exit(1);
    }

    state.esurface = eglCreateWindowSurface(state.edpy, state.econfig,
                                            (EGLNativeWindowType)state.ewindow, NULL);
    if (state.esurface == EGL_NO_SURFACE) {
        fprintf(stderr, "esurface\n");
        exit(1);
    }


    state.econtext = eglCreateContext(state.edpy, state.econfig, EGL_NO_CONTEXT, NULL);
    if (state.econtext == EGL_NO_CONTEXT) {
        fprintf(stderr, "econtext: %x\n", eglGetError());
        exit(1);
    }

    eglMakeCurrent(state.edpy, state.esurface, state.esurface, state.econtext);

    if (!gladLoadGL()) {
        fprintf(stderr, "gladLoadGL");
        exit(1);
    }

    {
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        printf("GL version %u.%u\n", major, minor);
    }


    float verticies[(4 * 4)] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
    };

    GLuint vertex_array;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (GLvoid*)(sizeof(float) * 2));

    GLuint indices[3 * 2] = {
        0, 1, 2,
        2, 1, 3,
    };

    GLuint element_buffer;
    glGenBuffers(1, &element_buffer);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 state.screen_width,
                 state.screen_height,
                 0, GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 state.screen_data);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint program = compile_shader(SHADER_PATH"/main.vert", SHADER_PATH"/main.frag");
    glUseProgram(program);
    GLint scale_uniform = glGetUniformLocation(program, "scale");
    GLint camera_uniform = glGetUniformLocation(program, "camera");
    GLint res_uniform = glGetUniformLocation(program, "res");
    GLint cursor_uniform = glGetUniformLocation(program, "in_cursor");
    GLint fl_radius_uniform = glGetUniformLocation(program, "fl_radius");
    GLint fl_shadow_uniform = glGetUniformLocation(program, "fl_shadow");
    GLint fl_snazzy_uniform = glGetUniformLocation(program, "fl_snazzy");

    struct timeval tvstart, tvend, tvelapsed;
    gettimeofday(&tvstart, NULL);
    gettimeofday(&tvend, NULL);
    timersub(&tvend, &tvstart, &tvelapsed);

    while(state.running) {
        state.dt = (double)tvelapsed.tv_sec + (double)tvelapsed.tv_usec/1000000.0f;
        gettimeofday(&tvstart, NULL);

        wl_display_dispatch(state.dpy);
        if (state.focused) {

            if (state.fl_enabled)
                state.fl_shadow = MIN(state.fl_shadow + (6.0 * state.dt), 0.8);
            else
                state.fl_shadow = MAX(state.fl_shadow - (6.0 * state.dt), 0.0);

            glUseProgram(program);
            glUniform1f(scale_uniform, state.scale);
            glUniform2f(camera_uniform, state.camera.x, state.camera.y);
            glUniform2f(res_uniform, state.width, state.height);
            glUniform2f(cursor_uniform, state.mouse_cur.x, state.mouse_cur.y);

            glUniform1f(fl_radius_uniform, state.fl_radius);
            glUniform1f(fl_shadow_uniform, state.fl_shadow);
            glUniform1i(fl_snazzy_uniform, state.fl_snazzy);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            eglSwapBuffers(state.edpy, state.esurface);

        }
        gettimeofday(&tvend, NULL);
        timersub(&tvend, &tvstart, &tvelapsed);
    }

    eglTerminate(state.edpy);
    wl_display_disconnect(state.dpy);
    return 0;
}
