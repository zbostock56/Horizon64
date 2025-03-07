/**
 * @file callback.h
 * @author Zack Bostocj
 * @brief Information pertaining to publishing or subscribing to an event callback
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <proc/process.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
STATUS cb_publish(PROC_ID id, CB_TYPE type, CB_PARAM param);
STATUS cb_subscribe(PROC_ID id, CB_TYPE type, CB_PARAM *param);
STATUS cb_dispatch();