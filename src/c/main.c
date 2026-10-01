#include "app_state.h"
#include "windows/splash_window.h"
#include <pebble.h>

int main(void) {
    AppState* state = AppState_create();
    Window* splash_window = SplashWindow_create(state);

    window_stack_push(splash_window, true);
    app_event_loop();

    return 0;
}