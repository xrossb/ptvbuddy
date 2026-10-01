#include "main_window.h"
#include "../array.h"
#include "departures_window.h"
#include "stops_window.h"

#define SECTION_NEARBY 0
#define SECTION_FAVOURITES 1

typedef struct {
    AppState* state;
    MenuLayer* menu_layer;
} MainWindow;

// Only one home window exists at a time, and the Settings/AppState handlers take
// no per-window context, so the live window is tracked here.
static MainWindow* active;

static Route* favourite(MainWindow* data, uint16_t row) {
    Route* routes = data->state->settings->favourite_routes;
    return &routes[row];
}

static void format_eta(char* dest, size_t size, int eta_secs) {
    if (eta_secs < 60) {
        snprintf(dest, size, "Arriving now");
    } else {
        snprintf(dest, size, "%dm", (eta_secs + 30) / 60);
    }
}

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) { return 2; }

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    MainWindow* data = context;

    if (section_index == SECTION_NEARBY) {
        return 1;
    }
    return arrlen(data->state->settings->favourite_routes);
}

static int16_t get_header_height(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void draw_header(
    GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* context
) {
    if (section_index == SECTION_NEARBY) {
        menu_cell_basic_header_draw(ctx, cell_layer, "Nearby");
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "Favourites");
    }
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    MainWindow* data = context;

    if (cell_index->section == SECTION_NEARBY) {
        menu_cell_basic_draw(ctx, cell_layer, "Find nearby", NULL, data->state->location_bitmap);
        return;
    }

    Route* route = favourite(data, cell_index->row);

    char subtitle[16];
    const Departure* departure =
        AppState_next_departure(data->state, route->stop_id, route->route_id);
    if (departure) {
        format_eta(subtitle, sizeof(subtitle), departure->eta_secs);
    } else {
        subtitle[0] = '\0';
    }

    menu_cell_basic_draw(
        ctx, cell_layer, route->display_name, subtitle,
        AppState_icon(data->state, route->route_type)
    );
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

static void selection_changed(
    MenuLayer* menu_layer, MenuIndex new_index, MenuIndex old_index, void* context
) {
    MainWindow* data = context;

    if (new_index.section != SECTION_FAVOURITES) {
        return;
    }

    Route* route = favourite(data, new_index.row);
    AppState_request_departures(data->state, route->stop_id, route->route_id);
}

static void select_click(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    MainWindow* data = context;

    if (cell_index->section == SECTION_NEARBY) {
        window_stack_push(StopsWindow_create(data->state), true);
        return;
    }

    Route* route = favourite(data, cell_index->row);
    window_stack_push(
        DeparturesWindow_create(data->state, route->stop_id, route->route_id, route->display_name),
        true
    );
}

static void reload(MainWindow* data) {
    MenuLayer* menu_layer = data->menu_layer;
    MenuIndex selected = menu_layer_get_selected_index(menu_layer);

    menu_layer_reload_data(menu_layer);
    menu_layer_set_selected_index(menu_layer, selected, MenuRowAlignCenter, false);
}

static void on_settings_changed(Settings* settings) {
    if (active) {
        reload(active);
    }
}

static void on_state_change(AppState* state) {
    if (active) {
        reload(active);
    }
}

static void load(Window* window) {
    MainWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    MenuLayerCallbacks callbacks = {
        .get_num_sections = get_num_sections,
        .get_num_rows = get_num_rows,
        .get_cell_height = get_cell_height,
        .get_header_height = get_header_height,
        .draw_row = draw_row,
        .draw_header = draw_header,
        .select_click = select_click,
        .selection_changed = selection_changed,
    };

    MenuLayer* menu_layer = menu_layer_create(layer_get_frame(root));
    menu_layer_set_callbacks(menu_layer, data, callbacks);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(root, menu_layer_get_layer(menu_layer));
    data->menu_layer = menu_layer;

    active = data;
    Settings_subscribe(data->state->settings, on_settings_changed);
    AppState_subscribe(data->state, on_state_change);

    // Kick off a fetch for whatever the selection starts on.
    selection_changed(menu_layer, menu_layer_get_selected_index(menu_layer), (MenuIndex){0}, data);
}

static void unload(Window* window) {
    MainWindow* data = window_get_user_data(window);

    Settings_unsubscribe(data->state->settings, on_settings_changed);
    AppState_unsubscribe(data->state, on_state_change);
    if (active == data) {
        active = NULL;
    }

    menu_layer_destroy(data->menu_layer);
    free(data);

    window_destroy(window);
}

Window* MainWindow_create(AppState* state) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    MainWindow* data = malloc(sizeof(MainWindow));
    *data = (MainWindow){
        .state = state,
    };
    window_set_user_data(window, data);

    return window;
}