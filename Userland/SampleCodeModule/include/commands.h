#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * @brief Muestra la ayuda del terminal con comandos disponibles
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void help_cmd(int argc, char **argv);

/**
 * @brief Limpia la pantalla
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void clear_cmd(int argc, char **argv);

/**
 * @brief Lista todos los procesos activos en el sistema
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void ps_cmd(int argc, char **argv);

/**
 * @brief Muestra el estado de la memoria
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void mem_cmd(int argc, char **argv);

#endif // COMMANDS_H
