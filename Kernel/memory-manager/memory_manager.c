#ifndef BUDDY_MM

#include "include/memory_manager.h"

/* Instancia global del memory manager */
MemoryManagerADT memory_manager;

/**
 * @brief Estructura que representa un fragmento de memoria
 */
typedef struct {
    void *start;         /* Direccion de inicio del bloque */
    uint64_t size;       /* Tamaño del bloque */
    uint8_t used;        /* 1 = ocupado, 0 = libre */
} MemoryFragment;

/**
 * @brief Estructura interna del Memory Manager Simple
 */
struct MemoryManagerCDT {
    void *start_of_memory;         /* Inicio de la memoria administrada */
    MemoryFragment *page_frames;   /* Array de fragmentos */
    uint32_t page_frames_dim;      /* Cantidad de fragmentos */
    HeapState info;                /* Informacion del estado */
};

/* Inicializa el memory manager simple */
MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory)
{
    MemoryManagerADT new_mm = (MemoryManagerADT) manager_memory;
    
    /* Alinear direccion de memoria administrada a 8 bytes */
    uint64_t managed_addr = (uint64_t)managed_memory;
    uint64_t aligned_addr = (managed_addr + 7) & ~7;

    new_mm->start_of_memory = (void *)aligned_addr;
    
    /* Inicializar array de fragmentos (justo despues de la estructura CDT) */
    new_mm->page_frames = (MemoryFragment*)(manager_memory + sizeof(struct MemoryManagerCDT));
    new_mm->page_frames_dim = 0;
    new_mm->page_frames[0].start = new_mm->start_of_memory;
    new_mm->page_frames[0].size = 0;
    new_mm->page_frames[0].used = 0;

    /* Inicializar informacion del heap */
    /* Calcular memoria disponible real: desde managed_memory hasta MEMORY_END */
    uint64_t available_memory = MEMORY_END - (uint64_t)managed_memory;
    new_mm->info.total_memory = available_memory;
    new_mm->info.used_memory = 0;
    new_mm->info.free_memory = available_memory;
    
    /* Copiar tipo de MM */
    const char *type = "simple";
    for (int i = 0; i < 6 && type[i]; i++) {
        new_mm->info.mm_type[i] = type[i];
    }
    new_mm->info.mm_type[6] = '\0';
    
    return new_mm;
}

/* Reserva un bloque de memoria */
void *memory_alloc(MemoryManagerADT self, const uint64_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* Alinear tamaño a 8 bytes */
    uint64_t aligned_size = (size + 7) & ~7;
    
    uint32_t i = 0;
    void *toReturn = NULL;
    
    /* buscar bloque libre o crear uno nuevo */
    while (toReturn == NULL)
    {
        /* Saltar bloques usados */
        while (i < self->page_frames_dim && self->page_frames[i].used == 1)
        {
            i++;
        }
        
        if (self->page_frames[i].start > (void *)MEMORY_END)
        {
            return NULL;
        }
        
        /* Si llegamos al final o encontramos un bloque suficientemente grande */
        if (i == self->page_frames_dim || self->page_frames[i].size >= aligned_size)
        {
            if (i == self->page_frames_dim)
            {
                /* Crear nuevo bloque al final */
                uint64_t start_addr = (uint64_t)self->page_frames[i].start;
                uint64_t aligned_start = (start_addr + 7) & ~7;
                
                self->page_frames[i].start = (void *)aligned_start;
                self->page_frames[i].size = aligned_size;
                
                /* Preparar el siguiente fragmento para el proximo malloc */
                self->page_frames[i+1].start = (void *)(aligned_start + aligned_size);
                
                self->page_frames_dim++;
            }
            
            /* Re-alinear si es necesario (para bloques reutilizados) */
            uint64_t addr = (uint64_t)self->page_frames[i].start;
            uint64_t aligned_addr = (addr + 7) & ~7;
            
            if (aligned_addr != addr)
            {
                uint64_t offset = aligned_addr - addr;
                self->page_frames[i].start = (void *)aligned_addr;
                self->page_frames[i].size -= offset;
            }
            
            toReturn = self->page_frames[i].start;
            self->page_frames[i].used = 1;
            self->info.used_memory += self->page_frames[i].size;
            self->info.free_memory = self->info.total_memory - self->info.used_memory;
        }
        else
        {
            /* Bloque muy pequeño, seguir buscando */
            i++;
        }
    }
    
    return toReturn;
}

/* Libera un bloque de memoria */
int memory_free(MemoryManagerADT self, void *ptr)
{
    if (ptr == NULL) {
        return -1;
    }

    /* Buscar el fragmento correspondiente */
    for (uint32_t i = 0; i < self->page_frames_dim; i++)
    {
        if (self->page_frames[i].start == ptr)
        {
            if (self->page_frames[i].used == 0) {
                /* Ya estaba libre - double free */
                return -1;
            }
            
            self->page_frames[i].used = 0;
            self->info.used_memory -= self->page_frames[i].size;
            self->info.free_memory = self->info.total_memory - self->info.used_memory;
            return 0;
        }
    }
    
    /* Puntero no encontrado */
    return -1;
}

/* Obtiene el estado actual de la memoria */
void memory_state_get(MemoryManagerADT self, HeapState *state)
{
    if (state == NULL) {
        return;
    }

    state->total_memory = self->info.total_memory;
    state->used_memory = self->info.used_memory;
    state->free_memory = self->info.free_memory;

    /* Copiar tipo de MM */
    for (size_t i = 0; i < 6 && self->info.mm_type[i]; i++) {
        state->mm_type[i] = self->info.mm_type[i];
    }
    state->mm_type[6] = '\0';
}

#endif /* BUDDY_MM */

