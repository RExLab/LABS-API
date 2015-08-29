#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "modbus.h"


void modbus_MasterAppendTime(tTime(*now_func)(void), int timeout);
int modbus_MasterReadStatus(void);
int modbus_MasterReadException(void);
int modbus_MasterReadRegisters(int addrSlave, int addrInit, int len, u16* regs);
int modbus_MasterWriteRegister(int addrSlave, int addr, u16 value);
int modbus_MasterWriteRegisters(int addrSlave, int addrInit, int len, u16* regs);
void modbus_MasterProcess(void);
void modbus_MasterInit(
	int(*puts_func)(u8* buffer, u16 count),
	int(*getc_func)(u8* ch),
	void(*flushRX_func)(void)
);

#endif
