#ifndef __RANDOM_H
#define __RANDOM_H

#include <stdint.h>

#define RANDOM_BUFF_SIZE_WORDS		8	// Количество генерируемых случайных слов(u32)

uint8_t Random_AddBit(uint32_t ext_bit);

uint32_t Random_GetRnd(void);

uint8_t Random_GetV(uint16_t rnd_linear);

#endif // __RANDOM_H
