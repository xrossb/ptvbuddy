#pragma once

#include <pebble.h>

#define ROUTE_NAME_MAX 32
#define STOP_NAME_MAX 40
#define HEADSIGN_MAX 24

typedef enum {
    ROUTE_TYPE_TRAIN = 0,
    ROUTE_TYPE_TRAM = 1,
    ROUTE_TYPE_BUS = 2,
    ROUTE_TYPE_VLINE = 3,
    ROUTE_TYPE_NIGHT_BUS = 4,
    ROUTE_TYPE_COUNT, // bounds the icon table
} RouteType;

// Shared by favourites and by routes-at-a-stop. stop_id is ignored in the latter.
typedef struct {
    RouteType route_type;
    int stop_id;
    int route_id;
    char display_name[ROUTE_NAME_MAX];
} Route;

typedef struct {
    int stop_id;
    int distance_m;
    char name[STOP_NAME_MAX];
} Stop;

typedef struct {
    int eta_secs; // negative = departed / cancelled
    bool realtime;
    char headsign[HEADSIGN_MAX];
} Departure;

// Seam. Mock impl now; PTV API impl later is a drop-in replacement.
// Every load_* allocates *out and sets *count; the caller takes ownership.
struct AppState;
typedef struct {
    void (*load_settings)(struct AppState*);
    void (*load_stops)(struct AppState*, Stop** out, int* count);
    void (*load_routes)(struct AppState*, int stop_id, Route** out, int* count);
    void (*load_departures)(
        struct AppState*, int stop_id, int route_id, Departure** out, int* count
    );
} DataProvider;

const DataProvider* MockProvider_get(void);