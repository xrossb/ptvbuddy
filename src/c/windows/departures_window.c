#include "departures_window.h"
#include <string.h>

#define SECTION_REALTIME 0
#define SECTION_SCHEDULED 1

typedef struct {
    AppState* state;
    int stop_id;
    int route_id;
    char route_label[ROUTE_NAME_MAX];
    MenuLayer* menu_layer;
} DeparturesWindow;

// Only one departures window is on the stack at a time; see main_window.c.
static DeparturesWindow* active;

static void format_countdown(char* dest, size_t size, int eta_secs) {
    if (eta_secs < 60) {
        snprintf(dest, size, "Due");
    } else {
        snprintf(dest, size, "%dm", (eta_secs + 30) / 60);
    }
}

static uint16_t section_of(const Departure* departure) {
    return departure->realtime ? SECTION_REALTIME : SECTION_SCHEDULED;
}

static uint16_t section_count(const Departure* departures, int count, uint16_t section) {
    uint16_t total = 0;

    for (int i = 0; i < count; i++) {
        if (section_of(&departures[i]) == section) {
            total++;
        }
    }

    return total;
}

// Row index within a section -> index into the departures array.
static uint16_t row_to_index(
    const Departure* departures, int count, uint16_t section, uint16_t row
) {
    uint16_t seen = 0;

    for (int i = 0; i < count; i++) {
        if (section_of(&departures[i]) != section) {
            continue;
        }
        if (seen == row) {
            return i;
        }
        seen++;
    }

    return 0;
}

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) { return 2; }

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    DeparturesWindow* data = context;

    int count;
    const Departure* departures =
        AppState_departures(data->state, data->stop_id, data->route_id, &count);
    return section_count(departures, count, section_index);
}

static int16_t get_header_height(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    DeparturesWindow* data = context;

    if (section_index == SECTION_REALTIME) {
        return MENU_CELL_BASIC_HEADER_HEIGHT;
    }

    int count;
    const Departure* departures =
        AppState_departures(data->state, data->stop_id, data->route_id, &count);
    return section_count(departures, count, SECTION_SCHEDULED) ? MENU_CELL_BASIC_HEADER_HEIGHT : 0;
}

static void draw_header(
    GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* context
) {
    DeparturesWindow* data = context;

    if (section_index == SECTION_REALTIME) {
        menu_cell_basic_header_draw(ctx, cell_layer, data->route_label);
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "Scheduled");
    }
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    DeparturesWindow* data = context;

    int count;
    const Departure* departures =
        AppState_departures(data->state, data->stop_id, data->route_id, &count);
    if (count == 0) {
        return;
    }

    const Departure* departure =
        &departures[row_to_index(departures, count, cell_index->section, cell_index->row)];

    char subtitle[16];
    format_countdown(subtitle, sizeof(subtitle), departure->eta_secs);

    // Scheduled-only rows are greyed out.
    graphics_context_set_text_color(ctx, departure->realtime ? GColorBlack : GColorLightGray);
    menu_cell_basic_draw(ctx, cell_layer, departure->headsign, subtitle, NULL);
    graphics_context_set_text_color(ctx, GColorBlack);
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
#ifdef PBL_ROUND
    MenuIndex selected = menu_layer_get_selected_index(menu_layer);
    return menu_index_compare(&selected, cell_index) == 0
        ? MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT
        : MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;
#else
    return 50;
#endif
}

static void reload(DeparturesWindow* data) {
    MenuLayer* menu_layer = data->menu_layer;
    MenuIndex selected = menu_layer_get_selected_index(menu_layer);

    menu_layer_reload_data(menu_layer);
    menu_layer_set_selected_index(menu_layer, selected, MenuRowAlignCenter, false);
}

static void on_state_change(AppState* state) {
    if (active) {
        reload(active);
    }
}

static void load(Window* window) {
    DeparturesWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    MenuLayerCallbacks callbacks = {
        .get_num_sections = get_num_sections,
        .get_num_rows = get_num_rows,
        .get_cell_height = get_cell_height,
        .get_header_height = get_header_height,
        .draw_row = draw_row,
        .draw_header = draw_header,
    };

    MenuLayer* menu_layer = menu_layer_create(layer_get_frame(root));
    menu_layer_set_callbacks(menu_layer, data, callbacks);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(root, menu_layer_get_layer(menu_layer));
    data->menu_layer = menu_layer;

    active = data;
    AppState_subscribe(data->state, on_state_change);
    AppState_request_departures(data->state, data->stop_id, data->route_id);
}

static void unload(Window* window) {
    DeparturesWindow* data = window_get_user_data(window);

    AppState_unsubscribe(data->state, on_state_change);
    if (active == data) {
        active = NULL;
    }

    menu_layer_destroy(data->menu_layer);
    free(data);

    window_destroy(window);
}

Window* DeparturesWindow_create(
    AppState* state, int stop_id, int route_id, const char* route_label
) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    DeparturesWindow* data = malloc(sizeof(DeparturesWindow));
    *data = (DeparturesWindow){
        .state = state,
        .stop_id = stop_id,
        .route_id = route_id,
    };
    strncpy(data->route_label, route_label, ROUTE_NAME_MAX - 1);
    data->route_label[ROUTE_NAME_MAX - 1] = '\0';
    window_set_user_data(window, data);

    return window;
}