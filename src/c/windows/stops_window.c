#include "stops_window.h"

typedef struct {
} StopsWindow;

static void unload(Window* window) {
    StopsWindow* data = window_get_user_data(window);
    free(data);

    window_destroy(window);
}

Window* StopsWindow_create(void) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    StopsWindow* data = malloc(sizeof(StopsWindow));
    window_set_user_data(window, data);

    return window;
}
