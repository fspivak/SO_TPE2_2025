#include "include/terminal.h"
#include "include/libasmUser.h"
#include "include/screen.h"
#include "include/snake.h"
#include "include/stinUser.h"
#include "include/stringUser.h"
#include "tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

#define STARTING_POSITION_X 0
#define STARTING_POSITION_Y 0

#define MAX_ZOOM 3
#define MIN_ZOOM 1

/* Configuracion de pantalla por zoom level */
int charsPerLine[] = {128, 64, 42};
int charSize = 1;
int screenWidth;
int screenHeight;
int lastRunHeight = 0;

void terminal() {
	char buffer[1000];
	char c;
	int i = 0;
	int tabs = 0;
	sound(1);
	getScreenSize(&screenWidth, &screenHeight);

	/* Mensaje de bienvenida */
	print("\nWelcome to x64 BareBones OS\n");
	print("Type 'help' for available commands\n\n");
	print(">  "); /* Prompt inicial */

	while (1) {
		if ((c = getchar()) != '\n') {
			if (c == 8) {
				if (i > 0) {
					if (buffer[i - 1] == '\t') {
						tabs--;
						putchar(c);
						putchar(c);
						putchar(c);
					}
					putchar(c);
					i--;
				}
				// moveCursor();
				// actualizarPantalla();
			}
			else if (c == '\t') {
				if (i + tabs * 3 + 4 < charsPerLine[charSize - 1] * 2) {
					tabs++;
					buffer[i++] = c;
					putchar(c);
				}
			}
			else {
				if ((i + tabs * 3 + 1) < charsPerLine[charSize - 1] * 2 && c != 0) {
					buffer[i++] = c;
					putchar(c);
				}
				// actualizarPantalla();
			}
		}
		else {
			buffer[i] = 0;
			print("\n"); /* Nueva linea despues del comando */

			if (!strcmp(buffer, "help")) {
				help();
			}
			else if (!strcmp(buffer, "zoom in")) {
				print("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "zoom out")) {
				print("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "showRegisters")) {
				imprimirRegistros();
			}
			else if (!strcmp(buffer, "ps")) {
				list_processes();
			}
			else if (!strcmp(buffer, "getpid")) {
				show_current_pid();
			}
			else if (startsWith(buffer, "test_process")) {
				// Extraer argumentos si los hay
				char *args = NULL;
				if (buffer[12] == ' ') {
					args = &buffer[13]; // Saltar el espacio
				}
				run_test_process(args);
			}
			else if (!strcmp(buffer, "exit")) {
				print("Goodbye!\n");
				sound(2);
				sleepUser(20);
				exit(); /* Halt del sistema */
			}
			else if (!strcmp(buffer, "snake")) {
				print("Snake not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "clock")) {
				clock();
			}
			else if (!strcmp(buffer, "clear")) {
				/* Llamar a syscall para limpiar pantalla */
				clearScreen();
			}
			else if (buffer[0] == 't' && buffer[1] == 'e' && buffer[2] == 's' && buffer[3] == 't' && buffer[4] == '_' &&
					 buffer[5] == 'm' && buffer[6] == 'm') {
				/* Parsear parametro opcional */
				char *size_param = NULL;
				if (buffer[7] == ' ') {
					size_param = &buffer[8];
				}

				if (size_param && size_param[0] != '\0') {
					print("Running test_mm with custom size: ");
					print(size_param);
					print(" bytes\n");
					char *argv[] = {size_param, NULL};
					test_mm(1, argv);
				}
				else {
					print("Running test_mm with default size: 1MB\n");
					char *argv[] = {"1048576", NULL}; /* 1MB default */
					test_mm(1, argv);
				}
			}
			else if (i > 0) { /* Solo mostrar error si se escribio algo */
				print("Command '");
				print(buffer);
				print("' not found\n");
			}

			print(">  "); /* Mostrar prompt para siguiente comando */
			// previousLength=i;
			tabs = 0;
			i = 0;
		}
	}
}

void clean(int ammount) {
	for (int j = 0; j < ammount; j++) {
		putchar(' ');
	}
}

/**
 * @brief Muestra la ayuda del terminal con comandos disponibles
 */
void help() {
	print("\n=== Available Commands ===\n\n");
	print("General:\n");
	print("  help              - Show this help message\n");
	print("  clear             - Clear screen\n");
	print("  clock             - Show current time\n");
	print("  showRegisters     - Display CPU registers\n");
	print("  exit              - Exit terminal\n");
	print("\nProcess Management:\n");
	print("  ps                - List all processes\n");
	print("  getpid            - Show current process ID\n");
	print("\nMemory Manager Tests:\n");
	print("  test_mm           - Run memory manager test (default: 1MB)\n");
	print("  test_mm <size>    - Run memory manager test with custom size\n");
	print("                          Example: test_mm 524288 (512KB)\n");
	print("  test_process      - Run process management test (default: 3 processes)\n");
	print("  test_process <n>  - Run process management test with n processes (1-64)\n");
	print("                          Example: test_process 5\n");
	print("\n");
}

void refreshScreen() {
	for (int i = 0; i <= screenHeight; i++) {
		for (int j = 0; j <= screenWidth; j++) {
			putPixel(0, j, i);
		}
	}
}

/**
 * @brief Muestra el PID del proceso actual
 */
void show_current_pid() {
	int pid = getpid();
	print("Current PID: ");
	printBase(pid, 10);
	print("\n");
}

/**
 * @brief Lista todos los procesos activos en el sistema
 */
void list_processes() {
	/* Definir estructura ProcessInfo local */
	typedef struct {
		int pid;
		char name[32];
		uint8_t priority;
		uint64_t stack_base;
		uint64_t rsp;
		char state_name[16];
		uint8_t hasForeground;
	} ProcessInfo;

	ProcessInfo processes[64]; /* Buffer para hasta 64 procesos */
	int count = ps(processes, 64);

	if (count <= 0) {
		print("No processes found or error occurred\n");
		return;
	}

	print("\nActive Processes:\n");
	print_padded("PID", 6);
	print_padded("Name", 12);
	print_padded("Priority", 10);
	print_padded("State", 12);
	print_padded("Stack Base", 16);
	print_padded("RSP", 16);
	print("FG\n");
	print("-----------------------------------------------------------------------------\n");

	for (int i = 0; i < count; i++) {
		/* PID */
		print_int_padded(processes[i].pid, 6);

		/* Name */
		print_padded(processes[i].name, 12);

		/* Priority */
		print_int_padded(processes[i].priority, 10);

		/* State */
		print_padded(processes[i].state_name, 12);

		/* Stack Base */
		print_hex_padded(processes[i].stack_base, 16);

		/* RSP */
		print_hex_padded(processes[i].rsp, 16);

		/* Foreground */
		print_padded(processes[i].hasForeground ? "1" : "0", 3);
		print("\n");
	}

	print("-----------------------------------------------------------------------------\n");
	print("Total processes: ");
	printBase(count, 10);
	print("\n\n");
}

/**
 * @brief Imprime una cadena y la rellena con espacios hasta un ancho dado
 * @param str La cadena a imprimir
 * @param width El ancho total deseado para la columna
 */
void print_padded(const char *str, int width) {
	int len = 0;
	while (str[len] != '\0' && len < 100)
		len++; // Evitar overflow

	print((char *) str);
	for (int i = len; i < width; i++) {
		print(" ");
	}
}

/**
 * @brief Imprime un entero y lo rellena con espacios hasta un ancho dado
 * @param value El valor entero a imprimir
 * @param width El ancho total deseado para la columna
 */
void print_int_padded(int value, int width) {
	char buffer[20];
	int pos = 0;

	// Convertir entero a string
	if (value == 0) {
		buffer[pos++] = '0';
	}
	else {
		int temp = value;
		if (temp < 0) {
			buffer[pos++] = '-';
			temp = -temp;
		}

		char digits[20];
		int digit_count = 0;
		while (temp > 0) {
			digits[digit_count++] = '0' + (temp % 10);
			temp /= 10;
		}

		// Invertir los dígitos
		for (int i = digit_count - 1; i >= 0; i--) {
			buffer[pos++] = digits[i];
		}
	}
	buffer[pos] = '\0';

	print_padded(buffer, width);
}

/**
 * @brief Imprime un valor hexadecimal con prefijo 0x y padding
 * @param value El valor a imprimir en hexadecimal
 * @param width El ancho total deseado para la columna (incluyendo 0x)
 */
void print_hex_padded(uint64_t value, int width) {
	char buffer[20];
	const char hex_chars[] = "0123456789ABCDEF";
	int pos = 0;

	// Agregar prefijo 0x
	buffer[pos++] = '0';
	buffer[pos++] = 'x';

	// Convertir a hexadecimal (maximo 16 digitos para uint64_t)
	char hex_digits[16];
	int digit_count = 0;

	if (value == 0) {
		hex_digits[digit_count++] = '0';
	}
	else {
		uint64_t temp = value;
		while (temp > 0) {
			hex_digits[digit_count++] = hex_chars[temp % 16];
			temp /= 16;
		}
	}

	// Invertir los digitos hex
	for (int i = digit_count - 1; i >= 0; i--) {
		buffer[pos++] = hex_digits[i];
	}
	buffer[pos] = '\0';

	print_padded(buffer, width);
}

void run_test_process(char *args) {
	print("\n=== Running Process Test ===\n");

	extern int64_t test_processes(uint64_t argc, char *argv[]);

	int process_count = 3;
	if (args != NULL && args[0] != '\0') {
		process_count = satoi(args);
		if (process_count <= 0 || process_count > 64) {
			print("Invalid process count (1-64). Using default: 3\n");
			process_count = 3;
		}
	}

	print("Starting test_process with ");
	printBase(process_count, 10);
	print(" processes...\n\n");

	// Convertir a string para pasar al test
	char process_str[10];
	intToString(process_count, process_str);
	char *argv[] = {process_str};

	int64_t result = test_processes(1, argv);

	if (result == -1) {
		print("test_process: ERROR occurred during test\n");
	}
	else {
		print("test_process: Test completed successfully\n");
	}

	print("\n=== Process Test Completed ===\n\n");
}

/**
 * @brief Convierte un entero a string
 * @param value El valor a convertir
 * @param buffer Buffer donde guardar el string
 */
void intToString(int value, char *buffer) {
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	char temp[10];
	int pos = 0;
	int temp_value = value;

	if (temp_value < 0) {
		temp_value = -temp_value;
	}

	while (temp_value > 0) {
		temp[pos++] = '0' + (temp_value % 10);
		temp_value /= 10;
	}

	int buffer_pos = 0;
	if (value < 0) {
		buffer[buffer_pos++] = '-';
	}

	for (int i = pos - 1; i >= 0; i--) {
		buffer[buffer_pos++] = temp[i];
	}
	buffer[buffer_pos] = '\0';
}

/**
 * @brief Verifica si un string comienza con otro
 * @param str El string principal
 * @param prefix El prefijo a buscar
 * @return 1 si comienza con el prefijo, 0 si no
 */
int startsWith(const char *str, const char *prefix) {
	while (*prefix) {
		if (*str != *prefix) {
			return 0;
		}
		str++;
		prefix++;
	}
	return 1;
}
