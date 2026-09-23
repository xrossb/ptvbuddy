#include "routes_window.h"

typedef struct {
} RoutesWindow;

static void unload(Window* window) {
    RoutesWindow* data = window_get_user_data(window);
    free(data);

    window_destroy(window);
}

Window* RoutesWindow_create(int stop_id) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    RoutesWindow* data = malloc(sizeof(RoutesWindow));
    window_set_user_data(window, data);

    return window;
}
