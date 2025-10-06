#include <sound.h>

//Función hecha en asm dentro de Kernel\asm\libasm.asm
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

//Play sound using built-in speaker
static void play_sound(uint32_t nFrequence) {
	uint32_t Div;
	uint8_t tmp;

	//Set the PIT to the desired frequency
	Div = 1193180 / nFrequence;
	outb(0x43, 0xb6);
	outb(0x42, (uint8_t) (Div) );
	outb(0x42, (uint8_t) (Div >> 8));

	//And play the sound using the PC speaker
	tmp = inb(0x61);
	if (tmp != (tmp | 3)) {
		outb(0x61, tmp | 3);
	}
}

//make it shut up
static void nosound() {
	uint8_t tmp = inb(0x61) & 0xFC;
	outb(0x61, tmp);
}

//Make a beep
void soundBeep() {
	play_sound(1000);
	sleep(3);
	nosound();
}

void soundWelcome() {
	play_sound(400);
	sleep(1);
	nosound();
	play_sound(500);
	sleep(1);
	nosound();
	play_sound(600);
	sleep(2);
	nosound();
}

void soundExit() {
	play_sound(600);
	sleep(1);
	nosound();
	play_sound(500);
	sleep(1);
	nosound();
	play_sound(400);
	sleep(2);
	nosound();
}

void soundApple() {
    play_sound(800);
    sleep(1);
	nosound();
    play_sound(1000);
    sleep(2);
    nosound();
}

void soundWinner1() {
	play_sound(700);
    sleep(1);
    nosound();
    play_sound(800);
    sleep(1);
    nosound();
    play_sound(900);
    sleep(2);
    nosound();
}

void soundWinner2() {
	play_sound(800);
    sleep(1);
    nosound();
    play_sound(900);
    sleep(1);
    nosound();
    play_sound(1000);
    sleep(2);
    nosound();
}

void soundGameOver() {
    play_sound(300);
    sleep(1);
	nosound();
    play_sound(200);
    sleep(1);
    nosound();
    play_sound(100);
    sleep(2);
    nosound();
}

void soundTie() {
    play_sound(500);
    sleep(1);
	nosound();
    play_sound(600);
    sleep(1);
    nosound();
    play_sound(700);
    sleep(2);
    nosound();
}


void playSoundSpeaker(int index) {
	if(index==0){
		soundBeep();
	}
	if(index==1){
		soundWelcome();
	}
	if(index==2){
		soundExit();
	}
	if(index==3){
		soundApple();
	}
	if(index==4){
		soundWinner1();
	}
	if(index==5){
		soundWinner2();
	}
	if(index==6){
		soundGameOver();
	}
	if(index==7){
		soundTie();
	}
}