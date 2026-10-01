#include "splash_window.h"
#include "main_window.h"

#define SETTINGS_TIMEOUT_MS 3000

typedef struct {
    AppState* state;
    TextLayer* text_layer;
    AppTimer* timeout;
    bool advanced;
} SplashWindow;

// Only one splash window exists at a time, and AppState handlers take no
// per-window context, so the active window is tracked here.
static Window* active;

static void start_main_window(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    if (data->advanced) {
        return;
    }
    data->advanced = true;

    window_stack_push(MainWindow_create(data->state), true);
    window_stack_remove(window, false);
}

static void on_state_change(AppState* state) {
    if (active && AppState_settings_loaded(state)) {
        start_main_window(active);
    }
}

static void on_settings_timeout(void* state) { AppState_mark_phone_unreachable(state); }

static void load(Window* window) {
    SplashWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    data->text_layer = text_layer_create(layer_get_frame(root));
    text_layer_set_text(data->text_layer, "Connecting to phone...");
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentCenter);
    layer_add_child(root, text_layer_get_layer(data->text_layer));

    active = window;
    AppState_subscribe(data->state, on_state_change);
    AppState_request_settings(data->state);

    // If the phone never replies, fall back to the offline provider rather than
    // hanging here forever.
    data->timeout = app_timer_register(SETTINGS_TIMEOUT_MS, on_settings_timeout, data->state);
}

static void unload(Window* window) {
    SplashWindow* data = window_get_user_data(window);

    app_timer_cancel(data->timeout);
    AppState_unsubscribe(data->state, on_state_change);
    if (active == window) {
        active = NULL;
    }

    text_layer_destroy(data->text_layer);
    free(data);

    window_destroy(window);
}

Window* SplashWindow_create(AppState* state) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    SplashWindow* data = malloc(sizeof(SplashWindow));
    *data = (SplashWindow){
        .state = state,
    };
    window_set_user_data(window, data);

    return window;
}