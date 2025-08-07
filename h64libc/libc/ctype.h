/**
 * @file ctype.h
 * @author Zack Bostock
 * @brief Character testing and mapping information
 * @verbatim
 * The ctype.h header file of the C Standard Library declares several
 * functions that are useful for testing and mapping characters.
 * All the functions accepts int as a parameter, whose value must be EOF or
 * representable as an unsigned char. All the functions return non-zero (true)
 * if the argument c satisfies the condition described, and zero(false) if not.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);