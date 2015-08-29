#ifndef CRC_H
#define CRC_H

#include "../_config_cpu_.h"

//defines para compatibilidade com sistemas antigos
#define getCRC16CCITT	getCRC16_XMODEM

u16 crc16_NBR14522(void* data, int length);
u16 crc16_MODBUS(void* data, int length);
u16 crc16_XMODEM(void* data, int length);
u16 crc16_DNP3(void* data, int length);
u8 crc7(void* data, int length);
u8 crc8_HEX(void* data, int length);

#endif
