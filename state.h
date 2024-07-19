#ifndef STATE_H_
#define STATE_H_
#include <stdbool.h>
#include <stdint.h>
#include "./la.h"

typedef struct {
    struct wl_display* dpy;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct xdg_wm_base* xdg_base;
    struct xdg_toplevel* toplevel;
    struct xdg_surface* xdg_surf;
    struct wl_egl_window* ewindow;
    struct wl_shm* shm;
    struct wl_output* output;

    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    struct wl_pointer* pointer;

    struct wl_surface* cursor_surface;
    struct wl_cursor_image* cursor_image;
    struct wl_cursor_theme* cursor_theme;
    struct wl_buffer* cursor_buffer;
    struct wl_cursor* cursor;

    struct zxdg_output_manager_v1* xdg_output_manager;
    struct zwlr_screencopy_manager_v1* screencopy_manager;
    struct zwlr_screencopy_frame_v1* screencopy_frame;
    struct wl_buffer* screen_buffer;
    struct wl_shm_pool *pool;
    bool buffer_done;
    uint8_t* screen_data;
    uint32_t screen_format;
    uint32_t screen_width, screen_height;

    struct xkb_context* xkbctx;
    struct xkb_keymap* xkbkeymap;
    struct xkb_state* xkbstate;

    uint32_t width, height;
    EGLDisplay edpy;
    EGLSurface esurface;
    EGLConfig econfig;
    EGLContext econtext;
    bool running;
    bool focused;

    float scale;
    float fl_radius;
    float fl_shadow;
    bool fl_enabled;
    double dt;
    uint8_t button_left;
    Vec2f camera;
    Vec2f delta;
    Vec2f mouse_cur;
    Vec2f mouse_prev;
} client_state;

client_state state = {0};
#endif // STATE_H_
