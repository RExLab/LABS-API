#ifndef _UART_H
#define _UART_H

#include "../uc_libdefs.h"
#include <termios.h>

int uart_Init(const char* port, uint baudrate);
void uart_Close(void);
int uart_SendString(const char* buf);
int uart_SendBuffer(u8* buf, u16 size);
int uart_PutChar(n16 ch);
int uart_GetChar(u8* ch);
void uart_ClearBufferRx(void);
int uart_BufferQtdRx(void);
void uart_ClearBufferTx(void);
//int uart_BufferQtdTx(void);
#endif
