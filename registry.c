#include <string.h>
#include "./state.h"

extern struct wl_seat_listener seat_listener;
extern struct wl_output_listener output_listener;

void registry_global_add(void *data, struct wl_registry *registry, uint32_t name,
                         const char *interface, uint32_t version) {
    client_state* state = data;

    printf("iface: '%s', ver: %d, name: %d\n", interface, version, name);

    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
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
    else if (!strcmp(interface, zwlr_screencopy_manager_v1_interface.name)) {
        state->screencopy_manager = wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, 3);
    }
    else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
        state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
    }
    else {
    }
}

void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
}

struct wl_registry_listener registry_listener = {
    .global = registry_global_add,
    .global_remove = registry_global_remove,
};
