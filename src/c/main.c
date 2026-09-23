#include "settings.h"
#include "windows/splash_window.h"
#include <pebble.h>

int main(void) {
    Settings* settings = Settings_create();
    Window* splash_window = SplashWindow_create(settings);

    window_stack_push(splash_window, true);
    app_event_loop();

    Settings_destroy(settings);
}
