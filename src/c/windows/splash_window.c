#include "splash_window.h"
#include "../array.h"
#include "../settings.h"
#include "main_window.h"

typedef struct {
    Settings* settings;

    TextLayer* text_layer;
} SplashWindow;

static void start_main_window(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    Window* main_window = MainWindow_create(data->settings);

    window_stack_push(main_window, true);
    window_stack_remove(window, false);
}

static void on_message(DictionaryIterator* iter, void* context) {
    app_message_deregister_callbacks();
    app_message_set_context(NULL);

    Window* window = context;
    SplashWindow* data = window_get_user_data(window);

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

    start_main_window(window);
}

static void request_settings(Window* window) {
    app_message_open(64, 64); // TODO: determine actual max message sizes, move to main.
    app_message_set_context(window);
    app_message_register_inbox_received(on_message);

    // TODO: send message to request settings
}

static void load(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    data->text_layer = text_layer_create(layer_get_frame(root));
    text_layer_set_text(data->text_layer, "hello world");
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentCenter);
    layer_add_child(root, text_layer_get_layer(data->text_layer));

    request_settings(window);

    // TODO:
    // 1. request settings
    // 2. listen for settings payload
    // 3. populate data->settings
    // 4. goto main window
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
