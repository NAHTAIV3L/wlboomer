#include "./state.h"

void geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
                     int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make,
                     const char *model, int32_t transform) {
}

void mode(void *data, struct wl_output *wl_output, uint32_t flags,
                 int32_t width, int32_t height, int32_t refresh) {
}

void done(void *data, struct wl_output *wl_output) {
}

void scale(void *data, struct wl_output *wl_output, int32_t factor) {
}

struct wl_output_listener output_listener = {
    .geometry = geometry,
    .mode = mode,
    .done = done,
    .scale = scale,
};
