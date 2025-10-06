#ifndef FONT_H
#define FONT_H

#include <stddef.h>
#include <stdint.h>

// Definimos un enum con los nombres de los colores
typedef enum { RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, BLACK, WHITE } Color;

uint8_t getFontWidth();

uint8_t getFontHeight();

uint8_t *getFontChar(char c);

uint32_t getColor(Color color);

#endif // FONT_H