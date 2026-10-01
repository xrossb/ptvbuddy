#include "app_state.h"
#include "array.h"
#include <string.h>

// Upper bounds must match the sized messageKeys in package.json.
#define FAVOURITES_MAX 8
#define STOPS_MAX 12
#define ROUTES_MAX 16
#define DEPARTURES_MAX 12

static const uint32_t ICON_RESOURCES[ROUTE_TYPE_COUNT] = {
    RESOURCE_ID_IMAGE_ICON_TRAIN, RESOURCE_ID_IMAGE_ICON_TRAM,      RESOURCE_ID_IMAGE_ICON_BUS,
    RESOURCE_ID_IMAGE_ICON_VLINE, RESOURCE_ID_IMAGE_ICON_NIGHT_BUS,
};

static void notify_changed(AppState* state) {
    for (size_t i = 0; i < arrlen(state->subscribers); i++) {
        state->subscribers[i](state);
    }
}

void AppState_subscribe(AppState* state, AppStateHandler handler) {
    for (size_t i = 0; i < arrlen(state->subscribers); i++) {
        if (state->subscribers[i] == handler) {
            return;
        }
    }

    arrpush(state->subscribers, handler);
}

void AppState_unsubscribe(AppState* state, AppStateHandler handler) {
    for (size_t i = 0; i < arrlen(state->subscribers); i++) {
        if (state->subscribers[i] == handler) {
            state->subscribers[i] = state->subscribers[arrlen(state->subscribers) - 1];
            arrpop(state->subscribers);
            return;
        }
    }
}

GBitmap* AppState_icon(AppState* state, RouteType type) {
    if ((uint32_t)type >= ROUTE_TYPE_COUNT) {
        return NULL;
    }
    return state->icons[type];
}

/* --- cache lookup --- */

static StopRoutes* find_stop_routes(AppState* state, int stop_id) {
    for (size_t i = 0; i < arrlen(state->stop_routes); i++) {
        if (state->stop_routes[i].stop_id == stop_id) {
            return &state->stop_routes[i];
        }
    }
    return NULL;
}

static RouteDepartures* find_route_departures(AppState* state, int stop_id, int route_id) {
    for (size_t i = 0; i < arrlen(state->route_departures); i++) {
        RouteDepartures* entry = &state->route_departures[i];
        if (entry->stop_id == stop_id && entry->route_id == route_id) {
            return entry;
        }
    }
    return NULL;
}

/* --- wire decoding --- */

static bool read_int(DictionaryIterator* iter, uint32_t key, int32_t* out) {
    Tuple* tuple = dict_find(iter, key);
    if (!tuple) {
        return false;
    }

    if (tuple->type == TUPLE_INT) {
        switch (tuple->length) {
        case 1:
            *out = tuple->value->int8;
            return true;
        case 2:
            *out = tuple->value->int16;
            return true;
        default:
            *out = tuple->value->int32;
            return true;
        }
    }

    if (tuple->type == TUPLE_UINT) {
        switch (tuple->length) {
        case 1:
            *out = tuple->value->uint8;
            return true;
        case 2:
            *out = tuple->value->uint16;
            return true;
        default:
            *out = (int32_t)tuple->value->uint32;
            return true;
        }
    }

    return false;
}

static int32_t read_int_or(DictionaryIterator* iter, uint32_t key, int32_t fallback) {
    int32_t value;
    return read_int(iter, key, &value) ? value : fallback;
}

static int read_count(DictionaryIterator* iter, uint32_t key, int max) {
    int32_t count;
    if (!read_int(iter, key, &count) || count <= 0) {
        return 0;
    }
    return count > max ? max : (int)count;
}

static RouteType read_route_type(DictionaryIterator* iter, uint32_t key) {
    int32_t type = read_int_or(iter, key, ROUTE_TYPE_TRAIN);
    if ((uint32_t)type >= ROUTE_TYPE_COUNT) {
        return ROUTE_TYPE_TRAIN;
    }
    return (RouteType)type;
}

static void read_name(DictionaryIterator* iter, uint32_t key, char* dest, size_t size) {
    Tuple* tuple = dict_find(iter, key);
    dest[0] = '\0';
    if (!tuple || tuple->type != TUPLE_CSTRING) {
        return;
    }
    strncpy(dest, tuple->value->cstring, size - 1);
    dest[size - 1] = '\0';
}

/* --- responses --- */

static void handle_settings(AppState* state, DictionaryIterator* iter) {
    int count = read_count(iter, MESSAGE_KEY_FavouriteCount, FAVOURITES_MAX);

    arrfree(state->settings->favourite_routes);
    for (int i = 0; i < count; i++) {
        Route route = {
            .route_type = read_route_type(iter, MESSAGE_KEY_FavouriteRouteTypes + i),
            .stop_id = read_int_or(iter, MESSAGE_KEY_FavouriteStopIDs + i, 0),
            .route_id = read_int_or(iter, MESSAGE_KEY_FavouriteRouteIDs + i, 0),
        };
        read_name(iter, MESSAGE_KEY_FavouriteDisplayNames + i, route.display_name, ROUTE_NAME_MAX);
        arrpush(state->settings->favourite_routes, route);
    }

    state->settings_loaded = true;
    Settings_notify_changed(state->settings);
    notify_changed(state);
}

static void handle_stops(AppState* state, DictionaryIterator* iter) {
    int count = read_count(iter, MESSAGE_KEY_StopsCount, STOPS_MAX);
    Stop* stops = NULL;

    for (int i = 0; i < count; i++) {
        Stop stop = {
            .stop_id = read_int_or(iter, MESSAGE_KEY_StopIDs + i, 0),
            .distance_m = read_int_or(iter, MESSAGE_KEY_StopDistances + i, 0),
        };
        read_name(iter, MESSAGE_KEY_StopNames + i, stop.name, STOP_NAME_MAX);
        arrpush(stops, stop);
    }

    arrfree(state->stops);
    state->stops = stops;
    state->stops_pending = false;
    notify_changed(state);
}

static void handle_routes(AppState* state, DictionaryIterator* iter, int stop_id) {
    int count = read_count(iter, MESSAGE_KEY_RoutesCount, ROUTES_MAX);
    StopRoutes* entry = find_stop_routes(state, stop_id);

    if (!entry) {
        StopRoutes fresh = {.stop_id = stop_id};
        arrpush(state->stop_routes, fresh);
        entry = &state->stop_routes[arrlen(state->stop_routes) - 1];
    }

    arrfree(entry->routes);
    entry->pending = false;

    for (int i = 0; i < count; i++) {
        Route route = {
            .route_type = read_route_type(iter, MESSAGE_KEY_RouteTypes + i),
            .stop_id = stop_id,
            .route_id = read_int_or(iter, MESSAGE_KEY_RouteIDs + i, 0),
        };
        read_name(iter, MESSAGE_KEY_RouteNames + i, route.display_name, ROUTE_NAME_MAX);
        arrpush(entry->routes, route);
    }

    notify_changed(state);
}

static void handle_departures(
    AppState* state, DictionaryIterator* iter, int stop_id, int route_id
) {
    int count = read_count(iter, MESSAGE_KEY_DeparturesCount, DEPARTURES_MAX);
    RouteDepartures* entry = find_route_departures(state, stop_id, route_id);

    if (!entry) {
        RouteDepartures fresh = {.stop_id = stop_id, .route_id = route_id};
        arrpush(state->route_departures, fresh);
        entry = &state->route_departures[arrlen(state->route_departures) - 1];
    }

    arrfree(entry->departures);
    entry->pending = false;

    for (int i = 0; i < count; i++) {
        Departure departure = {
            .eta_secs = read_int_or(iter, MESSAGE_KEY_DepartureETAs + i, 0),
            .realtime = read_int_or(iter, MESSAGE_KEY_DepartureFlags + i, 0) != 0,
        };
        read_name(iter, MESSAGE_KEY_DepartureHeadsigns + i, departure.headsign, HEADSIGN_MAX);
        arrpush(entry->departures, departure);
    }

    notify_changed(state);
}

static void inbox_received(DictionaryIterator* iter, void* context) {
    AppState* state = context;

    Tuple* kind = dict_find(iter, MESSAGE_KEY_Kind);
    if (!kind || kind->type != TUPLE_INT) {
        return;
    }

    state->phone = PHONE_CONNECTED;

    switch (kind->value->int32) {
    case MSG_SETTINGS:
        handle_settings(state, iter);
        break;
    case MSG_STOPS:
        handle_stops(state, iter);
        break;
    case MSG_ROUTES:
        handle_routes(state, iter, read_int_or(iter, MESSAGE_KEY_StopID, 0));
        break;
    case MSG_DEPARTURES:
        handle_departures(
            state, iter, read_int_or(iter, MESSAGE_KEY_StopID, 0),
            read_int_or(iter, MESSAGE_KEY_RouteID, 0)
        );
        break;
    default:
        break;
    }
}

static void inbox_dropped(AppMessageResult reason, void* context) {
    AppState* state = context;

    // APP_MSG_BUSY only means the inbox was full, so the phone is still there and
    // the reply may arrive later. Only a lost link justifies the offline fallback.
    if (reason == APP_MSG_NOT_CONNECTED) {
        AppState_mark_phone_unreachable(state);
    }
}

/* --- requests --- */

static bool send(DictionaryIterator** out) {
    if (app_message_outbox_begin(out) != APP_MSG_OK) {
        return false;
    }
    return true;
}

void AppState_request_settings(AppState* state) {
    if (state->settings_loaded) {
        return;
    }

    DictionaryIterator* iter;
    if (!send(&iter)) {
        return;
    }
    dict_write_int32(iter, MESSAGE_KEY_Kind, MSG_SETTINGS);
    app_message_outbox_send();
}

void AppState_request_stops(AppState* state) {
    if (state->stops || state->stops_pending) {
        return;
    }

    if (state->phone == PHONE_DISCONNECTED) {
        Stop* stops = NULL;
        int count = 0;
        state->offline->load_stops(state, &stops, &count);
        arrfree(state->stops);
        state->stops = stops;
        notify_changed(state);
        return;
    }

    DictionaryIterator* iter;
    if (!send(&iter)) {
        return;
    }
    state->stops_pending = true;
    dict_write_int32(iter, MESSAGE_KEY_Kind, MSG_STOPS);
    app_message_outbox_send();
}

void AppState_request_routes(AppState* state, int stop_id) {
    StopRoutes* entry = find_stop_routes(state, stop_id);
    if (entry && (entry->routes || entry->pending)) {
        return;
    }

    if (state->phone == PHONE_DISCONNECTED) {
        Route* routes = NULL;
        int count = 0;
        state->offline->load_routes(state, stop_id, &routes, &count);
        if (!entry) {
            StopRoutes fresh = {.stop_id = stop_id};
            arrpush(state->stop_routes, fresh);
            entry = &state->stop_routes[arrlen(state->stop_routes) - 1];
        }
        arrfree(entry->routes);
        entry->routes = routes;
        notify_changed(state);
        return;
    }

    DictionaryIterator* iter;
    if (!send(&iter)) {
        return;
    }

    if (!entry) {
        StopRoutes fresh = {.stop_id = stop_id};
        arrpush(state->stop_routes, fresh);
        entry = &state->stop_routes[arrlen(state->stop_routes) - 1];
    }
    entry->pending = true;

    dict_write_int32(iter, MESSAGE_KEY_Kind, MSG_ROUTES);
    dict_write_int32(iter, MESSAGE_KEY_StopID, stop_id);
    app_message_outbox_send();
}

void AppState_request_departures(AppState* state, int stop_id, int route_id) {
    RouteDepartures* entry = find_route_departures(state, stop_id, route_id);
    if (entry && (entry->departures || entry->pending)) {
        return;
    }

    if (state->phone == PHONE_DISCONNECTED) {
        Departure* departures = NULL;
        int count = 0;
        state->offline->load_departures(state, stop_id, route_id, &departures, &count);
        if (!entry) {
            RouteDepartures fresh = {.stop_id = stop_id, .route_id = route_id};
            arrpush(state->route_departures, fresh);
            entry = &state->route_departures[arrlen(state->route_departures) - 1];
        }
        arrfree(entry->departures);
        entry->departures = departures;
        notify_changed(state);
        return;
    }

    DictionaryIterator* iter;
    if (!send(&iter)) {
        return;
    }

    if (!entry) {
        RouteDepartures fresh = {.stop_id = stop_id, .route_id = route_id};
        arrpush(state->route_departures, fresh);
        entry = &state->route_departures[arrlen(state->route_departures) - 1];
    }
    entry->pending = true;

    dict_write_int32(iter, MESSAGE_KEY_Kind, MSG_DEPARTURES);
    dict_write_int32(iter, MESSAGE_KEY_StopID, stop_id);
    dict_write_int32(iter, MESSAGE_KEY_RouteID, route_id);
    app_message_outbox_send();
}

/* --- accessors --- */

const Stop* AppState_stops(AppState* state, int* count) {
    *count = arrlen(state->stops);
    return state->stops;
}

const Route* AppState_routes(AppState* state, int stop_id, int* count) {
    StopRoutes* entry = find_stop_routes(state, stop_id);
    if (!entry) {
        *count = 0;
        return NULL;
    }
    *count = arrlen(entry->routes);
    return entry->routes;
}

const Departure* AppState_departures(AppState* state, int stop_id, int route_id, int* count) {
    RouteDepartures* entry = find_route_departures(state, stop_id, route_id);
    if (!entry) {
        *count = 0;
        return NULL;
    }
    *count = arrlen(entry->departures);
    return entry->departures;
}

const Departure* AppState_next_departure(AppState* state, int stop_id, int route_id) {
    int count;
    const Departure* departures = AppState_departures(state, stop_id, route_id, &count);

    for (int i = 0; i < count; i++) {
        if (departures[i].eta_secs >= 0) {
            return &departures[i];
        }
    }

    return NULL;
}

bool AppState_settings_loaded(AppState* state) { return state->settings_loaded; }

PhoneState AppState_phone_state(AppState* state) { return state->phone; }

void AppState_mark_phone_unreachable(AppState* state) {
    if (state->phone == PHONE_DISCONNECTED) {
        return;
    }
    state->phone = PHONE_DISCONNECTED;

    // Favourites already loaded from the phone are kept; anything still pending
    // is served from the mock provider instead of waiting on a reply that will
    // never arrive.
    if (!AppState_settings_loaded(state)) {
        state->settings_loaded = true;
        state->offline->load_settings(state);
        Settings_notify_changed(state->settings);
    }

    state->stops_pending = false;
    for (size_t i = 0; i < arrlen(state->stop_routes); i++) {
        state->stop_routes[i].pending = false;
    }
    for (size_t i = 0; i < arrlen(state->route_departures); i++) {
        state->route_departures[i].pending = false;
    }

    notify_changed(state);
}

/* --- lifecycle --- */

AppState* AppState_create(void) {
    AppState* state = malloc(sizeof(AppState));
    *state = (AppState){
        .settings = Settings_create(),
        .offline = MockProvider_get(),
        .phone = PHONE_UNKNOWN,
        .settings_loaded = false,
    };

    for (int i = 0; i < ROUTE_TYPE_COUNT; i++) {
        state->icons[i] = gbitmap_create_with_resource(ICON_RESOURCES[i]);
    }
    state->location_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOCATION);

    // Callbacks must be in place before the inbox opens.
    app_message_set_context(state);
    app_message_register_inbox_received(inbox_received);
    app_message_register_inbox_dropped(inbox_dropped);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    AppState_request_settings(state);

    return state;
}

void AppState_destroy(AppState* state) {
    app_message_deregister_callbacks();
    app_message_set_context(NULL);

    for (int i = 0; i < ROUTE_TYPE_COUNT; i++) {
        gbitmap_destroy(state->icons[i]);
    }
    gbitmap_destroy(state->location_bitmap);

    arrfree(state->stops);
    for (size_t i = 0; i < arrlen(state->stop_routes); i++) {
        arrfree(state->stop_routes[i].routes);
    }
    arrfree(state->stop_routes);
    for (size_t i = 0; i < arrlen(state->route_departures); i++) {
        arrfree(state->route_departures[i].departures);
    }
    arrfree(state->route_departures);
    arrfree(state->subscribers);

    Settings_destroy(state->settings);
    free(state);
}