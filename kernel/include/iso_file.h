#pragma once

#include <globals.h>

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int check_string_ending(const char *str, const char *end);
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file);
/* --------------------------- EXTERNALLY DEFINED --------------------------- */
