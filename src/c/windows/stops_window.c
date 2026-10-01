#include "stops_window.h"
#include "routes_window.h"

typedef struct {
    AppState* state;
    MenuLayer* menu_layer;
} StopsWindow;

// Only one stops window is on the stack at a time; see main_window.c.
static StopsWindow* active;

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) { return 1; }

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    StopsWindow* data = context;

    int count;
    AppState_stops(data->state, &count);
    return count;
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    StopsWindow* data = context;

    int count;
    const Stop* stops = AppState_stops(data->state, &count);

    char subtitle[16];
    snprintf(subtitle, sizeof(subtitle), "%dm", stops[cell_index->row].distance_m);

    menu_cell_basic_draw(ctx, cell_layer, stops[cell_index->row].name, subtitle, NULL);
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

static void select_click(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    StopsWindow* data = context;

    int count;
    const Stop* stops = AppState_stops(data->state, &count);
    const Stop* stop = &stops[cell_index->row];

    window_stack_push(RoutesWindow_create(data->state, stop->stop_id, stop->name), true);
}

static void reload(StopsWindow* data) {
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
    StopsWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    MenuLayerCallbacks callbacks = {
        .get_num_sections = get_num_sections,
        .get_num_rows = get_num_rows,
        .get_cell_height = get_cell_height,
        .draw_row = draw_row,
        .select_click = select_click,
    };

    MenuLayer* menu_layer = menu_layer_create(layer_get_frame(root));
    menu_layer_set_callbacks(menu_layer, data, callbacks);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(root, menu_layer_get_layer(menu_layer));
    data->menu_layer = menu_layer;

    active = data;
    AppState_subscribe(data->state, on_state_change);
    AppState_request_stops(data->state);
}

static void unload(Window* window) {
    StopsWindow* data = window_get_user_data(window);

    AppState_unsubscribe(data->state, on_state_change);
    if (active == data) {
        active = NULL;
    }

    menu_layer_destroy(data->menu_layer);
    free(data);

    window_destroy(window);
}

Window* StopsWindow_create(AppState* state) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    StopsWindow* data = malloc(sizeof(StopsWindow));
    *data = (StopsWindow){
        .state = state,
    };
    window_set_user_data(window, data);

    return window;
}