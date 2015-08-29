/* Criado em 17/03/2015
 * Atualizado em 29/04/2015
 *
 * Biblioteca MODBUS escravo barramento serial rs232/rs485 no modo RTU (Binário)
 *
 * Este protocolo atende somente as funções:
 * 	 Leituta de muitos registradores, código 3,
 * 	 Escrita em um simples registrador, código 6
 * 	 Escrita em muitos registradores, código 16
 *
 * Esta lib somente captura as mensagens quando o barramento serial fique em silencio no minimo 5ms.
 * Então, o timeout do mestre na espera de uma resposta deste dispostivo deve ser superior a 5ms.
 * Geralmente usamos 3 segundos.
 * Entretanto, o mestre pode se comunicar com outros dispositivos que utilizam a técnica t35,
 * onde o tempo de silencio do barramento é muito menor. Neste caso, esta lib vai ignorar as mensagens
 * por considerar que as mensagens são destinadas para outros dispositivos.
 *
 * Os endereços dos registradores devem ser passados pelos seus valores reais e não por valores enumerados,
 *  ou seja, valores de 0 a N.
 *
 * Técnica t35 = Tempo do barramento em silêncio que é 3.5 vezes o tamanho de um char no barramento.
 *
 * Para detalhes do protocolo consunte o documento D:\meus_conhecimentos\_devices_misc__\modbus\resumo_modbus.docx
 *
 * */


#include "modbus_slave.h"
#if (MODBUS_USE_DEBUG == pdON)
#include "stdio_uc.h"
#endif

typedef struct {
	int slaveID;						// Endereço desse dispostivo no barramento modbus
	int (*puts)(u8 *buffer, u16 count);	// Ponteiro da função para envio de bytes
	int (*getc)(u8 *ch);				// Ponteiro da função para recebimento de bytes
	int (*byteAvailable)(void);			// Ponteiro da função para checagem se há bytes já recebidos
	void(*flushRX)(void);				// Ponteiro da função para limpar o buffer de recepção
	tTime (*now)(void);					// Função contadora de tempo decorrido
	tTime timeout; 						// Determina o tempo de silencio do barramento

	// vars aux para comunicação atual
	u8 query[256];						// Buffer aux de transmissão e recepção de dados
	int cmd;							// Comando (função) solicitado
	int lastLenBuffer;

	// Funções externas para antender os comandos recebidos
	int (*read_regs)(uint addrInit,  u8* query, uint count);
	int (*write_reg)(uint addr, u16 value);
	int (*write_regs)(uint addrInit, u8* query, uint count);
} modbusSlave_t;

static modbusSlave_t modbus;

static int modbus_GetPacket(u8* query);
static void modbus_SendPacketException(int exception);
static void modbus_SendPacketRegs(u8* query, int len);

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_SlaveInit
// Descrição: 	Inicializa o protocolo modbus no modo escravo
// Parametros:	slaveID: Endereço do escravo
//				puts_func:	Ponteiro da função de transmissão serial
//				getc_func:	Ponteiro da função de recepção de dados
//				byte_available_func: Ponteiro da função para verificar se há dados no buffer de recepção serial
//				flushRX_func: Ponteiro da função que limpar os buffers seriais
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------

// ATENÇÃO: O tamanho do buffers de recepção e transmissão deve ter no mínimo o frame do modbus, recomendo deixar no mínimo  256 bytes
// se usar rs485 tem que ter cuidado para fazer a inversão do barramento. exemplo
//		int uart_PutString(u8* buffer, u16 count) {
//			rs485_ENTX_ON = rs485_ENTX;
//			int ret = uart1_WriteTx(buffer, count);
//			while (!uart1_EmptyTx()); // Vamos esperar que o serial envie a mensagem
//			rs485_ENTX_OFF = rs485_ENTX;
//			return ret;
//		}

void modbus_SlaveInit(
	int slaveID,
	int(*puts_func)(u8* buffer, u16 count),
	int(*getc_func)(u8* ch),
	int(*byte_available_func)(void),
	void(*flushRX_func)(void)
) {

	modbus.slaveID = slaveID;
	modbus.puts = puts_func;
	modbus.getc = getc_func;
	modbus.byteAvailable = byte_available_func;
	modbus.flushRX = flushRX_func;
	modbus.lastLenBuffer = 0;
	modbus.timeout = 0;

	modbus.now =(tTime (*)()) NULL;
	modbus.read_regs = (int (*)(uint, u8*, uint) )NULL;
	modbus.write_reg = (int (*)(uint, u16))NULL;
	modbus.write_regs = (int (*)(uint, u8*, uint) )NULL;
}

void modbus_SlaveSetID(int slaveID) {
	modbus.slaveID = slaveID;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_AppendFunctions
// Descrição: 	Aponta para as funções de controle de acesso aos registradores

// Parametros:	now_func: Ponteiro para função de tempo e é obrigatório
//				readregs_func: Lê N valores dos registradores, ver mais detalhes abaixo
//				writereg_func:	Escreve um valor em um registrador de 16 bits
//				writeregs_func: Escreve N valores nos registradores, lembrando que o endereçamento de registradores de 16bits
// Retorna:		Código MODBUS, ver abaixo
// -------------------------------------------------------------------------------------------------------------------
// as funções devem ter a seguinte forma:
//		now_fun: Função deve retornar com a passagem do tempo em ms
//			tTime (*now)(void),
//		readregs_func: Lê N valores dos registradores, lembrando que o endereçamento de registradores de 16bits
//			int nomeFunc(uint addrInit, u8* query, uint count);
//				addrInit: Endereço do primeiro registrador de 16 bits a ser acessado
//				query: Buffer para ser armazenado os valores dos registradores para serem enviados ao mestre
//				count: Quantidade de registradores devem ser lidos
//		writereg_func: Escreve um valor em um registrador de 16 bits
//			int nomeFunc(uint addr, u16 value);
//				addr: Endereço do registrador de 16 bits a ser gravado
//				value: Valor a ser gravado no registrador
//		writeregs_func: Escreve N valores nos registradores, lembrando que o endereçamento de registradores de 16bits
//			int nomeFunc(uint addrInit, u8* query, uint count);
//				addrInit: Endereço do primeiro registrador de 16 bits a ser gravado
//				query: Buffer com os valores dos registradores a serem salvos
//				count: Quantidade de registradores devem ser gravados

// 	A funções devem retornar com os seguintes valores:
//		modbusNO_ERROR caso processador correto
//		modbusILLEGAL_DATA_ADDRESS:	Se vai acessar o endereço inixistente
//		modbusILLEGAL_DATA_VALUE:	A quantidade de registradores a ser lido é inválido
//		modbusSLAVE_DEVICE_FAILURE:	Um irrecuperável erro ocorreu enquanto o servidor estava tentando executar a ação solicitada.

// Exemplo para readregs_func:
//		int read_Registers(uint addrInit, u8* query, uint count) {
//			// checa se o endereço está dentro dos limite
//			if ((count == 0) || (addrInit + (count-1) >= 4)) return modbusILLEGAL_DATA_ADDRESS;
//
//			// preenche o buffer modbus com os valores dos registradores
//			uint i; for (i=0;i<count;i++) {
//				query[2*i] = (u8)(regs[addrInit+i]>>8);
//				query[2*i+1] = (u8)regs[addrInit+i]&0xFF;
//			}
//
//			return modbusNO_ERROR; // retorna sinalizando que a operação ocorreu bem
//		}

// Exemplo para writereg_func:
//		static int write_Register(uint addr, u16 value) {
//			// checa se o endereço está dentro dos limite
//			if (addr >= 4) return modbusILLEGAL_DATA_ADDRESS;
//			regs[addr] = value;
//			return modbusNO_ERROR; // retorna sinalizando que a operação ocorreu bem
//		}

// Exemplo para writeregs_func:
//		static int write_Registers(uint addrInit, u8* query, uint count) {
//			// checa se o endereço está dentro dos limite
//			if ((count == 0) || (addrInit + (count-1) >= 4)) return modbusILLEGAL_DATA_ADDRESS;
//
//			// preenche o buffer modbus com os valores dos registradores
//			uint i; for (i=0;i<count;i++) {
//				regs[addrInit+i] = ((u8)query[2*i]<<8) | query[2*i+1];
//			}
//
//			return modbusNO_ERROR; // retorna sinalizando que a operação ocorreu bem
//		}

// Exemplo de append
//		modbus_SlaveAppendFunctions(now, read_Registers, write_Register, NULL);
//		modbus_SlaveAppendFunctions(now, read_Registers, NULL, NULL);
void modbus_SlaveAppendFunctions(
	tTime(*now_func)(void),
	int (*readregs_func)(uint addrInit, u8* query, uint count),
	int (*writereg_func)(uint addr, u16 value),
	int (*writeregs_func)(uint addrInit, u8* query, uint count)
) {
	modbus.now = now_func;
	modbus.read_regs = readregs_func;
	modbus.write_reg = writereg_func;
	modbus.write_regs = writeregs_func;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_GetPacket
// Descrição: 	Captura o pacote de dados que estão o buffer da serial e já calcula o CRC
// Parametros:	query: Ponteiro do buffer para salvar o pacote
// Retorna:		retorna com a quantidade de bytes no buffer query, ou o valor do erro
// -------------------------------------------------------------------------------------------------------------------
static int modbus_GetPacket(u8* query) {
	int len = 0;
	u8 ch;

	// capturo os dados do buffer da serial e passo para o buffer modbus
	// faço enquanto há dados no buffer de recepção
	#if (MODBUS_USE_DEBUG == pdON)
	plognp("MODBUS: GET PACKET: ");
	#endif

	while (modbus.getc(&ch) == pdPASS) {
		if (len > 255) return errMODBUS_BUFRX_OVERFLOW;
		query[len++] = ch;
		#if (MODBUS_USE_DEBUG == pdON)
		plognp("0x%x ", ch);
		#endif
	}

	// o menor pacote é de 4 bytes
	if (len < 4) {
		#if (MODBUS_USE_DEBUG == pdON)
		plognp(CMD_TERMINATOR);
		plognp("ERROR: len packet "CMD_TERMINATOR);
		#endif

		return errMODBUS_BUFRX_INCOMPLETE;
	}

	// processar CRC
	len-=2; // desconsidera o CRC
	uint crc = crc16_MODBUS(query, len);
	uint crc_rx = ( query[len+1] << 8) | (query[len]);

	#if (MODBUS_USE_DEBUG == pdON)
    plognp(CMD_TERMINATOR);
    plognp("MODBUS: len %d ", len);
    plognp("query: ");
    int db_x; for(db_x=0;db_x<len;db_x++) plognp("0x%x ", query[db_x]);
    plognp(CMD_TERMINATOR);
    plognp("MODBUS: crc 0x%x == 0x%x"CMD_TERMINATOR, crc_rx, crc);
    #endif

	if (crc != crc_rx) return errMODBUS_CRC;

	return len;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_SendPacketException
// Descrição: 	Envia uma mensagem de exceção para o mestre
// Parametros:	exception: Código da exceção
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------
static void modbus_SendPacketException(int exception) {
	modbus.query[0] = modbus.slaveID;
	modbus.query[1] = 0x80 + modbus.cmd;
	modbus.query[2] = exception;
	uint crc = crc16_MODBUS(modbus.query, 3);
	modbus.query[3] = (u8)(crc&0xFF);
	modbus.query[4] = (u8)(crc>>8);

	modbus.puts(modbus.query, 5);

	#if (MODBUS_USE_DEBUG == pdON)
    plognp("MODBUS: SEND EXCPETION: ");
    int db_x; for(db_x=0;db_x<5;db_x++) plognp("0x%x ", modbus.query[db_x]);
    plognp(CMD_TERMINATOR);
    plognp("MODBUS: crc 0x%x"CMD_TERMINATOR, crc);
    #endif
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_SendPacketException
// Descrição: 	Envia uma mensagem com os valores lidos dos registradores
// Parametros:	query: Buffer já contendo os valores dos registradores
//				len: Quantidade de registradores de 16 bits
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------
static void modbus_SendPacketRegs(u8* query, int len) {
	query[0] = modbus.slaveID;
	query[1] = modbus.cmd;
	query[2] = (u8)(2*len);
	uint crc = crc16_MODBUS(query, (2*len)+3);
	query[(2*len)+3] = (u8)(crc&0xFF);
	query[(2*len)+4] = (u8)(crc>>8);
	modbus.puts(query, (2*len)+5);

	#if (MODBUS_USE_DEBUG == pdON)
    plognp("MODBUS: SEND PCK [%d]: ", len);
    int db_x; for(db_x=0;db_x<(2*len)+5;db_x++) plognp("0x%x ", query[db_x]);
    plognp(CMD_TERMINATOR);
    plognp("MODBUS: crc 0x%x"CMD_TERMINATOR, crc);
    #endif
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_SlaveProcess
// Descrição: 	Polling dde controle do modo escravo
// -------------------------------------------------------------------------------------------------------------------
// exemplos de pacotes
// 		len	  	 0    1    2	3    4    5    6    7
// 			 	0x01 0x03 0x00 0x00 0x00 0x02 0xC4 0x0B 	lendo dois regs no device 1 addr 2
//				0x04 0x03 0x00 0x0F 0x00 0x02 0xF4 0x5D		lendo dois regs no device 4 addr 2
int modbus_SlaveProcess(void) {
   	int len = modbus.byteAvailable();

   	// se não há nada no buffer não fazemos nada
    if ( len == 0 ) {
    	modbus.lastLenBuffer = 0;
        return pdFAIL;
    }

    // Limpa o timeout caso o buffer de recepção vai aumentando
    // se estourar o buffer o driver serial se encarrega disso, e neste caso não processamos nada
    // mesmo com o buffer cheio com dados lixo os dados serão todos processados e o CRC não vai bater e tudo será descartado
    if (modbus.lastLenBuffer != len) {
    	modbus.lastLenBuffer = len;
        modbus.timeout = modbus.now() + 10; // valor 10 funciona bem entre 2400 a 115200 bps. Valor 5 funcionou bem com 57600 e 115200
        									// para boudrate menores pode ser que devemos aumetar esse valor.
        									// Recomendo fazer uma macro associado ao boudrate da serial
        return pdFAIL;
    }

    // somente vamos processar quando estourar o timeout de recepção
    if (modbus.now() < modbus.timeout)
    	return pdFAIL;

   	modbus.lastLenBuffer = 0;
    #if (MODBUS_USE_DEBUG == pdON)
    plognp("MODBUS: proc len %d"CMD_TERMINATOR, len);
    #endif

	len = modbus_GetPacket(modbus.query);

    // se houve erro de CRC ou overflow do buffer, ou o endereço na mensagem é para este dispositivo
    // o menor pacote é 4 bytes (1 byte ID, 1 byte função e 2 bytes CRC), porém no modbus.query não vai conter CRC
    if ( (len < 1) || (modbus.query[0] != modbus.slaveID) ) {
	    #if (MODBUS_USE_DEBUG == pdON)
    	plognp("MODBUS: pck error"CMD_TERMINATOR);
    	#endif
		modbus.flushRX();
    	return pdFAIL;
    };

	int ret;
	uint addrInit; // Não podemos colocar a captura do addr aqui porque pode haver comandos simples

	modbus.cmd = modbus.query[1];

	// checa se a função é de leitura dos registradores, processa caso há função anexada
    if ( (modbus.cmd == modbusCMD_READ_REGISTERS) && (modbus.read_regs != NULL)) {
		addrInit = ((uint) modbus.query[2] << 8) + (uint)modbus.query[3];
		len = ((int) modbus.query[4] << 8) + (int)modbus.query[5];

		// chamar a função externa para capturar os dados
		// 	passo o endereço do buffer a partir do 4º byte, os tres primeiros são reservador para ID, função e count para tx mais tarde
		ret = modbus.read_regs(addrInit, &modbus.query[3], len);
		if (ret == modbusNO_ERROR)
			modbus_SendPacketRegs(modbus.query, len); // Envia os valores dos registradores
		else
			modbus_SendPacketException(ret);

    // checa se a função é de escrita no registrador, processa caso há função anexada
    } else if ( (modbus.cmd == modbusCMD_WRITE_REGISTER) && (modbus.write_reg != NULL)) {
		// chamar a função externa gravar o registrador
		addrInit = ((uint) modbus.query[2] << 8) + (uint)modbus.query[3];
		u16 data = ((u16) modbus.query[4] << 8) + (u16)modbus.query[5];
		ret = modbus.write_reg(addrInit, data);
		if (ret == modbusNO_ERROR)
			modbus.puts(modbus.query, 8); // retorna o eco do pedido sinalizando ao mestre que tudo ocorreu bem
		else
			modbus_SendPacketException(ret);

    // checa se a função é de escrita nos registradores, processa caso há função anexada
    } else if ( (modbus.cmd == modbusCMD_WRITE_REGISTERS) && (modbus.write_regs != NULL)) {
    	addrInit = ((uint) modbus.query[2] << 8) + (uint)modbus.query[3];
    	len = ((int) modbus.query[4] << 8) + (int)modbus.query[5];
		// se a quantidade de registradores não corresponde com a quantidade de bytes no buffer retorna erro
		if (2*len != modbus.query[6]) ret = modbusILLEGAL_FUNCTION;
		else
			// chamar a função externa gravar nos registradores
			// 	passo o endereço do buffer a partir do 7º byte onde começa os dados
			ret = modbus.write_regs(addrInit, &modbus.query[7], len);
		if (ret == modbusNO_ERROR) { // retorna o eco do pedido sinalizando ao mestre que tudo ocorreu bem
			uint crc = crc16_MODBUS(modbus.query, 6);
			modbus.query[6] = (u8)(crc&0xFF);
			modbus.query[7] = (u8)(crc>>8);
			modbus.puts(modbus.query, 8);
		} else
			modbus_SendPacketException(ret);
	} else {
    	// envia uma mensagem de exceção ao mestre que este dispositivo não suporta tal função
    	modbus_SendPacketException(modbusILLEGAL_FUNCTION);
    }

	modbus.flushRX();
	return pdPASS;
}
