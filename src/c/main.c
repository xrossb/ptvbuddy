#include "windows/main_window.h"
#include <pebble.h>

static void init(void) { main_window_push(); }

static void deinit(void) {}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
