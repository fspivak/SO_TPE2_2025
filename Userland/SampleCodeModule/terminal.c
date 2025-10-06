#include "include/stinUser.h"
#include "include/stringUser.h"
#include "include/terminal.h"
#include "include/screen.h"
#include "include/snake.h"
#include "include/libasmUser.h"
#include "include/test_util.h"
#include <stdint.h>

#define STARTING_POSITION_X 0
#define STARTING_POSITION_Y 0

#define MAX_ZOOM 3
#define MIN_ZOOM 1


int charsPerLine[]={128,64,42};
int charSize=1;
int screenWidth;
int screenHeight;
int lastRunHeight=0;

void terminal(){
    char buffer[1000];
    char c;
    int i=0;
    int tabs=0;
    sound(1);
    getScreenSize(&screenWidth,&screenHeight);
    
    /* Limpiar pantalla para modo texto VGA */
    print("\n\n");  /* Agregar separacion del boot */
    print(">  ");   /* Mostrar prompt inicial */
    
    while(1){
        if((c=getchar())!='\n'){
            if(c==8){
                if(i>0){
                    if(buffer[i-1]=='\t'){
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
            else if(c=='\t'){
                if(i+tabs*3+4<charsPerLine[charSize-1]*2){
                    tabs++;
                    buffer[i++]=c;
                    putchar(c);
                }
                
            }else{
                if((i+tabs*3+1)<charsPerLine[charSize-1]*2 && c!=0){
                    buffer[i++]=c;
                    putchar(c);
                }
                // actualizarPantalla();
            } 
        }
        else{
            buffer[i]=0;
            print("\n");  /* Nueva linea despues del comando */

            if(!strcmp(buffer,"help")){
                help();
            }
            else if(!strcmp(buffer,"zoom in")){
                print("Zoom no disponible en modo texto VGA\n");
            }
            else if(!strcmp(buffer,"zoom out")){
                print("Zoom no disponible en modo texto VGA\n");
            }
            else if(!strcmp(buffer,"showRegisters")){
                imprimirRegistros();
            }
            else if(!strcmp(buffer,"exit")){
                print("Bye Bye\n");
                sound(2);
                sleepUser(20);
                exit();  /* Halt del sistema */
            }
            else if (!strcmp(buffer,"snake")){
                print("Snake no disponible en modo texto VGA\n");
            }
            else if(!strcmp(buffer, "clock")){
                clock();
            }
            else if(!strcmp(buffer, "clear")){
                /* Llamar a syscall para limpiar pantalla */
                clearScreen();
            }
            else if(!strcmp(buffer, "testmm")){
                print("Ejecutando test_mm con 1MB de memoria...\n");
                test_mm(1024 * 1024);  /* 1 MB */
            }
            else if(i > 0){  /* Solo mostrar error si se escribio algo */
                print("Command '");
                print(buffer);
                print("' not found\n");
            }

            print(">  ");  /* Mostrar prompt para siguiente comando */
            // previousLength=i;
            tabs=0;
            i=0;
        }
    }
}

void clean(int ammount){
    for(int j=0;j<ammount;j++){
        putchar(' ');
    }
}


void help(){
    print("Bienvenido a la terminal\n");
    print("Comandos disponibles:\n");
    print("  help              - Muestra esta ayuda\n");
    print("  clear             - Limpia la pantalla\n");
    print("  clock             - Muestra la hora actual\n");
    print("  showRegisters     - Muestra los registros en pantalla\n");
    print("  testmm            - Test de Memory Manager\n");
    print("  exit              - Cierra la terminal\n");
}

void refreshScreen(){
    for(int i=0;i<=screenHeight;i++){
        for(int j=0;j<=screenWidth;j++){
            putPixel(0,j,i);
        }
    }
}
