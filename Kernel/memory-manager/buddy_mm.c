#ifdef BUDDY_MM

#include "../include/lib.h"
#include "../include/videoDriver.h"
#include "include/memory_manager.h"

#define MIN_LEVEL 5	 /* Tamaño minimo de bloque: 2^5 = 32 bytes */
#define MAX_ORDER 25 /* Maximo nivel de orden */

/* Instancia global del memory manager */
MemoryManagerADT memory_manager;

/**
 * @brief Estructura de un bloque en el Buddy System
 */
typedef struct block_t {
	int8_t order;		  /* Orden del bloque (tamaño = 2^order) */
	int8_t status;		  /* FREE u OCCUPIED */
	struct block_t *next; /* Siguiente bloque en la lista */
} block_t;

/**
 * @brief Estructura interna del Buddy Memory Manager
 */
typedef struct MemoryManagerCDT {
	int8_t max_order;				 /* Orden maximo del sistema */
	block_t *free_blocks[MAX_ORDER]; /* Arrays de listas de bloques libres por orden */
	HeapState info;					 /* Informacion del estado */
	uint64_t managed_memory_start;	 /* Base de memoria administrada (despues de metadata) */
} buddy_manager;

/* Crea un nuevo bloque y lo agrega a la lista de libres */
static block_t *create_block(MemoryManagerADT self, void *address, int8_t order) {
	block_t *block = (block_t *) address;
	block->order = order;
	block->status = FREE;
	block->next = self->free_blocks[order];
	self->free_blocks[order] = block;
	return block;
}

/* Remueve un bloque de la lista de bloques libres */
static void remove_block(MemoryManagerADT self, block_t *block) {
	if (block == NULL) {
		return;
	}

	uint8_t order = block->order;
	block_t *curr_block = self->free_blocks[order];

	if (curr_block == NULL) {
		return;
	}

	/* Si es el primero de la lista */
	if (curr_block == block) {
		self->free_blocks[order]->status = OCCUPIED;
		self->free_blocks[order] = self->free_blocks[order]->next;
		return;
	}

	/* Buscar el bloque en la lista */
	while (curr_block->next != NULL && curr_block->next != block) {
		curr_block = curr_block->next;
	}

	if (curr_block->next == NULL) {
		return;
	}

	/* Remover de la lista */
	block_t *to_remove = curr_block->next;
	curr_block->next = to_remove->next;
	to_remove->status = OCCUPIED; /* Marcar el bloque que se esta removiendo */
}

/* Divide un bloque en dos buddies de orden menor */
static void split_block(MemoryManagerADT self, int8_t order) {
	block_t *block = self->free_blocks[order];
	remove_block(self, block);

	/* Calcular posicion del buddy (mitad del bloque) */
	block_t *buddy_block = (block_t *) ((void *) block + (1L << (order - 1)));

	/* Crear dos bloques de orden-1 */
	create_block(self, (void *) buddy_block, order - 1);
	create_block(self, (void *) block, order - 1);
}

/* Fusiona dos bloques buddy en uno de orden superior */
static block_t *merge(MemoryManagerADT self, block_t *block, block_t *buddy_block) {
	if (block == NULL || buddy_block == NULL) {
		return NULL;
	}

	/* El bloque izquierdo es el de menor direccion */
	block_t *left = block < buddy_block ? block : buddy_block;
	block_t *right = block < buddy_block ? buddy_block : block;

	/* Remover AMBOS bloques de sus listas libres */
	remove_block(self, left);
	remove_block(self, right);

	left->order++;
	left->status = FREE;
	return left;
}

/* Inicializa el Buddy Memory Manager */
MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;

	/* Guardar la base de memoria administrada */
	mm->managed_memory_start = (uint64_t) managed_memory;

	/* Calcular tamaño de memoria administrada real */
	uint64_t managed_size = MEMORY_END - (uint64_t) managed_memory;

	/* Calcular orden maximo necesario para contener la memoria administrada */
	int current_size = 2;
	int level = 1;
	while (current_size < managed_size) {
		level++;
		current_size *= 2;
	}

	mm->max_order = level;
	mm->info.total_memory = managed_size;
	mm->info.used_memory = 0;
	mm->info.free_memory = managed_size;

	/* Copiar tipo de MM */
	const char *type = "buddy";
	for (int i = 0; i < 6; i++) {
		mm->info.mm_type[i] = (i < 5) ? type[i] : '\0';
	}

	/* Inicializar todas las listas de bloques libres */
	for (int i = 0; i < MAX_ORDER; i++) {
		mm->free_blocks[i] = NULL;
	}

	/* Crear bloque inicial con toda la memoria ADMINISTRADA (no desde MEMORY_START) */
	create_block(mm, managed_memory, level);

	return mm;
}

/* Reserva un bloque de memoria usando Buddy System */
void *memory_alloc(MemoryManagerADT self, const uint64_t size) {
	if (size == 0 || size > MEMORY_SIZE) {
		return NULL;
	}

	/* Calcular orden necesario (potencia de 2 que contenga size + metadata) */
	int8_t order = 1;
	int64_t block_size = 2;
	while (block_size < size + sizeof(block_t)) {
		order++;
		block_size *= 2;
	}

	/* Respetar tamaño minimo */
	order = (MIN_LEVEL > order) ? MIN_LEVEL : order;

	/* Si no hay bloque del orden exacto, buscar uno mayor */
	if (self->free_blocks[order] == NULL) {
		uint8_t order_approx = 0;

		for (uint8_t i = order + 1; i <= self->max_order && order_approx == 0; i++) {
			if (self->free_blocks[i] != NULL) {
				order_approx = i;
			}
		}

		if (order_approx == 0) {
			/* No hay bloques disponibles */
			return NULL;
		}

		/* Dividir bloques hasta llegar al orden necesario */
		while (order_approx > order) {
			split_block(self, order_approx);
			order_approx--;
		}
	}

	/* Tomar bloque de la lista de libres */
	block_t *block = self->free_blocks[order];

	/* Verificar que efectivamente hay un bloque disponible */
	if (block == NULL) {
		return NULL;
	}

	remove_block(self, block);
	block->status = OCCUPIED;

	/* Retornar puntero despues de la metadata */
	return (void *) ((uint8_t *) block + sizeof(block_t));
}

/* Libera un bloque y fusiona con buddies libres */
int memory_free(MemoryManagerADT self, void *ptrs) {
	if (ptrs == NULL) {
		return -1;
	}

	/* Obtener puntero al bloque (antes de la metadata) */
	block_t *block = (block_t *) (ptrs - sizeof(block_t));

	if (block == NULL || block->status == FREE) {
		/* Bloque invalido o doble free */
		return -1;
	}

	block->status = FREE;

	/* Calcular posicion del buddy usando XOR (relativo a managed_memory_start) */
	uint64_t block_pos = (uint64_t) ((void *) block - self->managed_memory_start);
	block_t *buddy_block = (block_t *) (self->managed_memory_start + (((uint64_t) block_pos) ^ ((1L << block->order))));

	/* Validar que buddy esta dentro del rango de memoria valido */
	if (buddy_block == NULL || (uint64_t) buddy_block < self->managed_memory_start ||
		(uint64_t) buddy_block >= MEMORY_END) {
		/* Buddy fuera de rango, no fusionar, solo agregar a free list */
		create_block(self, (void *) block, block->order);
		return 0;
	}

	/* Intentar fusionar con buddies mientras sea posible */
	while (block && buddy_block && block->order < self->max_order && buddy_block->status == FREE &&
		   buddy_block->order == block->order) {
		block = merge(self, block, buddy_block);

		if (block == NULL) {
			return -1;
		}

		block->status = FREE;

		/* Calcular nuevo buddy del bloque fusionado (relativo a managed_memory_start) */
		block_pos = (uint64_t) ((void *) block - self->managed_memory_start);
		buddy_block =
			(block_t *) (block_t *) (self->managed_memory_start + (((uint64_t) block_pos) ^ ((1L << block->order))));

		/* Validar que buddy esta dentro del rango de memoria valido */
		if ((uint64_t) buddy_block < self->managed_memory_start || (uint64_t) buddy_block >= MEMORY_END) {
			/* Buddy fuera de rango, salir del loop de merge */
			break;
		}
	}

	if (block == NULL) {
		return -1;
	}

	/* Agregar bloque a la lista de libres */
	create_block(self, (void *) block, block->order);

	return 0;
}

/* Obtiene el estado actual de la memoria */
void memory_state_get(MemoryManagerADT self, HeapState *state) {
	if (state == NULL) {
		return;
	}

	/* Calcular memoria libre recorriendo todas las listas */
	self->info.free_memory = 0;
	for (uint64_t i = MIN_LEVEL; i < self->max_order; i++) {
		block_t *current_block = self->free_blocks[i];
		while (current_block != NULL) {
			self->info.free_memory += (uint64_t) (1UL << i);
			current_block = current_block->next;
		}
	}

	state->total_memory = self->info.total_memory;
	state->used_memory = self->info.total_memory - self->info.free_memory;
	state->free_memory = self->info.free_memory;

	/* Copiar tipo de MM */
	for (size_t i = 0; i < 6 && self->info.mm_type[i]; i++) {
		state->mm_type[i] = self->info.mm_type[i];
	}
	state->mm_type[6] = '\0';
}

#endif /* BUDDY_MM */
