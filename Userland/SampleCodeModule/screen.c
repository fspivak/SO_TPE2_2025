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