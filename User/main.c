#include "stm32f10x.h"
#include "FONT.H"
#include "CFONT.H"
#include "IERG3810_LED.h"
#include "IERG3810_KEY.h"
#include "IERG3810_Buzzer.h"
#include "IERG3810_USART.h"
#include "IERG3810_Clock.h"


void IERG3810_TFTLCD_Init(void);
void IERG3810_TFTLCD_SetParameter(void);
void IERG3810_TFTLCD_WrReg(u16 regval);
void IERG3810_TFTLCD_WrData(u16 data);
void IERG3810_TFTLCD_FillRectangle(u16 color, u16 start_x, u16 length_x, u16 start_y, u16 length_y);
void IERG3810_TFTLCD_ShowChar(u16 x,u16 y,u8 ascii, u16 color, u16 bgcolor);
void IERG3810_TFTLCD_SevenSegment(u16 color, u16 start_x, u16 start_y, u8 digit);
void IERG3810_TFTLCD_ShowChinChar(u16 x,u16 y,u8 code, u16 color, u16 bgcolor);

//u8 ps2key=0x00; 
int GameStarted;//game started? 0 == no
int Gamestatus; // Game status? 0 == no
int score;//score
int doOnce = 0;
u32 ps2count=0;
u32 timeout = 20000;
u32 ps2key =0;//current key press
u32 dataread=0;
typedef enum {FALSE = 0,TRUE = 1} bool;
bool released=FALSE;

#define Black           0x0000      /*   0,   0,   0 */
void Delay(u32 count)
 {
	u32 i;
	for (i=0; i<count; i++);
 }
 
typedef struct
 {
	 u16 LCD_REG;
	 u16 LCD_RAM;	 
 } LCD_TypeDef;

#define LCD_BASE  ((u32)(0x6C000000 | 0x000007FE))
#define LCD       ((LCD_TypeDef *) LCD_BASE)
#define LCD_LIGHT_ON GPIOB->BSRR |= 000000001;

 //key2
void IERG3810_key2_ExtiInit(void)
{
	//KEY2 at PE2, EXTI-2, IRG#8
	RCC->APB2ENR |= 1<<6;
	GPIOE->CRL &= 0xFFFFF0FF;
	GPIOE->CRL |= 0x00000800;
	GPIOE->ODR |= 1<<2;
	RCC->APB2ENR |= 0x01;
	AFIO->EXTICR[0] &= 0xFFFFF0FF;
	AFIO->EXTICR[0] |= 0x00000400;
	EXTI->IMR |= 1<<2;
	EXTI->FTSR |= 1<<2;
	//EXIT->RTSR |= 1<<2;
	
	NVIC->IP[8] = 0x65;
	NVIC->ISER[0] |= (1<<8);
}
 //keyPad
void IERG3810_PS2key_ExtiInit(void)
{
	RCC->APB2ENR|=1<<4;
	GPIOC->CRH &= 0xFFFF0FFF;
	GPIOC->CRH |= 0x00008000;
	GPIOC->ODR |= 1<<11;
	
	
	RCC->APB2ENR |= 0x01;
	AFIO->EXTICR[2] &= 0xFFFF0FFF; // p185
	AFIO->EXTICR[2] |= 0x00002000;
	EXTI->IMR |= 1<<11;
	EXTI->FTSR |=1<<11;

	NVIC->IP[40] = 0x95;
	NVIC->ISER[1] |= (1<<8);
	
	//pc10 data
	RCC->APB2ENR|=1<<4;
	GPIOC->CRH &= 0xFFFFF0FF;
	GPIOC->CRH |= 0x00000800;
}
//Key up
void IERG3810_keyUP_ExtiInit(void)
{
    RCC->APB2ENR|=1<<2;       //pot A 0
    GPIOA->CRL&=0xFFFFFFF0;
    GPIOA->CRL|=0x00000008;
    GPIOA->ODR|=0<<0;
    RCC->APB2ENR|=0x01;
    AFIO->EXTICR[0]&=0xFFFFFFF0;
    AFIO->EXTICR[0]|=0x00000000;//set pinE as input source
    EXTI->IMR|=1<<0;
    EXTI->FTSR|=1<<0;
    
    NVIC->IP[8]=0x75;
    NVIC->ISER[0]|=(1<<6);
}
void IERG3810_NVIC_SetPriorityGroup(u8 prigroup){
	//set prigroup AIRCR[10:8]
	u32 temp, temp1;
	temp1 = prigroup & 0x00000007; // only concern 3 bits
	temp1 <<= 8; //why?
	temp = SCB->AIRCR;
	temp &= 0X0000F8FF;
  temp = 0x05FA0000;
	temp |= temp1;
	SCB->AIRCR = temp;
}
void IERG3810_SYSTICK_Init10ms(void){
	//SYSTICK
	SysTick->CTRL = 0; //Clear Systick->CTRL setting
	SysTick->LOAD = 89999; //what should be filled? refer to 0337E page 8-10
	                       //72M / (8*100) - 1  
	                       //10ms -> 100 Hz
	
	//CLKSOURCE=0 : STCLK (FCLK/8)
	//CLKSOURCE = 1: FCLK
	//CLKSOURCE=0 is synchronized and better than CLKSROUCE = 1
	//Refer to Clock tree on page-93 of RM0008
	SysTick->CTRL |= 0x03; //what should be filled?
	                      //refer to 033tE page 8-8
}

void IERG3810_TFTLCD_Init(void)
{
	RCC->AHBENR |= 1<<8;   //FSMC
	RCC->APB2ENR |= 1<<3;  //PORTB
	RCC->APB2ENR |= 1<<5;  //PORTD
	RCC->APB2ENR |= 1<<6;  //PORTE
	RCC->APB2ENR |= 1<<8;   //PORTG
	GPIOB->CRL &= 0xFFFFFFF0;  //PBO
	GPIOB->CRL |= 0x00000003;
	
	//PORTD
	GPIOD->CRH &= 0x00FFF000;
	GPIOD->CRH |= 0xBB000BBB;
	GPIOD->CRL &= 0xFF00FF00;
	GPIOD->CRL |= 0x00BB00BB;
	//PORTE
	GPIOE->CRH &= 0x00000000;
	GPIOE->CRH |= 0xBBBBBBBB;
	GPIOE->CRL &= 0x0FFFFFFF;
	GPIOE->CRL |= 0xB0000000;
	//PORTG12
	GPIOG->CRH &= 0xFFF0FFFF;
	GPIOG->CRH |= 0x000B0000;
	GPIOG->CRL &= 0xFFFFFFF0;
	GPIOG->CRL |= 0x0000000B;
	
	//LCD uses FSMC Bank 4 memory bank
	//Use Mode A
	FSMC_Bank1->BTCR[6]=0x00000000;
	FSMC_Bank1->BTCR[7]=0x00000000;
	FSMC_Bank1E->BWTR[6]=0x00000000;
	FSMC_Bank1->BTCR[6] |= 1<<12;
	FSMC_Bank1->BTCR[6] |= 1<<14;
	FSMC_Bank1->BTCR[6] |= 1<<4;
	FSMC_Bank1->BTCR[7] |= 0<<28;
	FSMC_Bank1->BTCR[7] |= 1<<0;
	FSMC_Bank1->BTCR[7] |= 0xF<<8;
	FSMC_Bank1E->BWTR[6] |= 0<<28;
	FSMC_Bank1E->BWTR[6] |= 0<<0;
	FSMC_Bank1E->BWTR[6] |= 3<<8;
	FSMC_Bank1->BTCR[6] |= 1<<0;
	IERG3810_TFTLCD_SetParameter();
	LCD_LIGHT_ON;
}	

 void IERG3810_TFTLCD_SetParameter(void){
	
IERG3810_TFTLCD_WrReg(0x01); //Software reset
IERG3810_TFTLCD_WrReg(0x11); //Exit_sleep_mode
	
IERG3810_TFTLCD_WrReg(0x3A);//Set_pixel_format
IERG3810_TFTLCD_WrData(0x55);//65536 colors

IERG3810_TFTLCD_WrReg(0x29);//Display On

IERG3810_TFTLCD_WrReg(0x36);//Memory Access Control(section 8.2.29, page127)
IERG3810_TFTLCD_WrData(0xCA);

}
 


void IERG3810_TFTLCD_WrReg(u16 regval){
	LCD->LCD_REG=regval;
}

void IERG3810_TFTLCD_WrData(u16 data){
	LCD->LCD_RAM=data;
}
void IERG3810_TFTLCD_DrawDot(u16 x,u16 y,u16 color)
	{
	IERG3810_TFTLCD_WrReg(0x2A);//set x position
		IERG3810_TFTLCD_WrData(x>>8);
		IERG3810_TFTLCD_WrData(x & 0xFF);
		IERG3810_TFTLCD_WrData(0x01);
		IERG3810_TFTLCD_WrData(0x3F);
	IERG3810_TFTLCD_WrReg(0x2B);//set y position
		IERG3810_TFTLCD_WrData(y>>8);
		IERG3810_TFTLCD_WrData(y & 0xFF);
		IERG3810_TFTLCD_WrData(0x01);
		IERG3810_TFTLCD_WrData(0xDF);
	IERG3810_TFTLCD_WrReg(0x2C);//set point with color
		IERG3810_TFTLCD_WrData(color);
}

void IERG3810_TFTLCD_FillRectangle(u16 color, u16 start_x, u16 length_x, u16 start_y, u16 length_y)
	{
		u32 index=0;
	IERG3810_TFTLCD_WrReg (0x2A);
		IERG3810_TFTLCD_WrData(start_x>>8);
		IERG3810_TFTLCD_WrData(start_x & 0xFF);
		IERG3810_TFTLCD_WrData ((length_x + start_x - 1) >> 8);
		IERG3810_TFTLCD_WrData((length_x + start_x - 1) & 0xFF);
	IERG3810_TFTLCD_WrReg(0x2B);
		IERG3810_TFTLCD_WrData(start_y>>8);
		IERG3810_TFTLCD_WrData(start_y & 0xFF);
		IERG3810_TFTLCD_WrData((length_y + start_y - 1) >> 8);
		IERG3810_TFTLCD_WrData((length_y + start_y - 1) & 0xFF);
	IERG3810_TFTLCD_WrReg(0x2C);
		for (index=0;index<length_x * length_y;index++)
		{
		IERG3810_TFTLCD_WrData(color);
		}
}
void IERG3810_TFTLCD_ShowChar(u16 x,u16 y,u8 ascii, u16 color, u16 bgcolor){
	u8 i,j;
	u8 index;
	u8 height = 16, length = 8;
	if(ascii < 32 || ascii > 127) return;
	ascii -= 32;
	IERG3810_TFTLCD_WrReg(0x2A);
	IERG3810_TFTLCD_WrData(x>>8);
	IERG3810_TFTLCD_WrData(x & 0xFF);
	IERG3810_TFTLCD_WrData((length + x - 1)>>8);
	IERG3810_TFTLCD_WrData((length + x - 1) & 0xFF);
	
	IERG3810_TFTLCD_WrReg(0x2B);
	IERG3810_TFTLCD_WrData(y>>8);
	IERG3810_TFTLCD_WrData(y & 0xFF);
	IERG3810_TFTLCD_WrData((height + y - 1)>>8);
	IERG3810_TFTLCD_WrData((height + y - 1) & 0xFF);
	
	IERG3810_TFTLCD_WrReg(0x2C);
	
	for(j=0; j<height/8; j++){
		
		for(i=0; i<height/2; i++){
			
			for(index=0; index<length;index++){
				if((asc2_1608[ascii][index*2+1-j]>>i) & 0x01)
					IERG3810_TFTLCD_WrData(color);
			  else
					IERG3810_TFTLCD_WrData(bgcolor);
				
			}
		}
	}
	
}

void IERG3810_TFTLCD_ShowChinChar(u16 x,u16 y,u8 code, u16 color, u16 bgcolor){
	u8 i,j;
	u8 index;
	u8 height = 16, length = 16;
	if(code >= 0x0A) return;
	
	IERG3810_TFTLCD_WrReg(0x2A);
	IERG3810_TFTLCD_WrData(x>>8);
	IERG3810_TFTLCD_WrData(x & 0xFF);
	IERG3810_TFTLCD_WrData((length + x - 1)>>8);
	IERG3810_TFTLCD_WrData((length + x - 1) & 0xFF);
	
	IERG3810_TFTLCD_WrReg(0x2B);
	IERG3810_TFTLCD_WrData(y>>8);
	IERG3810_TFTLCD_WrData(y & 0xFF);
	IERG3810_TFTLCD_WrData((height + y - 1)>>8);
	IERG3810_TFTLCD_WrData((height + y - 1) & 0xFF);
	
	IERG3810_TFTLCD_WrReg(0x2C);
	
	for(j=0; j<height/8; j++){
		
		for(i=0; i<height/2; i++){
			
			for(index=0; index<length;index++){
				if((C_1616[code][index*2+1-j]>>i) & 0x01)
					IERG3810_TFTLCD_WrData(color);
			  else
					IERG3810_TFTLCD_WrData(bgcolor);
				
			}
		}
	}
	
}

void IERG3810_TFTLCD_SevenSegment(u16 color, u16 start_x, u16 start_y, u8 pattern){

	 if((pattern & 0x01) >>0 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x+0x000A,0x003C,start_y+0x0096, 0x000A); //segment a
	 if((pattern & 0x02) >> 1 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x+0x0046,0x000A,start_y+0x0055, 0x0041); //segment b
	 if((pattern & 0x04) >> 2 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x+0x0046,0x000A,start_y+0x000A, 0x0041); //segment c
	 if((pattern & 0x08)>>3 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x+0x000A,0x003C,start_y, 0x000A); //segment d
	 if((pattern & 0x10) >>4 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x,0x000A,start_y+0x000A, 0x0041); //segment e
	 if((pattern & 0x20) >> 5 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x,0x000A,start_y+0x0055, 0x0041); //segment f
	 if((pattern & 0x40) >> 6 == 1)
			IERG3810_TFTLCD_FillRectangle(color,start_x+0x000A,0x003C,start_y+0x004B, 0x000A); //segment g
			
}

void IERG3810_TFTLCD_DrawDigit(u16 color, u16 start_x, u16 start_y, u8 digit){
		switch(digit){
			case(0x00):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x3F);
				break;
			case(0x01):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x06);
				break;
			case(0x02):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x5B);
				break;
			case(0x03):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x4F);
				break;
			case(0x04):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x66);
				break;
			case(0x05):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x6D);
				break;
			case(0x06):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x7D);
				break;
			case(0x07):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x07);
				break;
			case(0x08):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x7F);
				break;
			case(0x09):
				IERG3810_TFTLCD_SevenSegment(color,start_x, start_y,0x6F);
				break;
		}
}

void IERG3810_TFTLCD_ShowCharInLine(u16 x,u16 y,u8* str,u16 str_size, u16 color, u16 bgcolor){
		u16 i;
	  for(i=0x0; i < str_size; i++){
			IERG3810_TFTLCD_ShowChar(x+ i * 0x0008,y,str[i],color,bgcolor);
		}
}
void IERG3810_TFTLCD_ShowChinCharInLine(u16 x,u16 y,u8* str,u16 str_size, u16 color, u16 bgcolor){
		u16 i;
	  for(i=0x0; i < str_size; i++){
			IERG3810_TFTLCD_ShowChinChar(x+ i * 0x0010,y,str[i],color,bgcolor);
		}
}


void drawMole(int whichmole){
	u16 x=400; 
	u16 y=400;
	u8 i,j;
	
		u8 Mole[16][13]={
{0,0,0,0,1,1,1,1,1,0,0,0,0},
{0,0,0,1,4,4,4,4,4,1,0,0,0},
{0,0,1,4,4,4,4,4,4,4,1,0,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,1,4,4,2,4,4,4,2,4,4,1,0},
{0,1,4,4,2,4,4,4,2,4,4,1,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,1,4,4,4,4,3,4,4,4,4,1,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,5,5,5,5,5,5,5,5,5,5,5,0},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{0,5,5,5,5,5,5,5,5,5,5,5,0},

};
	

	if(whichmole == 1){
	x=70;
	y=80;
	}
	if(whichmole == 2){
	x=140;
	y=80;
	}
	if(whichmole == 3){
	x=210;
	y=80;
	}
	if(whichmole == 4){
	x=70;
	y=160;
	}
	if(whichmole == 5){
	x=140;
	y=160;
	}
	if(whichmole == 6){
	x=210;
	y=160;
	}
	if(whichmole == 7){
	x=70;
	y=240;
	}
	if(whichmole == 8){
	x=140;
	y=240;
	}
	if(whichmole == 9){
	x=210;
	y=240;
	}
	if(whichmole == 10){
	x=210;
	y=310;
	}
	

	for(i=0;i<13;i++){
	for(j=0;j<16;j++){
		if (Mole[j][i] == 1){IERG3810_TFTLCD_FillRectangle( 0x7BEF,x-i*3,3,y-j*3,3);}
		if (Mole[j][i] == 2){IERG3810_TFTLCD_FillRectangle( 0x0000,x-i*3,3,y-j*3,3);}
		if (Mole[j][i] == 3){IERG3810_TFTLCD_FillRectangle( 0xF800,x-i*3,3,y-j*3,3);}
		if (Mole[j][i] == 4){IERG3810_TFTLCD_FillRectangle( 0xFFE0,x-i*3,3,y-j*3,3);}
		if (Mole[j][i] == 5){IERG3810_TFTLCD_FillRectangle( 0x79E0,x-i*3,3,y-j*3,3);}
		if (Mole[j][i] == 0){IERG3810_TFTLCD_FillRectangle( 0x7E0,x-i*3,3,y-j*3,3);}
	}
}
}

void drawHurtMole(int whichmole){
	u16 x=400; 
	u16 y=400;
	u8 i,j;

	u8 HurtMole[16][13]={
{0,0,0,0,1,1,1,1,1,0,0,0,0},
{0,0,0,1,4,4,4,4,4,1,0,0,0},
{0,0,1,4,4,4,4,4,4,4,1,0,0},
{0,1,4,4,4,4,4,4,4,4,4,1,0},
{0,1,4,2,4,4,4,4,4,2,4,1,0},
{0,1,4,4,2,4,4,4,2,4,4,1,0},
{0,1,4,2,4,4,4,4,4,2,4,1,0},
{0,1,4,4,4,3,3,3,4,4,4,1,0},
{0,1,6,4,4,3,3,3,4,4,6,1,0},
{0,6,4,4,4,3,3,3,4,4,4,6,0},
{6,1,4,4,4,4,4,4,4,4,4,1,6},
{0,5,5,5,5,5,5,5,5,5,5,5,0},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{0,5,5,5,5,5,5,5,5,5,5,5,0},

};
	
if(whichmole == 1){
	x=70;
	y=80;
	}
	if(whichmole == 2){
	x=140;
	y=80;
	}
	if(whichmole == 3){
	x=210;
	y=80;
	}
	if(whichmole == 4){
	x=70;
	y=160;
	}
	if(whichmole == 5){
	x=140;
	y=160;
	}
	if(whichmole == 6){
	x=210;
	y=160;
	}
	if(whichmole == 7){
	x=70;
	y=240;
	}
	if(whichmole == 8){
	x=140;
	y=240;
	}
	if(whichmole == 9){
	x=210;
	y=240;
	}
	
	for(i=0;i<13;i++){
	for(j=0;j<16;j++){
		if (HurtMole[j][i] == 1){IERG3810_TFTLCD_FillRectangle( 0x7BEF,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 2){IERG3810_TFTLCD_FillRectangle( 0x0000,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 3){IERG3810_TFTLCD_FillRectangle( 0xF800,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 4){IERG3810_TFTLCD_FillRectangle( 0xFFE0,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 5){IERG3810_TFTLCD_FillRectangle( 0x79E0,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 0){IERG3810_TFTLCD_FillRectangle( 0x7E0,x-i*3,3,y-j*3,3);}
		if (HurtMole[j][i] == 6){IERG3810_TFTLCD_FillRectangle( 0x7FF,x-i*3,3,y-j*3,3);}
	}
}
}

void drawNoMole(int whichmole){
	u16 x=400; 
	u16 y=400;
	u8 i,j;

	u8 NoMole[16][13]={
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,5,5,5,5,5,5,5,5,5,5,5,0},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{0,5,5,5,5,5,5,5,5,5,5,5,0},

};
	
	if(whichmole == 1){
	x=70;
	y=80;
	}
	if(whichmole == 2){
	x=140;
	y=80;
	}
	if(whichmole == 3){
	x=210;
	y=80;
	}
	if(whichmole == 4){
	x=70;
	y=160;
	}
	if(whichmole == 5){
	x=140;
	y=160;
	}
	if(whichmole == 6){
	x=210;
	y=160;
	}
	if(whichmole == 7){
	x=70;
	y=240;
	}
	if(whichmole == 8){
	x=140;
	y=240;
	}
	if(whichmole == 9){
	x=210;
	y=240;
	}
	
	for(i=0;i<13;i++){
	for(j=0;j<16;j++){
		
		if (NoMole[j][i] == 5){IERG3810_TFTLCD_FillRectangle( 0x79E0,x-i*3,3,y-j*3,3);}
		if (NoMole[j][i] == 0){IERG3810_TFTLCD_FillRectangle( 0x7E0,x-i*3,3,y-j*3,3);}
	}
}
}
void drawBomb(int whichmole){
	u16 x=400; 
	u16 y=400;
	u8 i,j;
	
	
	u8 drawBomb[16][13]={
{0,0,0,0,0,0,0,0,0,0,3,3,3},
{0,0,0,0,0,0,0,0,0,1,3,3,3},
{0,0,0,0,2,2,2,2,1,0,0,0,0},
{0,0,0,2,2,2,2,2,2,2,0,0,0},
{0,0,2,2,2,2,2,2,2,2,2,0,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,2,2,2,2,2,2,2,2,2,2,2,0},
{0,5,2,2,2,2,2,2,2,2,2,5,0},
{5,5,5,2,2,2,2,2,2,2,5,5,5},
{5,5,5,5,2,2,2,2,5,5,5,5,5},
{5,5,5,5,5,5,5,5,5,5,5,5,5},
{0,5,5,5,5,5,5,5,5,5,5,5,0},

};
	if(whichmole == 1){
	x=70;
	y=80;
	}
	if(whichmole == 2){
	x=140;
	y=80;
	}
	if(whichmole == 3){
	x=210;
	y=80;
	}
	if(whichmole == 4){
	x=70;
	y=160;
	}
	if(whichmole == 5){
	x=140;
	y=160;
	}
	if(whichmole == 6){
	x=210;
	y=160;
	}
	if(whichmole == 7){
	x=70;
	y=240;
	}
	if(whichmole == 8){
	x=140;
	y=240;
	}
	if(whichmole == 9){
	x=210;
	y=240;
	}
	
	for(i=0;i<13;i++){
	for(j=0;j<16;j++){
		

		if (drawBomb[j][i] == 1){IERG3810_TFTLCD_FillRectangle( 0x7BEF,x-i*3,3,y-j*3,3);}
		if (drawBomb[j][i] == 2){IERG3810_TFTLCD_FillRectangle( 0x0000,x-i*3,3,y-j*3,3);}
		if (drawBomb[j][i] == 3){IERG3810_TFTLCD_FillRectangle( 0xF800,x-i*3,3,y-j*3,3);}
		if (drawBomb[j][i] == 4){IERG3810_TFTLCD_FillRectangle( 0xFFE0,x-i*3,3,y-j*3,3);}
		if (drawBomb[j][i] == 5){IERG3810_TFTLCD_FillRectangle( 0x79E0,x-i*3,3,y-j*3,3);}
		if (drawBomb[j][i] == 0){IERG3810_TFTLCD_FillRectangle( 0x7E0,x-i*3,3,y-j*3,3);}
	}
}
}

void JusttrytryDrawMole(){
	IERG3810_TFTLCD_FillRectangle(0x7E0,0x0000,0x00F0,0,0x0140);
	
		drawMole(1);
		drawHurtMole(2);
		drawMole(3);
	
		drawHurtMole(4);
		drawBomb(5);
		drawNoMole(6);
	
		drawMole(7);
		drawMole(8);
		drawHurtMole(9);

		Delay(1000000);
	
}

void EXTI0_IRQHandler(void){
	u8 j;
	for(j=0;j<10;j++){
		GPIOE->BRR = 1 << 5 ; 							//reset (lit)  DS1_ON
		Delay(1000000);
		GPIOE->BSRR = 1 << 5; 							//set (unlit)  DS1_OFF
		Delay(1000000);
	}
	EXTI->PR = 1<<0;							//Clear this exception pending bit
	
}

void EXTI2_IRQHandler(void){
	u8 i;
	for(i=0;i<10;i++){
		GPIOB->BRR = 1 << 5 ; 							//reset (lit)  DS0_ON
		Delay(1000000);
		GPIOB->BSRR = 1 << 5; 							//set (unlit)  DS0_OFF
		Delay(1000000);
	}
	EXTI->PR = 1<<2;							//Clear this exception pending bit
	
}

void EXTI15_10_IRQHandler(void){
	//students write program here
	if(ps2count>0 && ps2count <9){
		dataread |= ((GPIOC->IDR & (1<<10)) >>10)<<(ps2count-1); //IDR read C port bit 10 data then shift to right most and shift to the currnet bit
	}
	ps2count++;
	ps2key |= dataread;
	dataread=0;
	Delay(10);
	EXTI->PR = 1<<11;
}
u32 sheep=0;

void experiment_3_4(){
			u8 str1[10] = {0x0031,0x0031,0x0035,0x0035,0x0031,0x0033,0x0031,0x0031,0x0035,0x0035};
			u8 str2[10] = {0x0031,0x0031,0x0035,0x0035,0x0031,0x0032,0x0035,0x0031,0x0032,0x0036};
			IERG3810_TFTLCD_FillRectangle(0x780F,0x0000,0x00F0,0,0x0140);
			IERG3810_TFTLCD_ShowCharInLine(0x0050,0x0050,str1,0x000A,0x0000, 0x780F);
			IERG3810_TFTLCD_ShowCharInLine(0x0050,0x0060,str2,0x000A,0x0000, 0x780F);
		  Delay(1000000);
	
}

void experiment_3_5(){
			u8 str1[10] = {0x0031,0x0031,0x0035,0x0035,0x0031,0x0033,0x0031,0x0031,0x0035,0x0035};
			u8 str2[10] = {0x0031,0x0031,0x0035,0x0035,0x0031,0x0032,0x0035,0x0031,0x0032,0x0036};
			u8 cstr1[3] = {0x00,0x01,0x02};
			u8 cstr2[3] = {0x03,0x04,0x05};
			
			IERG3810_TFTLCD_FillRectangle(0x780F,0x0000,0x00F0,0,0x0140);
			IERG3810_TFTLCD_ShowCharInLine(0x0070,0x0050,str1,0x000A,0xFFFF, 0x780F);
			IERG3810_TFTLCD_ShowCharInLine(0x0070,0x0060,str2,0x000A,0xFFFF, 0x780F);
			
			IERG3810_TFTLCD_ShowChinCharInLine(0x30,0x0050,cstr1,0x0003,0xFFFF,0x780F);
			IERG3810_TFTLCD_ShowChinCharInLine(0x30,0x0060,cstr2,0x0003,0xFFFF,0x780F);
		Delay(100000);
}
void firstPage(){
	
	char GameName[12] = "whack a mole"; // Game Name
	
	char SID1[10]="1155131155"; // SID
  char SID2[10]="1155125126";
	char GPnum[10]   ="GROUP: A16"; // Group Info
	
	u8 CName1[3] = {0x00,0x01,0x02};
	u8 CName2[3] = {0x03,0x04,0x05};
	
	char Info0[13] = "Instructions:";
	char Info1[25] = "Press key 1-9 to hit mole";
	char Info2[22] = "If hit Bomb, Game Over";	
	char Info3[20] = "Press Enter to start"; // Game meau
	char Info4[17] = "Press* to Restart";
	
	int i =0;
	IERG3810_TFTLCD_FillRectangle( 0x7E0,0,240,0,320);

	
	for(i=0;i <12;i++) // Draw Game Name
				{
					IERG3810_TFTLCD_ShowChar(50+i*8, 280, GameName[i],Black,0x7E0);
					drawMole(10);
				}
				
	for(i=0;i <10;i++) // Draw SID
				{
					IERG3810_TFTLCD_ShowChar(50+i*8, 220, SID1[i],Black,0x7E0);
					IERG3810_TFTLCD_ShowChar(50+i*8, 200, SID2[i],Black,0x7E0);
				}
	for(i=0;i <3;i++) // Draw Name
				{
					IERG3810_TFTLCD_ShowChinChar(150+i*16, 220, CName1[i],Black,0x7E0);
					IERG3810_TFTLCD_ShowChinChar(150+i*16, 200, CName2[i],Black,0x7E0);
				}
	for(i=0;i <10;i++) // Draw Group
				{
					IERG3810_TFTLCD_ShowChinChar(150+i*16, 180, GPnum[i],Black,0x7E0);
					
				}
	for(i=0;i <12;i++) // Draw Instrustion
				{
					IERG3810_TFTLCD_ShowChar(80+i*8, 160, Info0[i],Black,0x7E0);
				}
	for(i=0;i <22;i++) // Draw Instrustion 2
				{
					IERG3810_TFTLCD_ShowChar(20+i*8, 120, Info2[i],Black,0x7E0);
				}
	for(i=0;i <25;i++) // Draw Instrustion 1
				{
					IERG3810_TFTLCD_ShowChar(20+i*8, 140, Info1[i],Black,0x7E0);
					
				}
	for(i=0;i <20;i++) // Draw Instrustion 3
				{
					IERG3810_TFTLCD_ShowChar(20+i*8, 100, Info3[i],Black,0x7E0);
				}
	for(i=0;i <17;i++) // Draw Instrustion 4
				{
					IERG3810_TFTLCD_ShowChar(20+i*8, 80, Info4[i],Black,0x7E0);
				}
}

void GameStart(){
	
JusttrytryDrawMole();
}

int main(void){
	//int i = 0;

	IERG3810_clock_tree_init(); // Initailization of several hardware components
  IERG3810_LED_Init();
  IERG3810_NVIC_SetPriorityGroup(6);
	IERG3810_SYSTICK_Init10ms();
	IERG3810_TFTLCD_Init();
	IERG3810_key2_ExtiInit();
	IERG3810_keyUP_ExtiInit();
	IERG3810_PS2key_ExtiInit();
	
	GPIOE->BSRR = 1 << 5; 							//set (unlit)  DS1_OFF
	GPIOB->BSRR = 1 << 5; 							//set (unlit)  DS0_OFF
	firstPage();
	while(1){
		
		sheep++;														//count sheep,same as do nothing
		if(doOnce==0){
			firstPage();
			doOnce=1;
		}
		//if PS2 keyboard received data correctly
		if(ps2count >= 11){
//from lab4
			if(released==FALSE){
				if(ps2key==0xF0){
					released=TRUE;
				}
				if(ps2key==0x7C){								//Pressing *
					GPIOB->BRR = 1 << 5 ;					//DS0_ON
					released=FALSE;
					firstPage();
				}
				if(ps2key==0x5A){								//Pressing enter
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
					GameStart();
				}
				if(ps2key==0x69){								//Pressing key1
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
					
				}
				if(ps2key==0x72){								//Pressing key2
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x7A){								//Pressing key3
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x6B){								//Pressing key4
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x73){								//Pressing key5
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x74){								//Pressing key6
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x6C){								//Pressing key7
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x75){								//Pressing key8
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
				if(ps2key==0x7D){								//Pressing key9
					GPIOE->BRR = 1 << 5 ;					//DS1_ON	
					released=FALSE;
				}
			}else{														//RELEASED = True
				if(ps2key==0x7C){								//release after press *
					GPIOB->BSRR = 1 << 5 ;				//DS0_OFF
					released=FALSE;
				}
				
				if(ps2key==0x5A){								//release after press enter
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x69){								//release after press key1
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
					
				}
				if(ps2key==0x72){								//release after press key2
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
					
				}
				if(ps2key==0x7A){								//release after press key3
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
					
				}
				if(ps2key==0x6B){								//release after press key4
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x73){								//release after press key5
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x74){								//release after press key6
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x6C){								//release after press key7
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x75){								//release after press key8
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
				if(ps2key==0x7D){								//release after press key9
					GPIOE->BSRR = 1 << 5 ;				//DS1_OFF
					released=FALSE;
				}
			}
			ps2key=0;
			ps2count=0;
			EXTI->PR=1<<11;										//CLEAR_BIT this exception pendingbit
			//EXTI->IMR |= (1<<11); 						//optional,resuem interrupt
		}//end of "if PS2 keyboard data then timout"
		
	
		//JusttrytryDrawMole();
		//experiment_3_4();
	}

}
