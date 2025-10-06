#include "include/videoDriver.h"


/* Buffer de modo texto VGA en 0xB8000 */
static uint8_t *const video = (uint8_t *)0xB8000;
static uint8_t *current_video = (uint8_t *)0xB8000;
static uint8_t current_color = 0x07; /* Blanco sobre negro (por defecto) */

/* Buffer interno para conversion de numeros a string */
static char buffer[64] = { '0' };

/* Prototipo de funcion auxiliar */
static uint32_t uint_to_base(uint64_t value, char * buffer, uint32_t base);

/* Macro para manejo de nueva linea */
#define NEW_LINE() do { \
    uint32_t current_line = ((current_video - video) / 2) / WIDTH; \
    current_video = video + ((current_line + 1) * WIDTH) * 2; \
} while(0)

void vd_print(const char *string)
{
    int i;

    for (i = 0; string[i] != 0; i++)
    {
        if (string[i] == '\n')
        {
            NEW_LINE();
            continue;
        }
        vd_draw_char(string[i]);
    }
}

void vd_nprint(const char *string, uint32_t n)  
{
    int i;

    for (i = 0; i < n; i++){
        vd_draw_char(string[i]);
    }
}

void vd_draw_char(char character)
{
    /* Verifica si necesitamos hacer scroll */
    if (current_video >= video + (WIDTH * HEIGHT * 2))
    {
        vd_scroll_up();
        current_video = video + ((HEIGHT - 1) * WIDTH) * 2;
    }
    
    /* Maneja salto de linea */
    if (character == '\n')
    {
        NEW_LINE();
        if (current_video >= video + (WIDTH * HEIGHT * 2))
        {
            vd_scroll_up();
            current_video = video + ((HEIGHT - 1) * WIDTH) * 2;
        }
        return;
    }
    
    /* Maneja backspace */
    if (character == '\b' || character == 8)
    {
        uint32_t current_col = ((current_video - video) / 2) % WIDTH;
        
        if (current_col > 0 && current_video > video)
        {
            current_video -= 2;
            *(current_video) = ' ';
            *(current_video + 1) = current_color;
        }
        return;
    }
    
    /* Maneja EOF (Ctrl+D) */
    if (character == CHAR_EOF)
    {
        return;
    }
    
    /* Verifica si necesitamos saltar a la siguiente linea */
    uint32_t current_col = ((current_video - video) / 2) % WIDTH;
    if (current_col >= WIDTH - 1)
    {
        NEW_LINE();
        if (current_video >= video + (WIDTH * HEIGHT * 2))
        {
            vd_scroll_up();
            current_video = video + ((HEIGHT - 1) * WIDTH) * 2;
        }
    }
    
    /* Escribe el caracter y su atributo de color */
    *current_video = character;
    *(current_video + 1) = current_color;
    current_video += 2;
}

void vd_clear_screen()
{
    int i;

    for (i = 0; i < HEIGHT * WIDTH * 2; i += 2)
    {
        video[i] = ' ';       
        video[i + 1] = current_color;
    }
    current_video = video; 
}

void vd_set_cursor(uint32_t x, uint32_t y)
{
    if (x < WIDTH && y < HEIGHT)
    {
        current_video = video + (x + (y * WIDTH)) * 2;
    }
}

uint8_t vd_get_color()
{
    return current_color;
}

void vd_set_color(uint8_t new_color)
{
    current_color = new_color;
}

void vd_scroll_up(void)
{
    int i;
    
    /* Mueve todas las lineas una posicion hacia arriba */
    for (i = 0; i < (HEIGHT - 1) * WIDTH * 2; i++)
    {
        video[i] = video[i + (WIDTH * 2)];
    }

    /* Limpia la linea inferior */
    for (i = (HEIGHT - 1) * WIDTH * 2; i < HEIGHT * WIDTH * 2; i += 2)
    {
        video[i] = ' ';          
        video[i + 1] = current_color;
    }
}

void vd_print_dec(uint64_t value)
{
	vd_print_base(value, 10);
}

void vd_print_hex(uint64_t value)
{
	vd_print_base(value, 16);
}

void vd_print_bin(uint64_t value)
{
	vd_print_base(value, 2);
}

void vd_print_base(uint64_t value, uint32_t base)
{
    uint_to_base(value, buffer, base);
    vd_print(buffer);
}

static uint32_t uint_to_base(uint64_t value, char * buffer, uint32_t base)
{
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	/* Convierte el numero a string (invertido) */
	do
	{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	}
	while (value /= base);

	/* Termina el string con null */
	*p = 0;

	/* Invierte el string */
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}
