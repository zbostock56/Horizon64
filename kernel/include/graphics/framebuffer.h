#pragma once

#include <globals.h>
#include <memory.h>

#define DEFAULT_BG (COLOR_BLACK)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void fb_init(struct limine_framebuffer_request req);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
