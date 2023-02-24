#include "stm32f10x.h"
#include "IERG3810_LED.h"

// put your procedure and code here
void IERG3810_LED_Init(){
	  //DS0 (LED0)  PB5
		RCC ->APB2ENR |= 1 << 3; // enable port B clock
		GPIOB->CRL &= 0xFF0FFFFF; // pin5
		GPIOB->CRL |= 0x00300000; //general purpose output push-pull
	//DS1 (LED1)  PE5
		RCC ->APB2ENR |= 1 << 6; // enable port E clock
		GPIOE->CRL &= 0xFF0FFFFF; // pin5
		GPIOE->CRL |= 0x00300000; //general purpose output push-pull
		GPIOE->BSRR = 1 << 5;
	
}