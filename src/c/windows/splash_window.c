#include "splash_window.h"
#include "../array.h"
#include "../settings.h"
#include "main_window.h"

typedef struct {
    Settings* settings;

    TextLayer* text_layer;
} SplashWindow;

static void on_timer(void* context) {
    Window* window = context;
    SplashWindow* data = window_get_user_data(window);
    Window* main_window = MainWindow_create(data->settings);

    window_stack_push(main_window, true);
    window_stack_remove(window, false);
}

static void load(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    data->text_layer = text_layer_create(layer_get_frame(root));
    text_layer_set_text(data->text_layer, "hello world");
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentCenter);
    layer_add_child(root, text_layer_get_layer(data->text_layer));

    // TODO:
    // 1. request settings
    // 2. listen for settings payload
    // 3. populate data->settings
    // 4. goto main window

    // dummy for now
    arrpush(
        data->settings->favourite_routes,
        ((Route){
            .route_type = ROUTE_TYPE_BUS,
            .stop_id = 42,
            .route_id = 24,
            .display_name = "Some Rd/Other St",
        })
    );

    app_timer_register(500, on_timer, window);
}

static void unload(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    text_layer_destroy(data->text_layer);
    free(data);

    window_destroy(window);
}

Window* SplashWindow_create(Settings* settings) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    SplashWindow* data = malloc(sizeof(SplashWindow));
    *data = (SplashWindow){
        .settings = settings,
    };
    window_set_user_data(window, data);

    return window;
}
