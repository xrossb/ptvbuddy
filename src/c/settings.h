#pragma once

typedef enum {
    ROUTE_TYPE_TRAIN = 0,
    ROUTE_TYPE_TRAM = 1,
    ROUTE_TYPE_BUS = 2,
    ROUTE_TYPE_VLINE = 3,
    ROUTE_TYPE_NIGHT_BUS = 4,
} RouteType;

typedef struct {
    RouteType route_type;
    int stop_id;
    int route_id;
    char display_name[32];
} Route;

typedef struct {
    Route* favourite_routes;
} Settings;

Settings* Settings_create(void);
void Settings_destroy(Settings* settings);

typedef void (*SettingsHandler)(Settings*);

void Settings_subscribe(Settings* settings, SettingsHandler handler);
void Settings_unsubscribe(Settings* settings, SettingsHandler handler);
void Settings_notify_changed(Settings* settings);
