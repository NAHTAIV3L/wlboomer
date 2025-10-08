#include "./state.h"
#define CLAMP(x, min, max) (x > max ? max : (x < min ? min : x))


void enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                   struct wl_surface *surface, wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
    client_state* state = data;
    state->mouse_cur.x = wl_fixed_to_int(fixed_surface_x);
    state->mouse_cur.y = wl_fixed_to_int(fixed_surface_y);
    state->mouse_prev = state->mouse_cur;
    state->focused = true;
    wl_pointer_set_cursor(pointer, serial, state->cursor_surface, 0, 0);
}

void leave(void *data, struct wl_pointer *pointer,
                   uint32_t serial, struct wl_surface *surface) {
    client_state* state = data;
    state->focused = false;
}

void motion(void *data, struct wl_pointer *pointer, uint32_t time,
                    wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
    client_state* state = data;
    int surface_x = wl_fixed_to_int(fixed_surface_x);
    int surface_y = wl_fixed_to_int(fixed_surface_y);

    state->mouse_cur.x = surface_x;
    state->mouse_cur.y = surface_y;
    if (state->button_left == WL_POINTER_BUTTON_STATE_PRESSED) {
        state->delta = (vec2){
            .x = state->mouse_prev.x - state->mouse_cur.x,
            .y = state->mouse_prev.y - state->mouse_cur.y,
        };
        /*vec2f_sub(state->mouse_prev, state->mouse_cur)*/
        state->camera.x += state->delta.x / state->scale;
        state->camera.y += state->delta.y / state->scale;
    }
    state->mouse_prev.x = state->mouse_cur.x;
    state->mouse_prev.y = state->mouse_cur.y;
}

void button(void *data, struct wl_pointer *pointer, uint32_t serial,
                    uint32_t time, uint32_t button, uint32_t button_state) {
    client_state* state = data;
    if (button == BTN_LEFT) {
        state->button_left = button_state;
    }
}

void axis(void *data, struct wl_pointer *pointer, uint32_t time,
                  uint32_t axis, wl_fixed_t fixed_value) {
    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) return;

    client_state* state = data;
    int value = wl_fixed_to_int(fixed_value);
    float delta = -value;
    if (xkb_state_mod_name_is_active(state->xkbstate, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0){
        state->fl_radius = CLAMP(state->fl_radius - (delta / state->scale), 0, 1000);
        return;
    }

    float old_scale = CLAMP(state->scale, MIN_SCALE, MAX_SCALE);
    float new_scale = CLAMP(state->scale + (delta * old_scale * state->dt * 0.5f), MIN_SCALE, MAX_SCALE);

    // calculate new camera position
    vec2 half_res = {
        .x = state->width * 0.5f,
        .y = state->height * 0.5f
    };
    vec2 p0 = {
        .x = (state->mouse_cur.x - half_res.x) / old_scale,
        .y = (state->mouse_cur.y - half_res.y) / old_scale,
    };
    vec2 p1 = {
        .x = (state->mouse_cur.x - half_res.x) / new_scale,
        .y = (state->mouse_cur.y - half_res.y) / new_scale,
    };

    // update scale and camera
    state->camera.x += p0.x - p1.x;
    state->camera.y += p0.y - p1.y;
    state->scale = new_scale;
}

struct wl_pointer_listener pointer_listener = {
    .enter = enter,
    .leave = leave,
    .motion = motion,
    .button = button,
    .axis = axis
};

