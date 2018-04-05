// Spider Attack.c
// Runs on LM4F120/TM4C123
// Fernando Renteria
// September 1, 2017
// ******* Required Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Red SparkFun Nokia 5110 (LCD-10168)
// -----------------------------------
// Signal        (Nokia 5110) LaunchPad pin
// 3.3V          (VCC, pin 1) power
// Ground        (GND, pin 2) ground
// SSI0Fss       (SCE, pin 3) connected to PA3
// Reset         (RST, pin 4) connected to PA7
// Data/Command  (D/C, pin 5) connected to PA6
// SSI0Tx        (DN,  pin 6) connected to PA5
// SSI0Clk       (SCLK, pin 7) connected to PA2
// back light    (LED, pin 8) not connected, consists of 4 white LEDs which draw ~80mA total



#include "tm4c123gh6pm.h"
#include "BSP.h"
#include "PLL.h"
#include "images.h" 

//Functions Prototypes
void DrawCover(void);
void DrawSpiders(void);
void DrawMissiles(void);
void DrawShip(void);
void EnableInterrupts(void);
void SysTick_Init(void);


	
void Delay100ms(unsigned long time){
  time *= 800000-1;  // .1 sec
  while(time){
		time--;
  }
}

void ledInit(void){volatile unsigned long Counts = 0;
  SYSCTL_RCGC2_R |= 0x00000020; // activate port F
  Counts = 0;
  GPIO_PORTF_DIR_R |= 0x08;   // make PF2 output (PF2 built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x08;// disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x08;   // enable digital I/O on PF2
                              // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF0FFF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
}


//Global Variables
unsigned long semaphore = 0;
unsigned long frameCount = 0;
unsigned long toggle = 0;
unsigned long prevBtn1 = 1;
unsigned long prevBtn2 = 1;
unsigned long touchR = 0;
unsigned long touchL = 1;
//Sprites
struct Sprite {
	uint16_t x;			// x coordinate
	uint16_t y;			// y coordinate
	uint16_t w;			// w width in px
	uint16_t h;			// h height in px
	const uint16_t *image; // ptr->image
	long life;			// 0=dead, 1=alive
	long cleared;   // sprite cleared from screen 
};	
typedef struct Sprite SpriteTyp;
SpriteTyp spider[4]; 
void CreateSpiders(void){ int i;
	for(i=0;i<4;i++){
		spider[i].x = 55;
		spider[i].y = 65;
		spider[i].w = 16;
		spider[i].h = 10;
		spider[i].image = spiderA;
		spider[i].life = 1;
		spider[i].cleared = 0;
	}
}

SpriteTyp ship;
void CreateShip(void){
		ship.x = 55;
		ship.y = 119;
		ship.w = 16;
		ship.h = 10;
		ship.image = playerShip;
		ship.life = 1;
}

SpriteTyp missile;
void CreateMissile(SpriteTyp *ship){
		missile.x = ship->x+ship->w/2-1;
		missile.y = ship->y-ship->h;
		missile.w = 2;
		missile.h = 9;
		missile.image = bullet;
		missile.life = 1;
		missile.cleared = 0;
}

void MoveSpriteRight(SpriteTyp *sprt);

void MoveSpriteLeft(SpriteTyp *sprt);

void SystemInit(void){
}
int main(void){
	PLL_Init();
	//ledInit();
	BSP_LCD_Init();
	BSP_Joystick_Init();
	BSP_Button2_Init();
	DrawCover();
	BSP_LCD_FillScreen(0);
	BSP_LCD_DrawFastHLine(0, 120, 128, LCD_RED);
	CreateSpiders();
	CreateShip();
	SysTick_Init();
	EnableInterrupts();
	while(1){
		if (semaphore){
			DrawSpiders();
			DrawShip();
			DrawMissiles();
			semaphore=0;
		}
	}
	
}

void DrawCover(void){unsigned short int i=0;
	BSP_LCD_DrawString(8, 2, "Spider", LCD_RED);
	BSP_LCD_DrawString(8, 4, "Attack", LCD_RED);
	while(i<7){
		if (i%2==0)
			BSP_LCD_DrawBitmap(55, 70, spiderA, 16, 10);
		else
			BSP_LCD_DrawBitmap(55, 70, spiderB, 16, 10);
		i++;
		Delay100ms(5);
	}
}

void DrawSpiders(void){
	if (spider[0].life)
		BSP_LCD_DrawBitmap(spider[0].x, spider[0].y, spider[0].image, spider[0].w, spider[0].h);
	else if (spider[0].cleared == 0){
		BSP_LCD_FillRect(spider[0].x, spider[0].y-spider[0].h, spider[0].w, spider[0].h, 0);
		spider[0].cleared =1;
		spider[0].x=128;
	}
}

void DrawShip(void){
	BSP_LCD_DrawBitmap(ship.x, ship.y, ship.image, ship.w, ship.h);
}


void DrawMissiles(void){
	if (missile.life)
		BSP_LCD_DrawBitmap(missile.x, missile.y, missile.image, missile.w, missile.h);
	else if (missile.cleared == 0){
		BSP_LCD_FillRect(missile.x, missile.y-missile.h+1, missile.w, missile.h, 0);
		missile.cleared =1;
	}
}


// **************SysTick_Init*********************
// Initialize Systick periodic interrupts
// Input: none
// Output: none
void SysTick_Init(void){
	NVIC_ST_CTRL_R = 0;				// disable Systick for Setup
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_RELOAD_R = 1333333; //interrupt 60Hz (12.5ns units)
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1     
	NVIC_ST_CTRL_R = 0x07;		// clock internal source, interrupt enable, systick enable
}

// Interrupt service routine
// Executed at 30Hz
void SysTick_Handler(void){//uint16_t *ship_x, *ship_y;uint8_t *pressed;
	//BSP_Joystick_Input(ship_x, ship_y, pressed);
	/*
	if (BSP_Button2_Input()==0 && prevBtn2 == 1){
		prevBtn2 = 0;
		sprite bullet;
		bullet
	}
	if (BSP_Button2_Input())
		prevBtn = 1;
	*/
	
	if (frameCount == 60){
		frameCount = 0;
	} 
	
	if (BSP_Button2_Input()==0 && prevBtn2){
			CreateMissile(&ship);
		}
	prevBtn2 = BSP_Button2_Input();
	if (frameCount % 2 == 0){
		if (missile.life)
			missile.y--;
	}
	
	if (frameCount % 15 == 0){
		if (spider[0].life){
			if (spider[0].x < 111 && touchR == 0)
				MoveSpriteRight(&spider[0]);
			else {touchR = 1;touchL = 0;}
			if(spider[0].x > 1 &&  touchL ==0)
				MoveSpriteLeft(&spider[0]);
			else{ touchL = 1;touchR = 0;}	
			if (frameCount % 2 ==0)
			spider[0].image = spiderA;
			else
			spider[0].image = spiderB;
		}
	}
	
	if (missile.x > spider[0].x && missile.x < spider[0].x+spider[0].w){
		if(missile.y-missile.h > spider[0].y-spider[0].h && missile.y-missile.h < spider[0].y){
			spider[0].life = 0;
			missile.life = 0;
		}
	}
	
	frameCount++;
	semaphore = 1;
}

void MoveSpriteRight(SpriteTyp *sprt){
//if (sprt.x < 127-sprt.w)
			sprt->x += 2;
}

void MoveSpriteLeft(SpriteTyp *sprt){
//if (sprt.x < 127-sprt.w)
			sprt->x -= 2;
}

