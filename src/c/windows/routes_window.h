#pragma once

#include "../app_state.h"
#include <pebble.h>

Window* RoutesWindow_create(AppState* state, int stop_id, const char* stop_name);
