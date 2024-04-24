#pragma once

#include <globals.h>

#include <limine.h>
#include <init.h>

static volatile LIMINE_BASE_REVISION(1);

static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};