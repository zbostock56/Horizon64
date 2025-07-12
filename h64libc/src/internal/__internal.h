/**
 * @file internal.h
 * @author Zack Bostock
 * @brief Helper functions/variables which are not designed to be called/used
 *        anywhere else other than in libc functions
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */
extern int __set_errno;

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void __syscall_ret(int r);