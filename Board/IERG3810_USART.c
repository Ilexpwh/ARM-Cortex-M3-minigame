//IERG3810_USART.c
#include "stm32f10x.h"
#include "IERG3810_USART.h"

// put your procedure and code here

void IERG3810_USART2_init(u32 pclkl, u32 bound)
{
	//USART2
	float temp;
	u16 mantissa;
	u16 fraction;
	temp=(float)(pclkl*1000000)/(bound*16);
	mantissa=temp;	
	fraction = (temp-mantissa)*16;
		mantissa<<=4;
	mantissa +=fraction;
	RCC->APB2ENR|=1<<2; //enable IOPA
	RCC->APB1ENR|=1<<17; //enable USART2
	GPIOA->CRL&=0xFFFF00FF; //set PA2,PA3 Alternate Function
	GPIOA->CRL|=0x00008B00;//set PA2,PA3 Alternate Function
	RCC->APB1RSTR|=1<<17; //USART2RST
	RCC->APB1RSTR&=~(1<<17);
	USART2->BRR=mantissa;
	USART2->CR1|=0x2008;
}

void IERG3810_USART1_init(u32 pclkl, u32 bound)
{
	//USART1
	float temp;
	u16 mantissa;
	u16 fraction;
	temp=(float)(pclkl*1000000)/(bound*16);
	mantissa=temp;	
	fraction = (temp-mantissa)*16;
		mantissa<<=4;
	mantissa +=fraction;
	RCC->APB2ENR|=1<<2; //enable IOPA
	RCC->APB2ENR|=1<<14; //enable USART1EN
	GPIOA->CRH&=0xFFFFF00F;//set PA9,PA10 Alternate Function
	GPIOA->CRH|=0x000008B0;//set the input and output of PA9,PA10
	RCC->APB2RSTR|=1<<14; //USART1RST
	RCC->APB2RSTR&=~(1<<14);
	USART1->BRR=mantissa;
	USART1->CR1|=0x2008;
}

void USART_print(u8 USARTport, char *st)
{
	u8 i=0;
	while(st[i] !=0x00)
	{
	if (USARTport == 1)	{
	USART1->DR=st[i];
	while(!(USART1->SR &(1<<7)));
	}
	if (USARTport == 2)	{
	USART2->DR=st[i];
	while(!(USART2->SR &(1<<7)));
	}
	//Delay(50000);
	if(i==255) break;
	i++;
	}
}