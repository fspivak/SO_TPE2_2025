#include "../memory-manager/include/memory_manager.h"
#include "../scheduler/include/process.h"
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

/* Process syscalls */
process_id_t sys_create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
								uint8_t priority);
process_id_t sys_getpid();
int sys_kill(process_id_t pid);
int sys_block(process_id_t pid);
int sys_unblock(process_id_t pid);
int sys_nice(process_id_t pid, uint8_t new_priority);
void sys_yield();
int sys_ps(ProcessInfo *buffer, int max_processes);
void sys_exit();
int sys_waitpid(process_id_t pid);
