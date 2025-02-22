#include "./state.h"

void configure(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial, uint32_t width, uint32_t height) {
    client_state* state = data;
    state->width = width;
    state->height = height;
    state->mouse_cur.x = width / 2.0;
    state->mouse_cur.y = height / 2.0;
    state->mouse_prev = state->mouse_cur;
    zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
}

void closed(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1) {
    client_state* state = data;
    state->running = false;
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = configure,
    .closed = closed,
};
