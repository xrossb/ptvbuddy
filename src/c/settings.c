#include "settings.h"
#include "array.h"
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    SettingsHandler* subscribers;
} SettingsHeader;

SettingsHeader* Settings_header(Settings* settings) { return (SettingsHeader*)(settings)-1; }

Settings* Settings_create(void) {
    SettingsHeader* header = malloc(sizeof(SettingsHeader) + sizeof(Settings));
    Settings* settings = (Settings*)(header + 1);

    *header = (SettingsHeader){};
    *settings = (Settings){};

    return settings;
}

void Settings_destroy(Settings* settings) {
    arrfree(settings->favourite_routes);

    SettingsHeader* header = Settings_header(settings);
    arrfree(header->subscribers);

    free(header);
}

void Settings_subscribe(Settings* settings, SettingsHandler handler) {
    SettingsHeader* header = Settings_header(settings);
    SettingsHandler* subscribers = header->subscribers;

    for (size_t i = 0; i < arrlen(subscribers); i++) {
        if (subscribers[i] == handler) {
            return;
        }
    }

    arrpush(header->subscribers, handler);
}

void Settings_unsubscribe(Settings* settings, SettingsHandler handler) {
    SettingsHeader* header = Settings_header(settings);
    SettingsHandler* subscribers = header->subscribers;

    for (size_t i = 0; i < arrlen(subscribers); i++) {
        if (subscribers[i] == handler) {
            size_t last = arrlen(subscribers) - 1;
            subscribers[i] = subscribers[last];
            arrpop(header->subscribers);
            return;
        }
    }
}

void Settings_notify_changed(Settings* settings) {
    SettingsHeader* header = Settings_header(settings);
    SettingsHandler* subscribers = header->subscribers;

    for (size_t i = 0; i < arrlen(subscribers); i++) {
        SettingsHandler handler = subscribers[i];
        handler(settings);
    }
}
