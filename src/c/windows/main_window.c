#include "main_window.h"

typedef struct {
} WindowData;

static void window_load(Window *window) {
    WindowData *data = malloc(sizeof(WindowData));
    window_set_user_data(window, data);
}

static void window_unload(Window *window) {
    WindowData *data = window_get_user_data(window);
    window_destroy(window);
    free(data);
}

void main_window_push(void) {
    Window *window = window_create();
    WindowHandlers handlers = {
        .load = window_load,
        .unload = window_unload,
    };

    window_set_window_handlers(window, handlers);
    window_stack_push(window, true);
}
