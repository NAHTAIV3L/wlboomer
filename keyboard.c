#include "./state.h"

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
    assert(state->xkbstate && "Failed to create state");
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
    if (keysym == XKB_KEY_s) {
        state->fl_snazzy = !state->fl_snazzy;
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
