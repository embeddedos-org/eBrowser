// SPDX-License-Identifier: MIT
#ifndef eBrowser_SETTINGS_PAGE_H
#define eBrowser_SETTINGS_PAGE_H

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>

bool eb_settings_init(void);
void eb_settings_deinit(void);

bool eb_settings_get_string(const char *key, char *value, size_t max);
bool eb_settings_set_string(const char *key, const char *value);
bool eb_settings_get_bool(const char *key, bool dflt);
bool eb_settings_set_bool(const char *key, bool val);

void eb_settings_render(lv_obj_t *parent);

#endif /* eBrowser_SETTINGS_PAGE_H */
