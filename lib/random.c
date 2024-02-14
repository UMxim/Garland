#include "random.h"

// формируем слово отдельно, по готовности - пишем в rndBuff[currentWord]
// Отдаем последнее готовое слово.

static uint32_t rndBuff[RANDOM_BUFF_SIZE_WORDS];
static uint8_t currentWord; // текущее ФОРМИРУЕМОЕ слово. Еще не готово

// Возврат:  	0 - буфер полон
//				1 - Первый бит пары
//				2 - 00 или 11. Отброшены
//				3 - Бит принят
//				4 - Слово сформировано
uint8_t Random_AddBit(uint32_t ext_bit)
{
	// Бит вычисляем так: Считываем 2 бита. Если 00 или 11 - отбрасываем. Если 01 то 0, если 10 то 1
	static uint32_t word = 0;
	static uint8_t bit = 0; // текущий вычисляемый бит
	static uint8_t pairN = 0; // Номер принимаемого бита
	static uint8_t lastBit ; // Сохраняем бит с номером 0

	if (currentWord == RANDOM_BUFF_SIZE_WORDS) 
		return 0;
	ext_bit &= 1;
	if ((pairN++ & 1) == 0)
	{
		lastBit = ext_bit;
		return 1;
	}
	if ((lastBit ^ ext_bit) == 0)	// 00 или 11 - отбрасываем
	{
		return 2;
	}
	if (lastBit)
	{
		word |= 1<<bit;
	}
	bit++;
	if (bit == 32)
	{
		rndBuff[currentWord++] = word;
		word = 0;
		bit = 0;
		return 4;
	}

	return 3;
}

// При ошибке возвращаем 0 - не хорошо, но и 0 - плохое случайное число, да и редкое
uint32_t Random_GetRnd()
{
	if (currentWord == 0) return 0; // нет готовых слов
	currentWord--;
	return rndBuff[currentWord - 1];
}

// вычисляем корень из x
static uint8_t GetSQRT(uint16_t x)
{
    uint8_t ans = 0;
    uint8_t tmp;
    for (int i=7; i>=0; i--)    
    {
        tmp = ans | (1<<i);
        if ((tmp * tmp) <= x)
            ans = tmp;
    }
    return ans;
}


// Возвращаем случайное число с распределением V (чаще всего 0 и 0xFF. 0x7F почти никогда)
uint8_t Random_GetV(uint16_t rnd_linear)
{
	uint8_t calc;
    calc = GetSQRT(rnd_linear) >> 1; // 0 - 0x7F
    if (rnd_linear & 1)
    {
        calc = 0x7F + calc;
    }
    else
    {
        calc = 0x7F - calc;
    }
    
    return calc;
}
