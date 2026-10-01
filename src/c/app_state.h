#pragma once

#include "data.h"
#include "settings.h"
#include <pebble.h>

// Requests (watch -> phone) and responses (phone -> watch) share this numbering.
// A response is routed by its Kind alone; only one response is in flight at a time.
typedef enum {
    MSG_SETTINGS = 0,
    MSG_STOPS = 1,
    MSG_ROUTES = 2,
    MSG_DEPARTURES = 3,
} MessageKind;

typedef enum {
    PHONE_UNKNOWN = 0,
    PHONE_CONNECTED,
    PHONE_DISCONNECTED,
} PhoneState;

typedef struct {
    int stop_id;
    Route* routes;
    bool pending;
} StopRoutes;

typedef struct {
    int stop_id;
    int route_id;
    Departure* departures;
    bool pending;
} RouteDepartures;

typedef void (*AppStateHandler)(struct AppState*);

typedef struct AppState {
    Settings* settings;
    const DataProvider* offline; // fallback provider, used when the phone is unreachable

    PhoneState phone;

    Stop* stops;                       // nearby, one list
    StopRoutes* stop_routes;           // keyed by stop_id, so BACK does not refetch
    RouteDepartures* route_departures; // keyed by (stop_id, route_id)

    bool stops_pending;
    bool settings_loaded;

    GBitmap* icons[ROUTE_TYPE_COUNT];
    GBitmap* location_bitmap;

    AppStateHandler* subscribers;
} AppState;

AppState* AppState_create(void);
void AppState_destroy(AppState* state);

void AppState_subscribe(AppState* state, AppStateHandler handler);
void AppState_unsubscribe(AppState* state, AppStateHandler handler);

GBitmap* AppState_icon(AppState* state, RouteType type);

void AppState_request_settings(AppState* state);
void AppState_request_stops(AppState* state);
void AppState_request_routes(AppState* state, int stop_id);
void AppState_request_departures(AppState* state, int stop_id, int route_id);

const Stop* AppState_stops(AppState* state, int* count);
const Route* AppState_routes(AppState* state, int stop_id, int* count);
const Departure* AppState_departures(AppState* state, int stop_id, int route_id, int* count);

// First departure not yet departed, or NULL. NULL triggers a fetch.
const Departure* AppState_next_departure(AppState* state, int stop_id, int route_id);

bool AppState_settings_loaded(AppState* state);
PhoneState AppState_phone_state(AppState* state);
void AppState_mark_phone_unreachable(AppState* state);