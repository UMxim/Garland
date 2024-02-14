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



/*
Массив конвертации при компиляции. Проверено, эквивалентно.
// Set Bit Pos   Текущий бит преобразуем в 3 бита 0X0
#define SBP(byte, bit) ( ((byte>>bit)&1)<<(3*bit+1) )
// Конверт байта в u32 где каждый бит X приводится в вид 1X0.  пример 0x5 -> 0b 110 100 110
#define CONVERT(byte) ( 0x924924 | SBP(byte, 0) | SBP(byte, 1) | SBP(byte, 2) | SBP(byte, 3) | SBP(byte, 4) | SBP(byte, 5) | SBP(byte, 6) | SBP(byte, 7) )
// Создание последовательности 16 чисел с заданным старшим ниблом
#define ADD_NIBBLE(hi_nibble_val) 	CONVERT(hi_nibble_val+0x0), CONVERT(hi_nibble_val+0x1), CONVERT(hi_nibble_val+0x2), CONVERT(hi_nibble_val+0x3), \
								 	CONVERT(hi_nibble_val+0x4), CONVERT(hi_nibble_val+0x5), CONVERT(hi_nibble_val+0x6), CONVERT(hi_nibble_val+0x7), \
									CONVERT(hi_nibble_val+0x8), CONVERT(hi_nibble_val+0x9), CONVERT(hi_nibble_val+0xA), CONVERT(hi_nibble_val+0xB), \
									CONVERT(hi_nibble_val+0xC), CONVERT(hi_nibble_val+0xD), CONVERT(hi_nibble_val+0xE), CONVERT(hi_nibble_val+0xF)

const uint32_t convertBuffer[0x100] = {	ADD_NIBBLE(0x00), ADD_NIBBLE(0x10), ADD_NIBBLE(0x20), ADD_NIBBLE(0x30),
										ADD_NIBBLE(0x40), ADD_NIBBLE(0x50), ADD_NIBBLE(0x60), ADD_NIBBLE(0x70),
										ADD_NIBBLE(0x80), ADD_NIBBLE(0x90), ADD_NIBBLE(0xA0), ADD_NIBBLE(0xB0),
										ADD_NIBBLE(0xC0), ADD_NIBBLE(0xD0), ADD_NIBBLE(0xE0), ADD_NIBBLE(0xF0) };

*/

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
