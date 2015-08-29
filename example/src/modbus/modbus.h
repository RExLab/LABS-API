#ifndef MODBUS_H
#define MODBUS_H

#include "../_config_cpu_.h"
#include "../crc/crc.h"

#define modbusCMD_READ_REGISTERS	0x03
#define modbusCMD_WRITE_REGISTER	0x06
#define modbusCMD_WRITE_REGISTERS	0x10

#define modbusNO_ERROR				00
#define modbusILLEGAL_FUNCTION		01 // O servidor recebeu uma função que não foi implementada ou não foi habilitada.
#define modbusILLEGAL_DATA_ADDRESS	02 // O servidor precisou acessar um endereço inexistente.
#define modbusILLEGAL_DATA_VALUE	03 // O valor contido no campo de dado não é permitido pelo servidor. Isto indica uma falta de informações na estrutura do campo de dados.
#define modbusSLAVE_DEVICE_FAILURE	04 // Um irrecuperável erro ocorreu enquanto o servidor estava tentando executar a ação solicitada.


#endif
