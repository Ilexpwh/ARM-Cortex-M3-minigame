#include "stm32f10x.h"
#include "IERG3810_KEY.h"

// put your procedure and code here

void IERG3810_KEY_Init(){
	  	//KEY2 PE2
		//KEY2 Input Press = Low PE2
		RCC ->APB2ENR |= 1 << 6; // enable port E clock
		GPIOE->CRL &= 0xFFFFF0FF;	//pin 2
		GPIOE->CRL |= 0x00000800;	//interal pull up
		GPIOE->BSRR |= 1<<2;
	
		//KEY1 PE3
		//KEY1 Input Press = Low PE3
		RCC ->APB2ENR |= 1 << 6; // enable port E clock
		GPIOE->CRL &= 0xFFFF0FFF;	//pin 3
		GPIOE->CRL |= 0x00008000;	//interal pull up
		GPIOE->BSRR |= 1<<3;
	
		//KEY_UP PA0
		//KEY_UP Input Press = High PA0
		RCC ->APB2ENR |= 1 << 2; // enable port A clock
		GPIOA->CRL &= 0xFFFFFFF0;	//pin 0
		GPIOA->CRL |= 0x00000008;	//interal pull up
		GPIOA->BRR |= 1<<0;
}
