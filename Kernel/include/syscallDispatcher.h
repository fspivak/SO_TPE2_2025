#include "../memory-manager/include/memory_manager.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum { STDIN = 0, STDOUT = 1, STDERR = 2 } FDS;

uint64_t syscallDispatcher(uint64_t rax, ...);
void sys_write(FDS fd, const char *buf, size_t count, size_t color, size_t background);
void sys_read(FDS fd, char *buffer, size_t count);
void sys_sleep(int seconds);
void sys_zoom(int zoom);
void sys_draw(int color, int x, int y);
void sys_screenDetails(int *width, int *height);
void sys_setCursor(int x, int y);
void sys_clear_screen();
void sys_getClock(char *str);
void sys_getMilis(uint64_t *milis);
void sys_getcharNL(char *charac);
void sys_playSound(int index);
void sys_impRegs();

/* Memory Manager syscalls */
void *sys_malloc(uint64_t size);
int sys_free(void *ptr);
void sys_mem_status(HeapState *state);
