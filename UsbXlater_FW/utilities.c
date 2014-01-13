#include "utilities.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stm32fx/peripherals.h>
#include <cmsis/core_cmFunc.h>

uint8_t dbgmode = 0; // stores runtime debug level
uint8_t global_temp_buff[GLOBAL_TEMP_BUFF_SIZE]; // this buffer is shared between many modules, do not use within interrupts!

#define BOOTLOADER_ADDR 0x1FFF0000
#define BOOTLOADER_STACK 0x20001000
typedef void (*pFunction)(void);
static volatile pFunction bl_jmp_func;
/*
 * use this to activate the system boot mode bootloader of the STM
 */
void jump_to_bootloader(void)
{
	RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	__set_PRIMASK(1); // disables interrupt
	__set_MSP(BOOTLOADER_STACK);
	bl_jmp_func = (void(*)(void))(*((uint32_t*)(BOOTLOADER_ADDR + 4)));
	bl_jmp_func();
	while(1);
}

uint16_t fletcher16(uint8_t const * data, size_t bytes)
{
	uint16_t sum1 = 0xff, sum2 = 0xff;
	while (bytes)
	{
		size_t tlen = bytes > 20 ? 20 : bytes;
		bytes -= tlen;
		do {
			sum2 += sum1 += *data++;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	return sum2 << 8 | sum1;
}

uint32_t version_crc()
{
	int i = sprintf(global_temp_buff, "Compiled on " __DATE__ ", " __TIME__", ");
	#ifdef __GNUC__
	i += sprintf(&global_temp_buff[i], "GNU C %d", __GNUC__);
	#ifdef __GNUC_MINOR__
	i += sprintf(&global_temp_buff[i], ".%d", __GNUC_MINOR__);
	#ifdef __GNUC_PATCHLEVEL__
	i += sprintf(&global_temp_buff[i], ".%d", __GNUC_PATCHLEVEL__);
	#endif
	#endif
	#else
	i += sprintf(&global_temp_buff[i], "unknown compiler\r\n");
	#endif
	global_temp_buff[i++] = 0;
	global_temp_buff[i++] = 0;
	global_temp_buff[i++] = 0;
	global_temp_buff[i++] = 0;
	CRC_ResetDR();
	return CRC_CalcBlockCRC((uint32_t*)global_temp_buff, i / 4);
}

uint8_t* print_bdaddr(uint8_t* data)
{
	sprintf(&global_temp_buff[GLOBAL_TEMP_BUFF_SIZE - (3 * 6) - 2], "%02X:%02X:%02X:%02X:%02X:%02X", data[5], data[4], data[3], data[2], data[1], data[0]);
	return &global_temp_buff[GLOBAL_TEMP_BUFF_SIZE - (3 * 6) - 2];
}

uint8_t* print_linkkey(uint8_t* data)
{
	for (int i = 0; i < 16; i++) {
		sprintf(&global_temp_buff[(GLOBAL_TEMP_BUFF_SIZE / 2) + (i * 2)], "%02X", data[i]);
	}
	return &global_temp_buff[GLOBAL_TEMP_BUFF_SIZE / 2];
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  dbg_printf(DBGMODE_ERR, "\r\nassert failed %s %d\r\n", file, line);
  dbgwdg_stop();
  while (1) { }
}
#endif
