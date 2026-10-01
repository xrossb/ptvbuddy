#pragma once

#include "data.h"

typedef struct {
    Route* favourite_routes;
} Settings;

Settings* Settings_create(void);
void Settings_destroy(Settings* settings);

typedef void (*SettingsHandler)(Settings*);

void Settings_subscribe(Settings* settings, SettingsHandler handler);
void Settings_unsubscribe(Settings* settings, SettingsHandler handler);
void Settings_notify_changed(Settings* settings);
