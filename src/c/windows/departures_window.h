#pragma once

#include "../app_state.h"
#include <pebble.h>

Window* DeparturesWindow_create(
    AppState* state, int stop_id, int route_id, const char* route_label
);
