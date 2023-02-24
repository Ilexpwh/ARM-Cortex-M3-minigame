#include "stm32f10x.h"
#include "IERG3810_Buzzer.h"

// put your procedure and code here
void IERG3810_Buzzer_Init(){
		//Buzzer PB8
		RCC ->APB2ENR |= 1 << 3; // enable port B clock
		GPIOB->CRH &= 0xFFFFFFF0; // pin 0
		GPIOB->CRH |= 0x00000003; //general purpose push-pull 0011
		GPIOB->BRR |= 1 << 8;
}