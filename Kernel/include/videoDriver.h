#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <stdint.h>

#define WIDTH 80
#define HEIGHT 25
#define CHAR_EOF 4  // Ctrl+D

/**
 * @file videoDriver.h
 * @brief Video Driver for VGA Text Mode (80x25)
 * 
 * This driver writes directly to VGA text mode buffer at 0xB8000.
 * Each character takes 2 bytes: [character][attribute]
 */

/**
 * @brief Prints a null-terminated string to the screen.
 * @param string The null-terminated string to print
 */
void vd_print(const char *string);

/**
 * @brief Prints up to n characters from a string to the screen.
 * @param string The string to print
 * @param n Maximum number of characters to print
 */
void vd_nprint(const char *string, uint32_t n);

/**
 * @brief Draws a single character to the screen at the current cursor position.
 * @param character The character to draw (supports \n, \b, EOF)
 */
void vd_draw_char(char character);

/**
 * @brief Clears the entire screen and resets cursor to top-left position.
 */
void vd_clear_screen();

/**
 * @brief Sets the cursor position on the screen.
 * @param x The column position (0-79)
 * @param y The row position (0-24)
 */
void vd_set_cursor(uint32_t x, uint32_t y);

/**
 * @brief Sets the text color for subsequent output.
 * @param new_color The new color value (VGA attribute byte)
 */
void vd_set_color(uint8_t new_color);

/**
 * @brief Gets the current text color setting.
 * @return The current color value
 */
uint8_t vd_get_color(void);

/**
 * @brief Prints a 64-bit value in decimal format.
 * @param value The value to print in decimal
 */
void vd_print_dec(uint64_t value);

/**
 * @brief Prints a 64-bit value in hexadecimal format.
 * @param value The value to print in hexadecimal
 */
void vd_print_hex(uint64_t value);

/**
 * @brief Prints a 64-bit value in binary format.
 * @param value The value to print in binary
 */
void vd_print_bin(uint64_t value);

/**
 * @brief Prints a 64-bit value in the specified base.
 * @param value The value to print
 * @param base The numeric base (2-36)
 */
void vd_print_base(uint64_t value, uint32_t base);

/**
 * @brief Scrolls the screen content up by one line.
 * The top line is lost and a new blank line appears at the bottom.
 */
void vd_scroll_up(void);

/* VGA Color Codes (for use with vd_set_color) */
#define VGA_COLOR_BLACK 0x00
#define VGA_COLOR_BLUE 0x01
#define VGA_COLOR_GREEN 0x02
#define VGA_COLOR_CYAN 0x03
#define VGA_COLOR_RED 0x04
#define VGA_COLOR_MAGENTA 0x05
#define VGA_COLOR_BROWN 0x06
#define VGA_COLOR_LIGHT_GREY 0x07
#define VGA_COLOR_DARK_GREY 0x08
#define VGA_COLOR_LIGHT_BLUE 0x09
#define VGA_COLOR_LIGHT_GREEN 0x0A
#define VGA_COLOR_LIGHT_CYAN 0x0B
#define VGA_COLOR_LIGHT_RED 0x0C
#define VGA_COLOR_LIGHT_MAGENTA 0x0D
#define VGA_COLOR_YELLOW 0x0E
#define VGA_COLOR_WHITE 0x0F

/* Helper macro to create VGA color attribute */
#define VGA_COLOR(fg, bg) (((bg) << 4) | (fg))

#endif
