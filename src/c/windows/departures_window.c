#include "departures_window.h"

typedef struct {
} DeparturesWindow;

static void unload(Window* window) {
    DeparturesWindow* data = window_get_user_data(window);
    free(data);

    window_destroy(window);
}

Window* DeparturesWindow_create(int stop_id, int route_id) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    DeparturesWindow* data = malloc(sizeof(DeparturesWindow));
    window_set_user_data(window, data);

    return window;
}
