#include "include/snake.h"
#include "include/screen.h"
#include "include/stinUser.h"
#include "include/stringUser.h"

#define GRIS 0x808080
#define ROJO 0xFF0000
#define VERDECLARO 0x74FF33
#define VERDEOSCURO 0x266F04
#define VIOLETA 0x9a4eae

#define TIME_INTERVAL 9

#define APPLE 3
#define WINNER1 4
#define WINNER2 5
#define LOSE 6
#define TIE 7

#define X 0
#define Y 1

#define STARTING_POS_X 2
#define STARTING_POS_Y 5

#define OCCUPIED 1
#define FREED 0

int played=0;
#define NARANJA 0xFF8000
#define CIAN 0x0098D5
#define AMARILLO 0xFFCC0F

int last_result_len=0;


void cleanResult(int len);

int checkPoints(int points);

void snake(){
    int quit=0;
    char c;
    int i=0;
    char buffer[128];
    int jugadores=1;
    printColor("Bienvenido a snake!\n",ROJO,0x000000);
    print("Como jugar: El objetivo es agarrar las manzanas (rojo), para ello, el jugador 1 (naranja) se puede mover con 'wasd'\nY el jugador 2 (azul) con 'ijkl'.\nSeleccione numero de jugadores: ");
    printColor("[1jug]",NARANJA,0x000000);
    print("  o  ");
    printColor("[2jug]\n",CIAN,0x000000);
    printColor("[quit]  ",ROJO,0x000000);
    print("para salir, ");
    printColor("[play] ",AMARILLO,0x000000);
    print("para jugar\n\n");
    putCharColor('>',0xFFCC0F,0);
    printTablero(); //11x11 de cuadrados de 45x45
    while(!quit){
        c=getchar();
        if(c!='\n'){
            if(c==8){
                if(i>0){
                    i--;
                    putchar(c);
                }
            }
            else if(c=='\t'){
                
            }
            else{
                if(i<128-1 && c!=0){
                    buffer[i++]=c;
                    putchar(c);
                }
            } 
        }
        else{
            buffer[i]=0;
            clearPromt(i);
            if((!strcmp(buffer,"1jug"))||(!strcmp(buffer,"1"))){
                jugadores=1;
            }
            else if((!strcmp(buffer,"2jug"))||(!strcmp(buffer,"2"))){
                jugadores=2;
            }
            else if(!strcmp(buffer,"quit")){
                quit=1;
            }else if(!strcmp(buffer,"play")){
                play(jugadores);
            }
            i=0;
        }
    }
}

void play(int jug){
    if(played==0){
        played=1;
    }
    else{
        printTablero();
    }
    if(jug==1){
        sigleplayer();
    }else{
        pvpMode();
    }
}

void sigleplayer(){
    putchar(8);
    int map[11][11]={0};
    int snk1[11*11][2];
    for(int i=0; i<3;i++){
        snk1[i][X]=STARTING_POS_X+i;
        snk1[i][Y]=STARTING_POS_Y;
        printCuadradoColor(STARTING_POS_X+i,STARTING_POS_Y, 0x00ff8000);
        map[STARTING_POS_X+i][STARTING_POS_Y]=OCCUPIED;
    }
    int len1=2;
    int crashed=0;
    int apple_x=8;
    int apple_y=5;
    printCuadradoColor(apple_x,apple_y,ROJO);
    int direc[][2]={{1,0},{-1,0},{0,1},{0,-1}};
    int dir=0;
    int cola=0;
    int siguiente;
    char pressed;
    int last_dir=0;
    int points=0;
    print("jug1 ");
    printBase(points,10);
    while(crashed==0){
        sleepUser(TIME_INTERVAL);
        while((pressed=getcharNonLoop())!=-1){
            if(pressed=='w'&&last_dir!=2){
                dir=3;
            }
            else if(pressed=='a'&&last_dir!=0){
                dir=1;
            }
            else if(pressed=='s'&&last_dir!=3){
                dir=2;
            }
            else if(pressed=='d'&&last_dir!=1){
                dir=0;
            } 
        }
        if(!isValidPos1J(snk1[len1][X]+direc[dir][X],snk1[len1][Y]+direc[dir][Y],snk1, len1, cola)){
            sound(LOSE);
            crashed=1;
        }
        else{
            if(snk1[len1][X]+direc[dir][X]==apple_x&&snk1[len1][Y]+direc[dir][Y]==apple_y){           
                sound(APPLE);
                siguiente=((len1+1)%(11*11));
                snk1[siguiente][X]=snk1[len1][X]+direc[dir][X];
                snk1[siguiente][Y]=snk1[len1][Y]+direc[dir][Y];
                len1=siguiente;
                printCuadradoColor(snk1[len1][X],snk1[len1][Y],0x00ff8000);
                map[snk1[len1][X]][snk1[len1][Y]]=OCCUPIED;
                PutManzana(&apple_x,&apple_y,map);
                cleanResult(4+checkPoints(points));
                points++;
                print("jug1 ");
                printBase(points,10);
            }
            else{
                siguiente=((len1+1)%(11*11));
                snk1[siguiente][X]=snk1[len1][X]+direc[dir][X];
                snk1[siguiente][Y]=snk1[len1][Y]+direc[dir][Y];
                len1=siguiente;
                printCuadradoColor(snk1[len1][X],snk1[len1][Y],0x00ff8000);
                map[snk1[len1][X]][snk1[len1][Y]]=OCCUPIED;
                printCuadrado(snk1[cola][X]+6,snk1[cola][Y]+4);
                map[snk1[cola][X]][snk1[cola][Y]]=FREED;
                cola=(cola+1)%(11*11);
            }
        }
        last_dir=dir;
    }
    cleanResult(4+checkPoints(points));
    putCharColor('>',0xFFCC0F,0);
}

int isValidPos1J(int x,int y,int snake[][2], int len, int cola){
    if(x<0||y<0||x>10||y>10){
        return 0;
    }
    for(int i=cola; i!=len+1; i++){
        i%=(11*11);
        if(x==snake[i][X]&&y==snake[i][Y]){
            return 0;
        }
    }
    return 1;
}

void pvpMode(){
    putchar(8);
    int snk1[11*11][2];
    int map[11][11]={0};

    for(int i=0; i<3;i++){
        snk1[i][X]=STARTING_POS_X+i;
        snk1[i][Y]=STARTING_POS_Y-1;
        printCuadradoColor(STARTING_POS_X+i,STARTING_POS_Y-1, 0x00ff8000);
        map[STARTING_POS_X+i][STARTING_POS_Y-1]=OCCUPIED;

    }
    int len1=2;
    int crashed=0;
    int apple_x=8;
    int apple_y=5;

    int loser=0;

    int snk2[11*11][2];
    for(int i=0; i<3;i++){
        snk2[i][X]=STARTING_POS_X+i;
        snk2[i][Y]=STARTING_POS_Y+1;
        printCuadradoColor(STARTING_POS_X+i,STARTING_POS_Y+1, CIAN);
        map[STARTING_POS_X+i][STARTING_POS_Y+1]=OCCUPIED;

    }
    int len2=2;

    printCuadradoColor(apple_x,apple_y,ROJO);
    int direc[][2]={{1,0},{-1,0},{0,1},{0,-1}};
    int dir=0;
    int cola=0;
    int dir2=0;
    int cola2=0;

    int siguiente;
    int points1=0;
    int points2=0;
    char pressed;
    int last_dir=0;
    int last_dir2=0;
    print("jug1 ");
    printBase(points1,10);
    print(" jug2 ");
    printBase(points2,10);
    while(crashed==0){
        sleepUser(TIME_INTERVAL);
        while((pressed=getcharNonLoop())!=-1){
            if(pressed=='w'&&last_dir!=2){
                dir=3;
            }
            else if(pressed=='a'&&last_dir!=0){
                dir=1;
            }
            else if(pressed=='s'&&last_dir!=3){
                dir=2;
            }
            else if(pressed=='d'&&last_dir!=1){
                dir=0;
            }else if(pressed=='i'&&last_dir2!=2){
                dir2=3;
            }
            else if(pressed=='j'&&last_dir2!=0){
                dir2=1;
            }
            else if(pressed=='k'&&last_dir2!=3){
                dir2=2;
            }
            else if(pressed=='l'&&last_dir2!=1){
                dir2=0;
            } 
        }
        if(snk1[len1][X]+direc[dir][X]==apple_x&&snk1[len1][Y]+direc[dir][Y]==apple_y){           
            sound(APPLE);
            siguiente=((len1+1)%(11*11));
            snk1[siguiente][X]=snk1[len1][X]+direc[dir][X];
            snk1[siguiente][Y]=snk1[len1][Y]+direc[dir][Y];
            len1=siguiente;
            printCuadradoColor(snk1[len1][X],snk1[len1][Y],0x00ff8000);
            map[snk1[len1][X]][snk1[len1][Y]]=OCCUPIED;
            PutManzana(&apple_x,&apple_y,map);
            cleanResult(4+2+checkPoints(points1)+4+checkPoints(points2));
            points1++;
            print("jug1 ");
            printBase(points1,10);
            print(" jug2 ");
            printBase(points2,10);

        }
        else{
            siguiente=((len1+1)%(11*11));
            snk1[siguiente][X]=snk1[len1][X]+direc[dir][X];
            snk1[siguiente][Y]=snk1[len1][Y]+direc[dir][Y];
            len1=siguiente;
            printCuadradoColor(snk1[len1][X],snk1[len1][Y],0x00ff8000);
            map[snk1[len1][X]][snk1[len1][Y]]=OCCUPIED;
            printCuadrado(snk1[cola][X]+6,snk1[cola][Y]+4);
            map[snk1[cola][X]][snk1[cola][Y]]=FREED;
            cola=(cola+1)%(11*11);
        }
        if(snk2[len2][X]+direc[dir2][X]==apple_x&&snk2[len2][Y]+direc[dir2][Y]==apple_y){           
            sound(APPLE);      
            siguiente=((len2+1)%(11*11));
            snk2[siguiente][X]=snk2[len2][X]+direc[dir2][X];
            snk2[siguiente][Y]=snk2[len2][Y]+direc[dir2][Y];
            len2=siguiente;
            printCuadradoColor(snk2[len2][X],snk2[len2][Y],CIAN);
            map[snk2[len2][X]][snk2[len2][Y]]=OCCUPIED;
            PutManzana(&apple_x,&apple_y,map);
            cleanResult(4+2+checkPoints(points1)+4+checkPoints(points2));
            points2++;
            print("jug1 ");
            printBase(points1,10);
            print(" jug2 ");
            printBase(points2,10);
            }
        else{
            siguiente=((len2+1)%(11*11));
            snk2[siguiente][X]=snk2[len2][X]+direc[dir2][X];
            snk2[siguiente][Y]=snk2[len2][Y]+direc[dir2][Y];
            len2=siguiente;
            printCuadradoColor(snk2[len2][X],snk2[len2][Y],CIAN);
            map[snk2[len2][X]][snk2[len2][Y]]=OCCUPIED;
            printCuadrado(snk2[cola2][X]+6,snk2[cola2][Y]+4);
            map[snk2[cola2][X]][snk2[cola2][Y]]=FREED;
            cola2=(cola2+1)%(11*11);
        }        
        if(!isValidPos2J(snk1[len1][X]+direc[dir][X],snk1[len1][Y]+direc[dir][Y],snk1, len1, cola,snk2,len2,cola2)){
            loser=1;
            crashed=1;
        }
        if(!isValidPos2J(snk2[len2][X]+direc[dir2][X],snk2[len2][Y]+direc[dir2][Y],snk2, len2, cola2,snk1,len1,cola)){
            loser+=2;
            crashed=2;
        }
        last_dir=dir;
        last_dir2=dir2;
    }
    cleanResult(4+2+checkPoints(points1)+4+checkPoints(points2));
    if(loser==1){
        sound(WINNER2);
        cleanResult(last_result_len);
        printColor("GANADOR: Jugador2\n",VIOLETA,0);
        putCharColor('>',0xFFCC0F,0);
        last_result_len=16;
    }else if(loser==2){
        sound(WINNER1);
        cleanResult(last_result_len);
        printColor("GANADOR: Jugador1\n",VIOLETA,0);
        putCharColor('>',0xFFCC0F,0);
        last_result_len=16;
    }else{
        sound(TIE);
        cleanResult(last_result_len);
        printColor("EMPATE\n",VIOLETA,0);
        putCharColor('>',0xFFCC0F,0);
        last_result_len=5;
    }
    
}

void cleanResult(int len){
    for(int i=0; i<=len; i++){
        putchar(8);
    }
}

int checkPoints(int points){
    if(points>=10&&points<100){
        return 2;
    }
    if(points>=100){
        return 3;
    }
    if(points>=0&&points<10){
        return 1;
    }
    return 4;
}

int isValidPos2J(int x,int y,int snake[][2], int len, int cola,int snake2[][2], int len2, int cola2){
    if(x<-1||y<-1||x>11||y>11){
        return 0;
    }
    for(int i=cola; i!=len+1; i++){
        i%=(11*11);
        if(x==snake[i][X]&&y==snake[i][Y]){
            return 0;
        }
    }
    for(int i=cola2; i!=len2+1; i++){
        i%=(11*11);
        if(x==snake2[i][X]&&y==snake2[i][Y]){
            return 0;
        }
    }
    return 1;
}

void printCuadradoColor(int x, int y, int color){
    x+=6;
    y+=4;
    for(int i=0; i<45; i++){
        for(int j=0; j<45; j++){
            putPixel(color,x*45+i,y*45+j);
        }
    }
}


void clearPromt(int len){
    for(int i=0; i<len; i++){
        putchar(8);
    }
}

void printTablero(){
    printBordeHorizontal(3);
    for(int i=4;i<15;i++){ 
        printAS(5,i);     //filas del 1 al 14
        for(int j=6;j<17;j++){
            printCuadrado(j,i);
        }
        printAS(17,i);
    }
    printBordeHorizontal(15);
}

void printBordeHorizontal(int fila){
    for(int i=5;i<18;i++){
        printAS(i,fila);
    }
}

void printCuadrado(int col, int fila){
    int color;
    if(fila%2==0){
        if(col%2==0){
            color= VERDECLARO;
        }else{
            color= VERDEOSCURO;
        }
    }
    else{
        if(col%2==0){
            color=VERDEOSCURO;
        }else{
            color=VERDECLARO;
        }
    }
    for(int i=0;i<45;i++){
        for(int j=0;j<45;j++){
            putPixel(color,i+col*45,j+fila*45);
        }
    }
}


void printAS(int col, int fila){
    int color;
    for(int i=0;i<45;i++){
        for(int j=0;j<45;j++){
            color= ((j+i)%2==0)? VIOLETA: GRIS;
            putPixel(color,i+col*45,j+fila*45);
        }
    }
}

void PutManzana(int * apple_x, int * apple_y,int map[11][11]){
    uint64_t a;
    uint64_t b;
    int x;
    int y;
    int flag=0;
    do{
        a=getMS();
        b=2*(a+5);
        x=numeroAleatorioEntre(0,10, &a);
        y=numeroAleatorioEntre(0,10, &b);
        if(map[x][y]==FREED){
            flag=1;
        }
    }while(!flag);
    *apple_x=x;
    *apple_y=y;

    printCuadradoColor(x,y,ROJO);
}

