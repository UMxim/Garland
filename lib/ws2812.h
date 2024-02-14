#ifndef __WS2812_H
#define __WS2812_H

#include <stdint.h>

#define LED_NUM 300
#define DMA_BUFF_SIZE (9*LED_NUM)

void ws2813_FillConvertBuffer(void);
void ws2813_AddRGB(uint32_t rgb, int num);
void ws2813_ChangeCurrentBuff(void);
uint8_t * ws2813_GetDMAbuff(void);

#endif // __WS2812_H
