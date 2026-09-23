#include "main_window.h"
#include "../array.h"
#include "departures_window.h"
#include "stops_window.h"

typedef struct {
    Settings* settings;

    GBitmap* location_bitmap;
    MenuLayer* menu_layer;
} MainWindow;

static uint16_t get_num_rows(MenuLayer* menu_layer, uint16_t section_index, void* context) {
    MainWindow* data = context;
    return 1 + arrlen(data->settings->favourite_routes);
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    MainWindow* data = context;

    if (cell_index->row == 0) {
        menu_cell_basic_draw(ctx, cell_layer, "Find nearby", NULL, data->location_bitmap);
        return;
    }

    Route* route = &data->settings->favourite_routes[cell_index->row - 1];
    menu_cell_basic_draw(ctx, cell_layer, route->display_name, "in 10m", NULL);
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    return 50;
}

static void select_click(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    MainWindow* data = context;

    Window* new_window;
    if (cell_index->row == 0) {
        new_window = StopsWindow_create();
    } else {
        int index = cell_index->row - 1;
        Route* route = &data->settings->favourite_routes[index];
        new_window = DeparturesWindow_create(route->stop_id, route->route_id);
    }

    window_stack_push(new_window, true);
}

static void unload(Window* window) {
    MainWindow* data = window_get_user_data(window);
    gbitmap_destroy(data->location_bitmap);
    menu_layer_destroy(data->menu_layer);
    free(data);

    window_destroy(window);
}

Window* MainWindow_create(Settings* settings) {
    Window* window = window_create();

    WindowHandlers handlers = {
        .unload = unload,
    };
    window_set_window_handlers(window, handlers);

    MainWindow* data = malloc(sizeof(MainWindow));
    *data = (MainWindow){
        .settings = settings,
    };
    window_set_user_data(window, data);

    GBitmap* location_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOCATION);
    data->location_bitmap = location_bitmap;

    Layer* root = window_get_root_layer(window);

    MenuLayer* menu_layer = menu_layer_create(layer_get_frame(root));
    data->menu_layer = menu_layer;
    MenuLayerCallbacks callbacks = {
        .get_num_rows = get_num_rows,
        .draw_row = draw_row,
        .get_cell_height = get_cell_height,
        .select_click = select_click,
    };
    menu_layer_set_callbacks(menu_layer, data, callbacks);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(root, menu_layer_get_layer(menu_layer));

    return window;
}
