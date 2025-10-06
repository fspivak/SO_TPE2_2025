#include <clock.h>

#define TIME_ZONE -3

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base){
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	//Calculate characters for each digit
	do{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	}while (value /= base);

	// Terminate string in buffer.
	*p = 0;

	//Reverse string in buffer.
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2){
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}

static void addFrontZero(char * buffer, uint64_t value) {
	if(uintToBase(value, buffer,10)==1) {
		buffer[2]=0; // Null Terminated
        buffer[1]=buffer[0];
        buffer[0]='0';
	}
}

// Decodifica el valor binario a decimal
static uint64_t decode(uint64_t time) {
    return (time >> 4) * 10 + (time & 0x0F);
}

void getClockTime(char * str) {
	int idx = 0;
    uint64_t hours = decode(getHours()) + TIME_ZONE;
    hours = (hours < 0) ? (24 + hours) : (hours % 24);  // Me aseguro que hours este entre 0-23
    addFrontZero(&str[idx], hours);
    idx += 2;
    str[idx++] = ':';
    addFrontZero(&str[idx], decode(getMinutes()));
    idx += 2;
    str[idx++] = ':';
    addFrontZero(&str[idx], decode(getSeconds()));
    idx += 2;
    str[idx] = '\0';  // Null Terminated
}