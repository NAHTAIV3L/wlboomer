#include "./state.h"

extern struct wl_keyboard_listener keyboard_listener;
extern struct wl_pointer_listener pointer_listener;

void seat_capabilities(void *data, struct wl_seat *seat, uint32_t cap) {
    client_state* state = data;

    if (cap & WL_SEAT_CAPABILITY_KEYBOARD && !state->keyboard) {
        state->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
    }
    if (cap & WL_SEAT_CAPABILITY_POINTER && !state->pointer) {
        state->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(state->pointer, &pointer_listener, state);
        state->cursor_theme = wl_cursor_theme_load(NULL, 24, state->shm);
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
