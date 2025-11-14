// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/screen.h"
#include "include/libasmUser.h"

void putPixel(int color, int x, int y) {
	draw(color, x, y);
}
void getScreenSize(int *width, int *height) {
	screenDetails(width, height);
}
void moveCursor(int x, int y) {
	setCursor(x, y);
}