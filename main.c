#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "./glad/glad.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include "./xdg-shell-protocol.h"
#include "./zxdg-output-v1.h"
#include "./zwlr-screencopy-v1.h"
#include "./shm.h"
#include "./state.h"
#include "./render.h"
#include "./shader.h"
#include "./utils.h"

#define MIN_SCALE 0.1
#define MAX_SCALE 10

/******************************/
/*****wlr_screencopy_frame*****/
/******************************/

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

/*******************/
/*****wl_output*****/
/*******************/

void output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
    int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make,
    const char *model, int32_t transform) {
}

void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
    int32_t width, int32_t height, int32_t refresh) {
}

void output_done(void *data, struct wl_output *wl_output) {
}

void output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
};

/*******************/
/****xdg_wm_base****/
/*******************/

void wm_base_ping(void *data, struct xdg_wm_base *xbase, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(xbase, serial);
}

struct xdg_wm_base_listener wm_base_listener = { .ping = wm_base_ping };

/*******************/
/****xdg_surface****/
/*******************/

void surface_configure(void *data, struct xdg_surface *xsurface, uint32_t serial) {
    (void)data;
    xdg_surface_ack_configure(xsurface, serial);
}

struct xdg_surface_listener surface_listener = { .configure = surface_configure };

/*******************/
/****xdg_toplevel***/
/*******************/

void toplevel_configure(void *data, struct xdg_toplevel *xtoplevel,
    int32_t width, int32_t height, struct wl_array *states) {
    client_state* state = data;

    if (!width && !height) return;

    if (state->width != (uint32_t)width || state->height != (uint32_t)height) {
        state->width = width;
        state->height = height;

        wl_egl_window_resize(state->ewindow, width, height, 0, 0);
        wl_surface_commit(state->surface);
        glViewport(0, 0, width, height);
    }
}

void toplevel_close(void *data, struct xdg_toplevel *xtoplevel) {
    (void)xtoplevel;
    client_state* state = data;
    state->running = false;
}

struct xdg_toplevel_listener toplevel_listener = {
    .configure = toplevel_configure,
    .close = toplevel_close,
};

/*******************/
/****wl_keyboard****/
/*******************/

void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
    uint32_t format, int32_t fd, uint32_t size) {
    (void)keyboard;
    client_state* state = data;

    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char* map_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(map_shm != MAP_FAILED && "map_shm failed");

    state->xkbctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(state->xkbctx && "Failed to create xkb context");

    state->xkbkeymap = xkb_keymap_new_from_string(state->xkbctx, map_shm,
                                        XKB_KEYMAP_FORMAT_TEXT_V1,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_shm, size);
    close(fd);
    state->xkbstate = xkb_state_new(state->xkbkeymap);
    assert(state && "Failed to create state");
}

void keyboard_enter(void *data, struct wl_keyboard *keyboard,
    uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
}

void keyboard_leave(void *data, struct wl_keyboard *keyboard,
    uint32_t serial, struct wl_surface *surface) {
}

void keyboard_key(void *data, struct wl_keyboard *keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t kstate) {
    if (kstate == 0) return;
    client_state* state = data;

    xkb_keysym_t keysym = xkb_state_key_get_one_sym(state->xkbstate, key + 8);
    if (keysym == XKB_KEY_q) {
        state->running = false;
    }
    if (keysym == XKB_KEY_f) {
        state->fl_enabled = !state->fl_enabled;
    }
}

void keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
    uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {

    client_state* state = data;
    xkb_state_update_mask(state->xkbstate,
        mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,
    int32_t rate, int32_t delay) {
}

struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info,
};

/*******************/
/*****wl_pointer****/
/*******************/

void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
struct wl_surface *surface, wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
    client_state* state = data;
    int surface_x = wl_fixed_to_int(fixed_surface_x);
    int surface_y = wl_fixed_to_int(fixed_surface_y);
    state->mouse_cur.x = surface_x;
    state->mouse_cur.y = surface_y;
    state->mouse_prev.x = state->mouse_cur.x;
    state->mouse_prev.y = state->mouse_cur.y;
    state->focused = true;
    wl_pointer_set_cursor(pointer, serial, state->cursor_surface, 0, 0);
}

void pointer_leave(void *data, struct wl_pointer *pointer,
    uint32_t serial, struct wl_surface *surface) {
    client_state* state = data;
    state->focused = false;
}

void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time,
    wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
    client_state* state = data;
    int surface_x = wl_fixed_to_int(fixed_surface_x);
    int surface_y = wl_fixed_to_int(fixed_surface_y);

    state->mouse_cur.x = surface_x;
    state->mouse_cur.y = surface_y;
    if (state->button_left == WL_POINTER_BUTTON_STATE_PRESSED) {
        state->delta = vec2f_sub(state->mouse_prev, state->mouse_cur);
        state->camera.x += state->delta.x / state->scale;
        state->camera.y += state->delta.y / state->scale;
    }
    state->mouse_prev.x = state->mouse_cur.x;
    state->mouse_prev.y = state->mouse_cur.y;
}

void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t button_state) {
    client_state* state = data;
    if (button == BTN_LEFT) {
        state->button_left = button_state;
    }
}

void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time,
uint32_t axis, wl_fixed_t fixed_value) {
    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) return;

    client_state* state = data;
    int value = wl_fixed_to_int(fixed_value);
    float delta = -value;
    if (xkb_state_mod_name_is_active(state->xkbstate, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0){
        state->fl_radius = CLAMP(state->fl_radius - (delta / state->scale), 0, 1000);
        return;
    }

    Vec2f old_scale = vec2fs(CLAMP(state->scale, MIN_SCALE, MAX_SCALE));
    Vec2f new_scale = vec2fs(CLAMP(state->scale + (delta * state->dt), MIN_SCALE, MAX_SCALE));
    Vec2f half_res = vec2f(state->width * 0.5f, state->height * 0.5f);

    Vec2f p0 = vec2f_div(vec2f_sub(state->mouse_cur, half_res), old_scale);
    Vec2f p1 = vec2f_div(vec2f_sub(state->mouse_cur, half_res), new_scale);

    state->camera.x += p0.x - p1.x;
    state->camera.y += p0.y - p1.y;
    state->scale = new_scale.x;
}

struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis
};

/*******************/
/******wl_seat******/
/*******************/

void seat_capabilities(void *data, struct wl_seat *seat, uint32_t cap) {
    client_state* state = data;

    if (cap & WL_SEAT_CAPABILITY_KEYBOARD && !state->keyboard) {
        state->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
    }
    if (cap & WL_SEAT_CAPABILITY_POINTER && !state->pointer) {
        state->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(state->pointer, &pointer_listener, state);
        state->cursor_theme = wl_cursor_theme_load("Adwaita", 24, state->shm);
        state->cursor = wl_cursor_theme_get_cursor(state->cursor_theme, "left_ptr");
        state->cursor_image = state->cursor->images[0];
        state->cursor_buffer = wl_cursor_image_get_buffer(state->cursor_image);
        state->cursor_surface = wl_compositor_create_surface(state->compositor);
        wl_surface_attach(state->cursor_surface, state->cursor_buffer, 0, 0);
        wl_surface_commit(state->cursor_surface);
    }

}

void seat_name(void *data, struct wl_seat *seat, const char *name) {
}

struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

/*******************/
/****wl_registry****/
/*******************/

void registry_global_add(void *data, struct wl_registry *registry, uint32_t name,
    const char *interface, uint32_t version) {
    (void)version;

    client_state* state = data;
    // printf("iface: '%s', ver: %d, name: %d\n", interface, version, name);

    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    }
    else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        state->xdg_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_base, &wm_base_listener, NULL);
    }
    else if (!strcmp(interface, wl_seat_interface.name)) {
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(state->seat, &seat_listener, state);
    }
    else if (!strcmp(interface, wl_shm_interface.name)) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (!strcmp(interface, wl_output_interface.name)) {
        state->output = wl_registry_bind(registry, name, &wl_output_interface, 3);
        wl_output_add_listener(state->output, &output_listener, state);
    }
    else if (!strcmp(interface, zxdg_output_manager_v1_interface.name)) {
        state->xdg_output_manager = wl_registry_bind(registry, name,
            &zxdg_output_manager_v1_interface, 1);
    }
    else if (!strcmp(interface, zwlr_screencopy_manager_v1_interface.name)) {
        state->screencopy_manager = wl_registry_bind(registry, name,
        &zwlr_screencopy_manager_v1_interface, 3);
    }
    else {
    }
}

void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
}

struct wl_registry_listener listener = {
    .global = registry_global_add,
    .global_remove = registry_global_remove,
};

/*******************/
/*******main********/
/*******************/

int main() {
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
    wl_registry_add_listener(state.registry, &listener, &state);
    wl_display_roundtrip(state.dpy);

    state.screencopy_frame = zwlr_screencopy_manager_v1_capture_output(
        state.screencopy_manager, 0, state.output);
    zwlr_screencopy_frame_v1_add_listener(state.screencopy_frame, &screencopy_listener, &state);
    wl_display_roundtrip(state.dpy);

    while (!state.buffer_done)
        wl_display_dispatch_pending(state.dpy);
    zwlr_screencopy_frame_v1_copy(state.screencopy_frame, state.screen_buffer);

    state.surface = wl_compositor_create_surface(state.compositor);

    state.xdg_surf = xdg_wm_base_get_xdg_surface(state.xdg_base, state.surface);
    xdg_surface_add_listener(state.xdg_surf, &surface_listener, NULL);

    state.toplevel = xdg_surface_get_toplevel(state.xdg_surf);
    xdg_toplevel_add_listener(state.toplevel, &toplevel_listener, &state);
    xdg_toplevel_set_title(state.toplevel, "wlscreenshot");
    xdg_toplevel_set_fullscreen(state.toplevel, state.output);
    wl_surface_commit(state.surface);

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
    if (!state.ewindow)
        die("ewindow\n");


    state.edpy = eglGetDisplay(state.dpy);
    if (state.edpy == EGL_NO_DISPLAY)
        die("edpy\n");

    {
        EGLint major, minor;
        if (eglInitialize(state.edpy, &major, &minor) != EGL_TRUE)
            die("eglinit\n");

        printf("EGL version %u.%u\n", major, minor);
    }
    eglBindAPI(EGL_OPENGL_API);

    EGLint num_configs;
    if (eglChooseConfig(state.edpy, attrs, &state.econfig, 1, &num_configs) != EGL_TRUE)
        die("econfig: %d\n", eglGetError());

    state.esurface = eglCreateWindowSurface(state.edpy, state.econfig,
        (EGLNativeWindowType)state.ewindow, NULL);
    if (state.esurface == EGL_NO_SURFACE)
        die("esurface\n");


    state.econtext = eglCreateContext(state.edpy, state.econfig, EGL_NO_CONTEXT, NULL);
    if (state.econtext == EGL_NO_CONTEXT)
        die("econtext: %x\n", eglGetError());

    eglMakeCurrent(state.edpy, state.esurface, state.esurface, state.econtext);

    if (!gladLoadGL())
        die("gladLoadGL");

    {
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        printf("GL version %u.%u\n", major, minor);
    }


    Render glr = {0};
    gl_render_init(&glr, "./main.vert", "./main.frag");

    GLuint texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        state.screen_width,
        state.screen_height,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        state.screen_data);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    Shader* shdr = NULL;
    shader_create(&shdr, "test", "./main.vert", "./main.frag");
    shader_use(shdr, "test");
    GLint scale_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "scale");
    GLint camera_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "camera");
    GLint res_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "res");
    GLint cursor_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "in_cursor");
    GLint fl_radius_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "fl_radius");
    GLint fl_shadow_uniform = glGetUniformLocation(shader_get_program(shdr, "test"), "fl_shadow");

    struct timeval tvstart, tvend, tvelapsed;
    gettimeofday(&tvstart, NULL);
    gettimeofday(&tvend, NULL);
    timersub(&tvend, &tvstart, &tvelapsed);

    while(state.running) {
        state.dt = (double)tvelapsed.tv_sec + (double)tvelapsed.tv_usec/1000000.0f;
        gettimeofday(&tvstart, NULL);


        wl_display_dispatch_pending(state.dpy);

        if (state.fl_enabled)
            state.fl_shadow = MIN(state.fl_shadow + (6.0 * state.dt), 0.8);
        else
            state.fl_shadow = MAX(state.fl_shadow - (6.0 * state.dt), 0.0);

        glUniform1f(scale_uniform, state.scale);
        glUniform2f(camera_uniform, state.camera.x, state.camera.y);
        glUniform2f(res_uniform, state.width, state.height);
        glUniform2f(cursor_uniform, state.mouse_cur.x, state.mouse_cur.y);

        glUniform1f(fl_radius_uniform, state.fl_radius);
        glUniform1f(fl_shadow_uniform, state.fl_shadow);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl_clear(&glr);
        gl_render_quad(&glr, vec2f(0.0, 0.0), vec2f(0.0, 1.0), vec2f(1.0, 0.0), vec2f(1.0, 1.0), vec2f(0.0, 1.0), vec2f(0.0, 0.0), vec2f(1.0, 1.0), vec2f(1.0, 0.0));
        gl_sync(&glr);
        shader_use(shdr, "test");
        glDrawArrays(GL_TRIANGLES, 0, glr.buffer_count);

        eglSwapBuffers(state.edpy, state.esurface);

        gettimeofday(&tvend, NULL);
        timersub(&tvend, &tvstart, &tvelapsed);
    }

    eglTerminate(state.edpy);
    wl_display_disconnect(state.dpy);
    return 0;
}

//wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
//wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
