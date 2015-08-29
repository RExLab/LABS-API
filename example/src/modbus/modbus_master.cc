/* Criado em 20/04/2015
 * Atualizado 28/04/2015
 *
 * Biblioteca MODBUS mestre barramento serial rs232/rs485 no modo RTU (Binário)
 *
 * Este protocolo atende somente as funções:
 * 	 Leituta de muitos registradores, código 3,
 * 	 Escrita em um simples registrador, código 6
 * 	 Escrita em muitos registradores, código 16
 *
 * Os escravos somente capturam as mensagens quando o barramento serial fique em silencio no minimo 5ms.
 * Logo, o timeout do mestre na espera de uma resposta de um escravo deve ser superior a 5ms.
 * Geralmente usamos 3 segundos.
 *
 * Os endereços dos registradores devem ser passados pelos seus valores reais e não por valores enumerados,
 *  ou seja, valores de 0 a N.
 *
 * Para detalhes do protocolo consunte o documento D:\meus_conhecimentos\_devices_misc__\modbus\resumo_modbus.docx
 *
 * */


#include "modbus_master.h"

#if (MODBUSM_USE_DEBUG == pdON)
#if defined(LINUX)
#include <stdio.h>
#define modbus_printf printf
#else
#include "stdio_uc.h"
#define modbus_printf plognp
#endif
#endif

typedef struct {
	int (*ps)(u8 *buffer, u16 count); // Ponteiro da função para envio de bytes
	int (*gc)(u8 *ch);				// Ponteiro da função para recebimento de bytes
	void(*flushRX)(void);				// Ponteiro da função para limpar os buffers de recepção
	tTime (*now)(void);					// Função contadora de tempo decorrido
	tTime timeout; 						// Tempo de espera pela resposta do escravo após envio de um comando

	// vars aux para comunicação atual
	u8 querie[256];						// Buffer aux de transmissão e recepção de dados
	int slaveID;						// Endereço do escravo alvo no barramento para troca de dados
	int cmd;							// Comando (função) solicitado
	int waitResponse;					// Sinaliza para esperar uma resposta após envio de um comando para o escravo
	tTime tout;							// Conta o tempo na espera da resposta do escravo
	u16* regs;							// Ponteiro dos registradores envolvido na troca de dados
	int len;							// Tamanho do ponteiro
	int addr;							// Endereço do registrador para ser gravado um valor
	u16 value;							// Valor a ser gravado no registrador
	int sts;							// Status da comunicação com o escravo
											// pdPASS ou >1: Comunicação feita com sucesso
											// errMODBUS_BUFFER_OVERFLOW: Estourou o tamanho do buffer modbus
											// errMODBUS_LENPACKET: Tamanho errado do pacote de resposta do escravo
											// errMODBUS_CRC: Houve erro de CRC na resposta do escravo
											// errMODBUS_TIMEOUT: Passou o tenpo da espera pela resposta do escravo
											// errMODBUS_BUSY: Sinaliza que o gerenciador está no processo de comunicação com o escravo
											// errMODBUS_TX: Erro no envio de bytes ao escravo
											// errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
											// errMODBUS_ADDR: O endereço do registrador a ser gravado no escravo é inválido
											// errMODBUS_VALUE: Valor escrito no registrador do escravo não bate com o valor enviado
											// errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
											// errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
											// errMODBUS_LEN: O tamanho do pacote recebido do escravo não confere ao esperado
	uint exception;						// Código de exceção do modbus caso for emitido
											// modbusNO_ERROR: Sem erro de exceção
											// modbusILLEGAL_FUNCTION: 		O escravo recebeu uma função que não foi implementada ou não foi habilitada.
											// modbusILLEGAL_DATA_ADDRESS: O escravo precisou acessar um endereço inexistente.
											// modbusILLEGAL_DATA_VALUE:  O valor contido no campo de dado não é permitido pelo escravo. Isto indica uma falta de informações na estrutura do campo de dados.
											// modbusSLAVE_DEVICE_FAILURE: Um irrecuperável erro ocorreu enquanto o escravo estava tentando executar a ação solicitada.
} modbusMaster_t;

static modbusMaster_t modbus;
static int ValidatePacket(void);
static int GetPacket (void);
static int ProcessCmd3(void);
static int ProcessCmd6(void);
static int ProcessCmd16(void);

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterInit
// Descrição: 	Inicializa o protocolo modbus no modo mestre
// Parametros:	puts_func:	Ponteiro da função de transmissão serial
//				getc_func:	Ponteiro da função de recepção de dados
//				byte_available_func: Ponteiro da função para verificar se há dados no buffer de recepção serial
//				flushRX_func: Ponteiro da função que limpar os buffers seriais
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------
// ATENÇÃO: O tamanho do buffers de recepção e transmissão deve ter no mínimo o frame do modbus, recomendo deixar no mínimo 256 bytes
// se usar rs485 tem que ter cuidado para fazer a inversão do barramento. exemplo
//		int uart_PutString(u8* buffer, u16 count) {
//			rs485_ENTX_ON = rs485_ENTX;
//			int ret = uart1_WriteTx(buffer, count);
//			while (!uart1_EmptyTx()); // Vamos esperar que o serial envie a mensagem
//			rs485_ENTX_OFF = rs485_ENTX;
//			return ret;
//		}
void modbus_MasterInit(
	int(*puts_func)(u8* buffer, u16 count),
	int(*getc_func)(u8* ch),
	void(*flushRX_func)(void)
) {
	modbus.ps = puts_func;
	modbus.gc = getc_func;
	modbus.flushRX = flushRX_func;

	modbus.slaveID = 0;
	modbus.cmd = 0;
	modbus.sts = 0;
	modbus.regs = (u16*)NULL;
	modbus.len = 0;
	modbus.waitResponse = pdFALSE;
	modbus.exception = modbusNO_ERROR;
	modbus.now = (tTime (*)())NULL;
	modbus.tout = 0;
    #if (MODBUSM_USE_DEBUG == pdON)
   	modbus_printf("modbusM: INIT"CMD_TERMINATOR);
	#endif
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterAppendTime
// Descrição: 	Aponta para as funções de controle de tempo e timeout na espera da resposta do escravo
// Parametros:	now_func: Ponteiro para função de tempo e é obrigatório
//				Função deve retornar com a passagem do tempo em ms
//					tTime (*now)(void)
//				timeout:  Tempo de espera pela resposta do escravo após envio de um comando
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------
// Exemplo para readregs_func:	modbus_MasterAppendTime(now, 3000); timeout = 3000 = 3 segundos
void modbus_MasterAppendTime(tTime(*now_func)(void), int timeout) {
	modbus.now = now_func;
	modbus.timeout = timeout;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterReadStatus
// Descrição: 	Retorna com status da comunicação modbus com o escravo
// Retorna:		pdPASS ou >1: Comunicação feita com sucesso
//				errMODBUS_BUSY: Sinaliza que o gerenciador está no processo de comunicação com o escravo
//				errMODBUS_BUFFER_OVERFLOW: Estourou o tamanho do buffer modbus
//				errMODBUS_LENPACKET: Tamanho errado do pacote de resposta do escravo
//				errMODBUS_CRC: Houve erro de CRC na resposta do escravo
//				errMODBUS_TIMEOUT: Passou o tenpo da espera pela resposta do escravo
//				errMODBUS_TX: Erro no envio de bytes ao escravo
//				errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
//				errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
//				errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
//				errMODBUS_LEN
// -------------------------------------------------------------------------------------------------------------------
int modbus_MasterReadStatus(void) {
	return modbus.sts;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterReadException
// Descrição: 	Retorna com exceção ocorrinda na comunicação modbus com o escravo
// Retorna: 	modbusNO_ERROR: Sem erro de exceção
// 				modbusILLEGAL_FUNCTION: O escravo recebeu uma função que não foi implementada ou não foi habilitada.
// 				modbusILLEGAL_DATA_ADDRESS: O escravo precisou acessar um endereço inexistente.
// 				modbusILLEGAL_DATA_VALUE: O valor contido no campo de dado não é permitido pelo escravo. Isto indica uma falta de informações na estrutura do campo de dados.
// 				modbusSLAVE_DEVICE_FAILURE: Um irrecuperável erro ocorreu enquanto o escravo estava tentando executar a ação solicitada.
// -------------------------------------------------------------------------------------------------------------------
int modbus_MasterReadException(void) {
	return modbus.exception;
}

// #####################################################################################################################
// AUX
// #####################################################################################################################

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		ValidatePacket
// Descrição: 	Valida o pacote recebido, checa se o pacote recebido vem do escravo alvo, checa se o comando corresponde
//				Verifica se o escravo mandou algum código de exceção
// Retorna: 	pdPASS se o pacote é válido
//				errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
//				errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
//				errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
// -------------------------------------------------------------------------------------------------------------------
static int ValidatePacket(void) {
    // checar se o ID do escravo é o mesmo enviado
    if (modbus.querie[0] != modbus.slaveID)
		return errMODBUS_ID;

	 // checar se a função é a mesma enviada
    if ((modbus.querie[1] & 0x7f) != modbus.cmd)
        return errMODBUS_CMD;

	// checa se o escravo mandou algum erro de exceção
	if ((modbus.querie[1] & 0x80) > 0) {
        modbus.exception = modbus.querie[2];
        return errMODBUS_EXCEPTION;
	}

	return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		GetPacket
// Descrição: 	Se o gerenciador estiver esperando pela uma resposta do escravo o mesmo fica esperando por um tempo, e
//				na medida que os dados vão sendo recebiso serão adicionados no buffer.
//				Uma vez recebido todos os dados o mesmo é submetido ao teste CRC
// Retorna: 	pdPASS: Pacote recebido com sucesso
//				0: Não está esperando pela resposta do escravo
//				errMODBUS_BUFFER_OVERFLOW: Estourou o tamanho do buffer modbus
//  			errMODBUS_LENPACKET: Tamanho errado do pacote de resposta do escravo
//  			errMODBUS_CRC: Houve erro de CRC na resposta do escravo
//  			errMODBUS_TIMEOUT: Passou o tenpo da espera pela resposta do escravo
// -------------------------------------------------------------------------------------------------------------------
static int GetPacket (void) {
	static int len = 0;
    static int firstByte = pdTRUE;
	static tTime timeDataIn; // conta quanto tempo o dado é recebido do escravo. se o bus ficar em silencio mais que 10ms é porque o escravo terminou a sua transmissão

    u8 dat;

	if (!modbus.waitResponse) {
		len = 0;
		firstByte = pdTRUE;
		modbus.tout = modbus.now();
		return 0;
	}

    if (modbus.gc(&dat) == pdPASS) { 			// Checa se recebeu dados
        #if (MODBUSM_USE_DEBUG == pdON)
        modbus_printf("modbusM: getp dat 0x%x [%c] len %d"CMD_TERMINATOR, dat, dat, len);
        #endif

		firstByte = pdFALSE;                    // Sinaliza que não é o mais o primeiro byte

    	if (len >= 256) return errMODBUS_BUFFER_OVERFLOW;
        modbus.querie[len++] = dat;			// Adiciona o dado no buffer e aponta para o próximo indice do buffer
		timeDataIn = modbus.now();			// zera o tempo de espera de recebiemntos de dados do escravo

 	// se não há mais bytes no buffer serial em um determinado tempo é porque é fim de transmissão
 		// valor 10 funciona bem entre 2400 a 115200 bps. Valor 5 funcionou bem com 57600 e 115200
        // para boudrate menores pode ser que devemos aumetar esse valor.
        // Recomendo fazer uma macro associado ao boudrate da serial
 	} else if ( ( modbus.now() > timeDataIn + 10)) {
        if (!firstByte) {
        	if (len < 3) {
		        #if (MODBUSM_USE_DEBUG == pdON)
        		modbus_printf("modbusM: err len %d"CMD_TERMINATOR, len);
        		#endif

        		return errMODBUS_LENPACKET;
        	}

            // Vamos pegar o pacote deste buffer e calcular e verificar a legitimidade
            // Calcular CRC do pacote e comparar
            u16 crc_calc = crc16_MODBUS(modbus.querie, len-2);
            u16 crc = (modbus.querie[len-1] << 8 ) | modbus.querie[len-2];
            if (crc != crc_calc) {
		        #if (MODBUSM_USE_DEBUG == pdON)
        		modbus_printf("modbusM: err crc 0x%x calc 0x%x len %d"CMD_TERMINATOR, crc, crc_calc, len);
        		#endif

            	return errMODBUS_CRC;
            } else return pdPASS;

        // se ainda não recebemos o primeiro byte após um tempo vamos cancelar
        } else if ( ( modbus.now() > modbus.tout + modbus.timeout)) {
			#if (MODBUSM_USE_DEBUG == pdON)
        	modbus_printf("modbusM: err timeout"CMD_TERMINATOR);
        	#endif

			return errMODBUS_TIMEOUT;
        }
	}

	return 0;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		ProcessCmd3
// Descrição: 	Processa a resposta do escravo mediante requisição do comando 3
// Retorna:		pdPASS sinalizando que foi pego a resposta do escravo com sucesso
//				errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
//				errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
//				errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
//				errMODBUS_LEN: O tamanho do pacote recebido do escravo não confere ao esperado
// -------------------------------------------------------------------------------------------------------------------
static int ProcessCmd3(void) {
   	// Checa se este pacote é mesmo do escravo solicitado
   	int ret = ValidatePacket(); // retorna pdPASS	errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION
   	if (ret != pdPASS ) return ret;

   	// captura a quantidade de bytes recebidos
   	int countBytes = modbus.querie[2];
   	if (2*modbus.len != countBytes) return errMODBUS_LEN;

	// Tirar os valores dos registradores do bufferin para o buffer da aplicação
   	int x; for(x=0; x<modbus.len;x++)
       	*modbus.regs++ = (modbus.querie[2*x+3] << 8) | (modbus.querie[2*x+4]);

	modbus.cmd = 0; // sinaliza que não estamos mais operando nenhum comando
  	modbus.waitResponse = pdFALSE; // sinaliza que não estamos esperando pela resposta do escravo
  	return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		ProcessCmd6
// Descrição: 	Processa a resposta do escravo mediante requisição do comando 6
// Retorna:		pdPASS sinalizando que foi pego a resposta do escravo com sucesso
//				errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
//				errMODBUS_ADDR: O endereço do registrador a ser gravado no escravo é inválido
//				errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
//				errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
//				errMODBUS_LEN: O tamanho do pacote recebido do escravo não confere ao esperado
// -------------------------------------------------------------------------------------------------------------------
static int ProcessCmd6(void) {
   	// Checa se este pacote é mesmo do escravo solicitado
   	int ret = ValidatePacket(); // retorna pdPASS	errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION
   	if (ret != pdPASS ) return ret;

    // compara endereço do registrador
    int addrComp = (modbus.querie[2] << 8) | (modbus.querie[3]);
    if (modbus.addr != addrComp)  return errMODBUS_ADDR;

    // compara valor do registrador
    u16 valueComp = (modbus.querie[4] << 8) | (modbus.querie[5]);
    if (modbus.value != valueComp) return errMODBUS_VALUE;

	modbus.cmd = 0; // sinaliza que não estamos mais operando nenhum comando
  	modbus.waitResponse = pdFALSE; // sinaliza que não estamos esperando pela resposta do escravo
  	return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		ProcessCmd16
// Descrição: 	Processa a resposta do escravo mediante requisição do comando 16
// Retorna:		pdPASS sinalizando que foi pego a resposta do escravo com sucesso
//				errMODBUS_ID: O ID do escravo na sua resposta do escravo não bate com o ID do escravo na solicitação
//				errMODBUS_ADDR: O endereço do registrador a ser gravado no escravo é inválido
//				errMODBUS_CMD: O comando (função) do pacote de recebimento do escravo não bate com o comando enviado a ele
//				errMODBUS_EXCEPTION: Sinaliza que o escravo enviou uma exceção, consultar status
//				errMODBUS_LEN: O tamanho do pacote recebido do escravo não confere ao esperado
// -------------------------------------------------------------------------------------------------------------------
static int ProcessCmd16(void) {
   	// Checa se este pacote é mesmo do escravo solicitado
   	int ret = ValidatePacket(); // retorna pdPASS	errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION
   	if (ret != pdPASS ) return ret;

    // compara endereço do registrador
    int cmp = (modbus.querie[2] << 8) | (modbus.querie[3]);
    if (modbus.addr != cmp)  return errMODBUS_ADDR;

    // compara a quantidade
    cmp = (modbus.querie[4] << 8) | (modbus.querie[5]);
    if (modbus.len != cmp) return errMODBUS_VALUE;

	modbus.cmd = 0; // sinaliza que não estamos mais operando nenhum comando
  	modbus.waitResponse = pdFALSE; // sinaliza que não estamos esperando pela resposta do escravo
  	return pdPASS;
}

// #####################################################################################################################
// FUNCTIONS
// #####################################################################################################################

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterReadRegisters
// Descrição: 	Envia uma solicitação de leitura de registradores no escravo
// Retorna:		pdPASS se enviou a querie com sucesso ao escravo, ou retorna pdFAIL se houve algum erro de envio, neste caso consute status.
// ATENÇÃO: 	Quando uma querie for enviada com sucesso, ficar monitorando o status da comunicação para três tipos de respostas:
//					pdPASS: Avisar ao sistema que a leitura dos registradores foi feita com sucesso e é para capturar seus valores
//					errMODBUS_BUSY: Sinaliza que o gerenciador está no processo de comunicação com o escravo
//  				errMODBUS_XXXXX: Notificar ao sistema o tipo de erro e tomar procedimento cabíveis
//						sistema deve consultar com a função modbus_MasterReadStatus()
// -------------------------------------------------------------------------------------------------------------------
int modbus_MasterReadRegisters(int addrSlave, int addrInit, int len, u16* regs) {
	if (modbus.waitResponse) return pdFAIL;

	modbus.slaveID = addrSlave;
	modbus.cmd = 3;
	modbus.len = len;
	modbus.regs = regs;
	modbus.sts = errMODBUS_BUSY;

	modbus.flushRX(); // limpa os byffers RX da serial

   	// preparar a query
	modbus.querie[0] = modbus.slaveID;
    modbus.querie[1] = modbus.cmd;
    modbus.querie[2] = (addrInit >> 8) & 0xff;
    modbus.querie[3] = addrInit & 0xff;
    modbus.querie[4] = (len >> 8) & 0xff;
    modbus.querie[5] = len & 0xff;
    u16 crc = crc16_MODBUS(modbus.querie, 6);
    modbus.querie[6] = crc & 0xff;
    modbus.querie[7] = (crc >> 8) & 0xff;

    // enviar a query para o escravo
    if (modbus.ps(modbus.querie, 8) < 0) {
        modbus.sts = errMODBUS_TX;
        return pdFAIL;
	}

	// sinalisa que vamos esperar a resposta do escravo
	modbus.waitResponse = pdTRUE;

	#if (MODBUSM_USE_DEBUG == pdON)
	modbus_printf("modbusM: TX RR: ");
	int x; for (x=0;x<8;x++) modbus_printf("0x%x ", modbus.querie[x]);
	modbus_printf(CMD_TERMINATOR);
	#endif

    return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterWriteRegister
// Descrição: 	Envia uma solicitação de escrita a um registrador no escravo
// Retorna:		pdPASS se enviou a querie com sucesso ao escravo, ou retorna pdFAIL se houve algum erro de envio, neste caso consute status.
// ATENÇÃO: 	Quando uma querie for enviada com sucesso, ficar monitorando o status da comunicação para três tipos de respostas:
//					pdPASS: Avisar ao sistema que a escrita no registrador foi feita com sucesso
//					errMODBUS_BUSY: Sinaliza que o gerenciador está no processo de comunicação com o escravo
//  				errMODBUS_XXXXX: Notificar ao sistema o tipo de erro e tomar procedimento cabíveis.
//						sistema deve consultar com a função modbus_MasterReadStatus()
// -------------------------------------------------------------------------------------------------------------------
int modbus_MasterWriteRegister(int addrSlave, int addr, u16 value) {
	if (modbus.waitResponse) return pdFAIL;

	modbus.slaveID = addrSlave;
	modbus.cmd = 6;
	modbus.addr = addr;
	modbus.value = value;
	modbus.sts = errMODBUS_BUSY;

	modbus.flushRX(); // limpa os byffers RX da serial

    // preparar a query
    modbus.querie[0] = modbus.slaveID;
    modbus.querie[1] = modbus.cmd;
    modbus.querie[2] = (addr >> 8) & 0xff;
    modbus.querie[3] = addr & 0xff;
    modbus.querie[4] = (value >> 8) & 0xff;
    modbus.querie[5] = value & 0xff;
    u16 crc = crc16_MODBUS(modbus.querie, 6);
    modbus.querie[6] = crc & 0xff;
    modbus.querie[7] = (crc >> 8) & 0xff;

	// enviar a query para o escravo
    if (modbus.ps(modbus.querie, 8) < 0) {
        modbus.sts = errMODBUS_TX;
        return pdFAIL;
	}

	// sinalisa que vamos esperar a resposta do escravo
	modbus.waitResponse = pdTRUE;

	#if (MODBUSM_USE_DEBUG == pdON)
	modbus_printf("modbusM: TX WR: ");
	int x; for (x=0;x<8;x++) modbus_printf("0x%x ", modbus.querie[x]);
	modbus_printf(CMD_TERMINATOR);
	#endif

    return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterWriteRegisters
// Descrição: 	Envia uma solicitação de escrita de registradores no escravo
// Retorna:		pdPASS se enviou a querie com sucesso ao escravo, ou retorna pdFAIL se houve algum erro de envio, neste caso consute status.
// ATENÇÃO: 	Quando uma querie for enviada com sucesso, ficar monitorando o status da comunicação para três tipos de respostas:
//					pdPASS: Avisar ao sistema que a escrita no registrador foi feita com sucesso
//					errMODBUS_BUSY: Sinaliza que o gerenciador está no processo de comunicação com o escravo
//  				errMODBUS_XXXXX: Notificar ao sistema o tipo de erro e tomar procedimento cabíveis.
//						sistema deve consultar com a função modbus_MasterReadStatus()
// -------------------------------------------------------------------------------------------------------------------
int modbus_MasterWriteRegisters(int addrSlave, int addrInit, int len, u16* regs) {
	if (modbus.waitResponse) return pdFAIL;

	modbus.slaveID = addrSlave;
	modbus.cmd = 16;
	modbus.addr = addrInit;
	modbus.len = len;
	modbus.regs = regs;
	modbus.sts = errMODBUS_BUSY;

	modbus.flushRX(); // limpa os byffers RX da serial

    // preparar a query
    modbus.querie[0] = modbus.slaveID;
    modbus.querie[1] = modbus.cmd;
    modbus.querie[2] = (addrInit >> 8) & 0xff;
    modbus.querie[3] = addrInit & 0xff;
    modbus.querie[4] = (len >> 8) & 0xff;
    modbus.querie[5] = len & 0xff;
    modbus.querie[6] = 2*len;

    int x; for(x=0;x<len;x++) {
        modbus.querie[7+2*x] = *regs >> 8;
        modbus.querie[8+2*x] = *regs & 0xff;
        regs++;
	}

    u16 crc = crc16_MODBUS(modbus.querie, 7+(2*len));
    modbus.querie[7+2*len] = crc & 0xff;
    modbus.querie[8+2*len] = (crc >> 8) & 0xff;

    // enviar a query para o escravo
    if (modbus.ps(modbus.querie, 9+2*len) < 0) {
        modbus.sts = errMODBUS_TX;
        return pdFAIL;
	}

   	// sinalisa que vamos esperar a resposta do escravo
	modbus.waitResponse = pdTRUE;

	#if (MODBUSM_USE_DEBUG == pdON)
	modbus_printf("modbusM: TX WRs: ");
	for (x=0;x<9+2*len;x++) modbus_printf("0x%x ", modbus.querie[x]);
	modbus_printf(CMD_TERMINATOR);
	#endif

    return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// FUNÇÃO:		modbus_MasterProcess
// Descrição: 	Processa as respostas do escravo mediante as requisiççies de comandos
// -------------------------------------------------------------------------------------------------------------------
void modbus_MasterProcess(void) {
    int ret = GetPacket(); // retorna
		//		Quantidade de bytes recebidos com sucesso
		//		0: Não está esperando pela resposta do escravo
		//		errMODBUS_BUFFER_OVERFLOW
		//  	errMODBUS_LENPACKET
		//  	errMODBUS_CRC
		//  	errMODBUS_TIMEOUT

	if (ret == 0 ) return;
   	if (ret < 0 ) {
   		modbus.sts = ret; 				// salva o erro
   		modbus.waitResponse = pdFALSE; 	// sinaliza que não estamos esperando pela resposta do escravo
	} else {
		if (modbus.cmd == 3) 		modbus.sts = ProcessCmd3();	// retorna	pdPASS errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION errMODBUS_LEN
		else if (modbus.cmd == 6) 	modbus.sts = ProcessCmd6();	// retorna	pdPASS errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION errMODBUS_ADDR errMODBUS_VALUE
		else if (modbus.cmd == 16) 	modbus.sts = ProcessCmd16();// retorna	pdPASS errMODBUS_ID	errMODBUS_CMD errMODBUS_EXCEPTION errMODBUS_ADDR errMODBUS_VALUE
	}
}
