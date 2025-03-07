/**
 * @file ctype.c
 * @author Zack Bostock
 * @brief Character testing and mapping
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

#include <libc/ctype.h>

/**
 * @brief Determines if a character is alphabetic
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/**
 * @brief Determines if a character is a digit
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isdigit(int c) {
    return c >= '0' && c <= '9';
}

/**
 * @brief Determines if a character is a digit or alphabetical
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

/**
 * @brief Determines if a character is a control character
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int iscntrl(int c) {
    return c >= 0 && c <= 31;
}

/**
 * @brief Determines if a character is a blank character (a space or tab)
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isblank(int c) {
    return c == ' ' || c == '\t';
}

/**
 * @brief Determines if a character is printable, expect for space
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isgraph(int c) {
    return c >= 33 && c <= 126;
}

/**
 * @brief Determines if a character is lowercase
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int islower(int c) {
    return c >= 'a' && c <= 'z';
}

/**
 * @brief Determines if a character is uppercase
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

/**
 * @brief Determines if a character is printable, including space
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isprint(int c) {
    return c >= 32 && c <= 126;
}

/**
 * @brief Determines if a character is printable, excluding spaces and alphanumeric
 * characters
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int ispunct(int c) {
    return (c >= 33 && c <= 47) ||
           (c >= 58 && c <= 64) ||
           (c >= 91 && c <= 96) ||
           (c >= 123 && c <= 126);
}

/**
 * @brief Determines if a character is a whitespace character
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isspace(int c) {
    return (c == ' ') || (c >= 9 && c <= 13);
}

/**
 * @brief Determines if a character is one of the hexadecimal characters
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isxdigit(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/**
 * @brief Converts a character to its lowercase equivalent
 * @note If the character passed in is not alphabetic or is already lowercase,
 * the function returns the character passed in
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int tolower(int c) {
    if (!isalpha(c) || islower(c)) {
        return c;
    }

    return c + 32;
}

/**
 * @brief Converts a character to its uppercase equivalent
 * @note If the character passed in is not alphabetic or is already uppercase,
 * the function returns the character passed in
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int toupper(int c) {
    if (!isalpha(c) || isupper(c)) {
        return c;
    }

    return c - 32;
}