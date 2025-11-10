// funcion para obtener el ultimo caracter del buffer de entrada
#ifndef STDINOUT_H
#define STDINOUT_H

/**
 * @brief Obtiene el ultimo caracter del buffer de entrada
 * @return El ultimo caracter del buffer de entrada o -1 si no hay
 */
char getChar();

/**
 * @brief Muestra en salida estandar un caracter
 * @param character El caracter a mostrar
 * @param color El color del caracter
 * @param bg El fondo del caracter
 */
void putChar(char character, int color, int bg);

/**
 * @brief Imprime un string en salida estandar
 * @param string El string a imprimir
 * @param len La longitud del string
 * @param color El color del string
 * @param background El fondo del string
 */
void write(const char *string, int len, int color, int background);

/**
 * @brief Toma un string de hasta longitud len de entrada estandar
 * @param buffer El buffer donde guardar el string
 * @param len La longitud del string
 * @return La cantidad de caracteres leidos
 */
int read_keyboard(char *buffer, int len);

/**
 * @brief Obtiene el ultimo caracter del buffer de entrada sin bucle
 * @return El ultimo caracter del buffer de entrada sin bucle o -1 si no hay
 */
char getcharNonLoop();

#endif // STDINOUT_H