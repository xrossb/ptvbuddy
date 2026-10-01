#include "data.h"
#include "app_state.h"
#include "array.h"
#include <stdio.h>

#define MOCK_STOP_COUNT 6
#define MOCK_ROUTES_PER_STOP 3
#define MOCK_DEPARTURES_PER_ROUTE 4
#define MOCK_FAVOURITE_COUNT 4

typedef struct {
    const char* name;
    int distance_m;
} MockStop;

static const MockStop MOCK_STOPS[MOCK_STOP_COUNT] = {
    {"Flinders St",       120},
    {"Southern Cross",    240},
    {"Melbourne Central", 310},
    {"Richmond",          480},
    {"Glenferrie",        620},
    {"Prahran",           900},
};

// Cycled by stop index so every stop gets a different mix of modes, and the
// two NIGHT_BUS slots give the icon table a night bus to draw.
static const RouteType MOCK_ROUTE_TYPES[3][MOCK_ROUTES_PER_STOP] = {
    {ROUTE_TYPE_TRAM,  ROUTE_TYPE_BUS,       ROUTE_TYPE_TRAIN},
    {ROUTE_TYPE_BUS,   ROUTE_TYPE_NIGHT_BUS, ROUTE_TYPE_TRAM },
    {ROUTE_TYPE_TRAIN, ROUTE_TYPE_TRAM,      ROUTE_TYPE_BUS  },
};

static const int MOCK_ROUTE_NUMBERS[] = {109, 86, 48, 12, 75, 19};
static const char* const MOCK_HEADSIGNS[] = {"City", "South Cross", "Bundoora RMIT"};
static const int MOCK_ETA_SECS[] = {120, 420, 900, 1800};

// stop index, route slot
static const int MOCK_FAVOURITES[][2] = {
    {0, 0},
    {1, 1},
    {2, 0},
    {3, 1},
};

static int mock_stop_id(int stop_index) { return stop_index + 1; }

static int mock_stop_index(int stop_id) { return stop_id - 1; }

static RouteType mock_route_type(int stop_index, int slot) {
    return MOCK_ROUTE_TYPES[stop_index % 3][slot];
}

static int mock_route_id(int stop_index, int slot) { return (stop_index + 1) * 100 + slot + 1; }

static int mock_route_number(int stop_index, int slot) {
    return MOCK_ROUTE_NUMBERS[(stop_index * MOCK_ROUTES_PER_STOP + slot) % 6];
}

static const char* mock_headsign(int stop_index, int slot) {
    return MOCK_HEADSIGNS[(stop_index + slot) % 3];
}

static Route mock_route(int stop_index, int slot) {
    Route route = {
        .route_type = mock_route_type(stop_index, slot),
        .stop_id = mock_stop_id(stop_index),
        .route_id = mock_route_id(stop_index, slot),
    };
    snprintf(
        route.display_name, ROUTE_NAME_MAX, "%d %s", mock_route_number(stop_index, slot),
        mock_headsign(stop_index, slot)
    );
    return route;
}

static void mock_load_settings(AppState* state) {
    arrfree(state->settings->favourite_routes);

    for (int i = 0; i < MOCK_FAVOURITE_COUNT; i++) {
        int stop_index = MOCK_FAVOURITES[i][0];
        int slot = MOCK_FAVOURITES[i][1];
        arrpush(state->settings->favourite_routes, mock_route(stop_index, slot));
    }
}

static void mock_load_stops(AppState* state, Stop** out, int* count) {
    Stop* stops = NULL;

    for (int i = 0; i < MOCK_STOP_COUNT; i++) {
        Stop stop = {
            .stop_id = mock_stop_id(i),
            .distance_m = MOCK_STOPS[i].distance_m,
        };
        snprintf(stop.name, STOP_NAME_MAX, "%s", MOCK_STOPS[i].name);
        arrpush(stops, stop);
    }

    *out = stops;
    *count = MOCK_STOP_COUNT;
}

static void mock_load_routes(AppState* state, int stop_id, Route** out, int* count) {
    Route* routes = NULL;
    int stop_index = mock_stop_index(stop_id);

    if (stop_index < 0 || stop_index >= MOCK_STOP_COUNT) {
        *out = NULL;
        *count = 0;
        return;
    }

    for (int slot = 0; slot < MOCK_ROUTES_PER_STOP; slot++) {
        arrpush(routes, mock_route(stop_index, slot));
    }

    *out = routes;
    *count = MOCK_ROUTES_PER_STOP;
}

static void mock_load_departures(
    AppState* state, int stop_id, int route_id, Departure** out, int* count
) {
    Departure* departures = NULL;
    int stop_index = mock_stop_index(stop_id);
    int slot = -1;

    for (int i = 0; i < MOCK_ROUTES_PER_STOP; i++) {
        if (mock_route_id(stop_index, i) == route_id) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        *out = NULL;
        *count = 0;
        return;
    }

    for (int i = 0; i < MOCK_DEPARTURES_PER_ROUTE; i++) {
        Departure departure = {
            .eta_secs = MOCK_ETA_SECS[i],
            .realtime = i < 2,
        };
        snprintf(departure.headsign, HEADSIGN_MAX, "%s", mock_headsign(stop_index, slot));
        arrpush(departures, departure);
    }

    *out = departures;
    *count = MOCK_DEPARTURES_PER_ROUTE;
}

static const DataProvider MOCK_PROVIDER = {
    .load_settings = mock_load_settings,
    .load_stops = mock_load_stops,
    .load_routes = mock_load_routes,
    .load_departures = mock_load_departures,
};

const DataProvider* MockProvider_get(void) { return &MOCK_PROVIDER; }