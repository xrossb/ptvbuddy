#include "routes_window.h"
#include "departures_window.h"
#include <string.h>

typedef struct {
    AppState* state;
    int stop_id;
    char stop_name[STOP_NAME_MAX];
    MenuLayer* menu_layer;
} RoutesWindow;

// Only one routes window is on the stack at a time; see main_window.c.
static RoutesWindow* active;

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) { return 1; }

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    RoutesWindow* data = context;

    int count;
    AppState_routes(data->state, data->stop_id, &count);
    return count;
}

static int16_t get_header_height(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void draw_header(
    GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* context
) {
    RoutesWindow* data = context;
    menu_cell_basic_header_draw(ctx, cell_layer, data->stop_name);
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    RoutesWindow* data = context;

    int count;
    const Route* routes = AppState_routes(data->state, data->stop_id, &count);
    const Route* route = &routes[cell_index->row];

    menu_cell_basic_draw(
        ctx, cell_layer, route->display_name, NULL, AppState_icon(data->state, route->route_type)
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

static void select_click(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    RoutesWindow* data = context;

    int count;
    const Route* routes = AppState_routes(data->state, data->stop_id, &count);
    const Route* route = &routes[cell_index->row];

    window_stack_push(
        DeparturesWindow_create(data->state, route->stop_id, route->route_id, route->display_name),
        true
    );
}

static void reload(RoutesWindow* data) {
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
    RoutesWindow* data = window_get_user_data(window);
    Layer* root = window_get_root_layer(window);

    MenuLayerCallbacks callbacks = {
        .get_num_sections = get_num_sections,
        .get_num_rows = get_num_rows,
        .get_cell_height = get_cell_height,
        .get_header_height = get_header_height,
        .draw_row = draw_row,
        .draw_header = draw_header,
        .select_click = select_click,
    };

    MenuLayer* menu_layer = menu_layer_create(layer_get_frame(root));
    menu_layer_set_callbacks(menu_layer, data, callbacks);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(root, menu_layer_get_layer(menu_layer));
    data->menu_layer = menu_layer;

    active = data;
    AppState_subscribe(data->state, on_state_change);
    AppState_request_routes(data->state, data->stop_id);
}

static void unload(Window* window) {
    RoutesWindow* data = window_get_user_data(window);

    AppState_unsubscribe(data->state, on_state_change);
    if (active == data) {
        active = NULL;
    }

    menu_layer_destroy(data->menu_layer);
    free(data);

    window_destroy(window);
}

Window* RoutesWindow_create(AppState* state, int stop_id, const char* stop_name) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .load = load,
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    RoutesWindow* data = malloc(sizeof(RoutesWindow));
    *data = (RoutesWindow){
        .state = state,
        .stop_id = stop_id,
    };
    strncpy(data->stop_name, stop_name, STOP_NAME_MAX - 1);
    data->stop_name[STOP_NAME_MAX - 1] = '\0';
    window_set_user_data(window, data);

    return window;
}