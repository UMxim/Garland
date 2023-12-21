#include "ws2812.h"

uint8_t dmaBuff[2][LED_NUM*3*3];
uint8_t dmaCurrBuff = 0;

uint32_t convertBuffer[0x100];

// Заполняем буфер конвертации
void ws2813_FillConvertBuffer(void)
{	
	uint8_t bit;
	uint8_t pos;
	uint32_t mask;	
	for (int i=0; i<0x100; i++) {
		convertBuffer[i] = 0x924924;
		for (int b=0; b<8; b++) {
			bit = (i>>b)&1;
			pos = b*3+1;
			mask = bit<<pos;
			convertBuffer[i] |= mask;
		}			
	}		
}

// Добавляем цвет диода в буфер DMA 
void ws2813_AddRGB(uint32_t rgb, int num)
{		
	uint32_t val;
	uint8_t *dBuff = &dmaBuff[dmaCurrBuff][9*num];	
	val = convertBuffer[(uint8_t)(rgb>>8)];
	*dBuff++ = val>>16;
	*dBuff++ = val>>8;
	*dBuff++ = val;
	val = convertBuffer[(uint8_t)(rgb>>16)];
	*dBuff++ = val>>16;
	*dBuff++ = val>>8;
	*dBuff++ = val;
	val = convertBuffer[(uint8_t)(rgb)];
	*dBuff++ = val>>16;
	*dBuff++ = val>>8;
	*dBuff++ = val;	
}

void ws2813_ChangeCurrentBuff()
 {	
	dmaCurrBuff++;
	dmaCurrBuff&= 1;
}

uint8_t * ws2813_GetDMAbuff()
{
	return &dmaBuff[dmaCurrBuff][0];
}
