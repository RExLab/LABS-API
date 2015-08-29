#ifndef APP_H
#define APP_H

// ###########################################################################################################################################
// DESENVOLVIMENTO
#define LOG_MODBUS		pdON

// ###########################################################################################################################################
// MODEL WORKTEMP

#define MODEL			"ms10"	// somente 4 chars, por causa so modbus
#define FIRMWARE_VERSION "1.0"	// usar x.y só com uma casa decimal, pois estamos usando a impressão de temperatura
								// usar 3 chars por causa do modbus
#define PROGAM_NAME "MSIP" MODEL " firmware v" FIRMWARE_VERSION " [compiled in " __DATE__ " " __TIME__ "]" CMD_TERMINATOR


// ###########################################################################################################################################
// INCLUDES


// ###########################################################################################################################################
// MODBUS
#define COM_PORT "/dev/ttyAMA0" 	// Consultar /dev/ttySxxx do linux
									// Opensuse com placa pci para serial = "/dev/ttyS4"
									// Raspberry "/dev/ttyAMA0"
									// Para descobrir qual o nó da porta UART faça o seguinte:
									// 		Conecte o RASP com um PC via cabo serial.
									//		No PC com um terminal de comnicação serial abra uma conexão com RASP a 115200bps 8N1
									//		No RAPS liste as portas tty: ls tty*
									// 		No RAPS na linha de comando envie uma mensagem para cada porta serial: echo ola /dev/ttyXXXX
									//			Faça para para todas as portas listadas até que receba a mensagem no terminal do PC

#define MODBUS_BAUDRATE		B57600 // Não usar acima de 57600, pois há erro de recepção do raspberry.
									// Deve ser algum bug de hardware do rasp porque o baudrate do rasp não fica indentico do ARM
									// pois a comunicação com PC a 115200 funciona bem.
									// Ou a tolerancia de erro no rasp não é tão grande como no PC onde o ARM tem um erro considerável
									//	TODO Quando usar o oscilador interno do ARM refazer os testes a sabe com usando oscilador interno do ARM isso se resolve

// ###########################################################################################################################################
// CONTROLE DO SISTEMA

typedef struct {
	int stsCom;	// Status de comunicação com multimetro via modbus
				//		0: Sem comunicação com o RH. O mesmo não está conectado, ou está desligado, ou não há dispositivo neste endereço.
				//		1: O RH recebeu uma função que não foi implementada;
				//		2: Foi acessado a um endereço de registrador inexistente;
				//		3: Foi tentado gravar um valor inválido no registrador do RH;
				//		4: Um irrecuperável erro ocorreu enquanto o RH estava tentando executar a ação solicitada;
				//		5: Comunicação estabelecida com sucesso

	int func;	// Função assumida do multímetro:
				//		0: Amperímetro;
				// 		1: Voltímetro.
	int sts;	// Status do sensor
				// 		0: Sinaliza que o multímetro está lendo pela primeira vez o sensor. Isto somente acontece no momento que o multímetro é ligado.
				// 		1: O multímetro já contém o valor convertido;
				// 		2: Sinaliza um erro, indica que o valor está abaixo da escala permitida pelo multímetro;
				// 		3: Sinaliza um erro, indica que o valor está acima da escala permitida pelo multímetro;
	int value; 	// valores em miliampers ou milivolts
} tMultimeter;
#define nMULTIMETER  16

// função que o MSIP deve assumir referente ao experimento que deve ser gerenciado
typedef enum {
	funcPANEL_ELETRIC = 0	// Experimento do painel elétrico
} tFuncEx;

typedef struct {
	unsigned exit:1;		// 1 Sinaliza para cair fora do programa
	unsigned getInfo:1;		// Sinaliza para capturar as informações do recurso de hardware
	unsigned getRelays:1;	// Sinaliza para capturar as informações dos reles do recurso de hardware
	unsigned getDouts:1;	// Sinaliza para capturar as informações das saídas digitais do recurso de hardware
	tFuncEx funcEx;			// Qual a função o MSIP deve assumir para gerenciar o experimento

	int stsCom;				// Status de comunicação com RH via modbus
							//		0: Sem comunicação com o RH. O mesmo não está conectado, ou está desligado, ou não há dispositivo neste endereço.
							//		1: O RH recebeu uma função que não foi implementada;
							//		2: Foi acessado a um endereço de registrador inexistente;
							//		3: Foi tentado gravar um valor inválido no registrador do RH;
							//		4: Um irrecuperável erro ocorreu enquanto o RH estava tentando executar a ação solicitada;
							//		5: Comunicação estabelecida com sucesso
	uint rhID;				// Qual o ID do RH no barramento modbus
	string rhModel;			// Modelo do RH
	string rhFirmware;		// Firmware versão
	uint douts;				// estados das saidas digitais
		//#define nOUTS 	9
	uint relays;			// estados dos reles
		//#define nRELAY 	5
	uint nMultimetersGeren;	// Determina a quantidade de multimetros o RH deve gerenciar, inicio pelo ID=1
		#define nMULTIMETER_GEREN 9 // TODO colocar valor 9
	tMultimeter multimeter[nMULTIMETER];
	uint relaysOld, doutsOld;
} tControl, *pControl;




typedef enum {
    cmdNONE = 0,
    cmdGET_INFOS,        // Comando para ler os registradores: modelo, versão firmware e modo de operações dos reles
    cmdGET_MULTIMETERS, 	// Comando para ler os sensores
    cmdGET_DOUTS,        // Comando para ler os estados das saídas digitais do RH
    cmdSET_DOUTS,        // Comando para gravar os estados das saídas digitais do RH
    cmdGET_RELAYS,       // Comando para ler os estados dos reles do RH
    cmdSET_RELAYS        // Comando para gravar os estados dos reles do RH
} tCommand;

typedef enum {readREGS, writeREG, writeREGS} tcmd;


// ###############################################################################
// PROTOTIPOS

int modbus_Init(void);
void modbus_SendCommand( tCommand c) ;
void init_control_tad();
void * modbus_Process(void * params);

#endif
