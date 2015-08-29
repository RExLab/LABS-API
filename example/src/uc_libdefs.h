#ifndef __UC_LIBDEFS_H
#define __UC_LIBDEFS_H

#define __VERSION "uC_libc2.7"

// ###################################################################################################################
// DIFINIÇÕES DOS BITS
// ###################################################################################################################
#define BIT_0 (1<<0)
#define BIT_1 (1<<1)
#define BIT_2 (1<<2)
#define BIT_3 (1<<3)
#define BIT_4 (1<<4)
#define BIT_5 (1<<5)
#define BIT_6 (1<<6)
#define BIT_7 (1<<7)
#define BIT_8 (1<<8)
#define BIT_9 (1<<9)
#define BIT_10 (1<<10)
#define BIT_11 (1<<11)
#define BIT_12 (1<<12)
#define BIT_13 (1<<13)
#define BIT_14 (1<<14)
#define BIT_15 (1<<15)
#define BIT_16 (1<<16)
#define BIT_17 (1<<17)
#define BIT_18 (1<<18)
#define BIT_19 (1<<19)
#define BIT_20 (1<<20)
#define BIT_21 (1<<21)
#define BIT_22 (1<<22)
#define BIT_23 (1<<23)
#define BIT_24 (1<<24)
#define BIT_25 (1<<25)
#define BIT_26 (1<<26)
#define BIT_27 (1<<27)
#define BIT_28 (1<<28)
#define BIT_29 (1<<29)
#define BIT_30 (1<<30)
#define BIT_31 (1<<31)

// ###################################################################################################################
// DEFINIÇÕES DE ESCOLHA DE DIPOSTIVOS NAS BIBLIOTECAS
// ###################################################################################################################
// PADRÃO DE ENTRADA E SAIDA
#define STDIO_USE_UART0	0
#define STDIO_USE_UART1	1
#define STDIO_USE_UART2	2
#define STDIO_USE_UART3	3
#define STDIO_USE_LCD	4

// LIBS DOS DISPLAYS
#define LCD_CONNECTION_GPIO		0				// Acesso ao display é via porta IO
#define LCD_CONNECTION_IOMAP  	1				// Acesso ao display é mapeada em memória
#define glcdCONNECTION_GPIO		0				// Acesso ao display é via porta IO
#define glcdCONNECTION_IOMAP  	1				// Acesso ao display é mapeada em memória

// TIPOS DE NIC E ORDEM DO BYTE USADA PELA PILHA TCPI/IP UIP
#define UIP_USE_NIC_RTL8019  	0
#define UIP_USE_NIC_ENC28J60 	1
#define UIP_USE_NIC_AX88796 	2

#define UIP_LITTLE_ENDIAN  3412
#define UIP_BIG_ENDIAN     1234

// ###################################################################################################################
// DEFINIÇÕES DE MACROS
// ###################################################################################################################
#define PI		3.14159265359
#ifndef _BV
#define _BV(n)              (1 << (n))
#endif
#define __BIT_MASK(A) (1<<(A))

#define set_bit(x,y) 		(x |= (y)) 					// Seta  o bit y no byte x
#define clear_bit(x,y) 		(x &= (~y)) 				// Limpa o bit y no byte x
#define check_bit(x,y) 		(x & (y)) 					// Checa o bit y no byte x

// Macros de minimo maximo e absoluto
#ifndef min
#define min(a,b)			((a<b)?(a):(b))
#define max(a,b)			(((a)>(b))?(a):(b))
#endif
#define arrsizeof(x) 		((sizeof (x))/(sizeof (x [0])))	

//#ifndef abs
//#define abs(x)				(((x)>0)?(x):(-x))
//#endif

#define __builtin_expect(x, expected_value) (x)
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define percentage(value, per) 	(n32)(((n64)value * per)/100) //Traz a porcentagem desejada do valor inteiro

// Transforma byte em caractere ascii, os valores ascii deve ser mostrado no formato %c
#define byte2ascii_UpperNibble(x) ((x>>4) < 10) ? (x>>4) + 48 : (x>>4) + 55   			// upper nibble
#define byte2ascii_LowerNibble(x) ((x&0x0f) < 10) ? (x&0x0f) + 48 : (x&0x0f) + 55  		// lower nibble

#if defined(CPU_AVR)	
#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond
#define us2cnt(us) ((u16)((u32)(us) / (1000000L / (float)((u32)F_CPU / 1L)))) // Converte microsegundos em valor de contadores
// Mudar e testar estados dos bits

// SOMENTE FUNCIONA COM VARIAVEIS
#define cbi(reg,bit)	reg &= ~(_BV(bit))
#define sbi(reg,bit)	reg |= (_BV(bit))
#define cli()			__asm__ __volatile__ ("cli" ::)
#define sei()			__asm__ __volatile__ ("sei" ::)
#else
#define us2cnt(us) ((u32)((float)(us) / (1000000L / (float)((u32)CCLK / 1L)))) // Converte microsegundos em valor de contadores em relação ao clock da CPU
#define us2cnt_Perif(us) ((u32)((float)(us) / (1000000L / (float)((u32)PCLK / 1L)))) // Converte microsegundos em valor de contadores em relação ao clock dos periféricos
#endif

#define nop()  __asm__ __volatile__ ("nop" ::)


// ###################################################################################################################
// DEFINIÇÕES DE TIPOS DE VARIÁVEIS USADAS NAS BIBLIOTECAS
// ###################################################################################################################

typedef volatile unsigned char 	reg8;
typedef volatile unsigned short reg16;
typedef volatile unsigned long 	reg32;

typedef unsigned char 			u8;
typedef unsigned short 			u16;
typedef unsigned long 			u32;
typedef unsigned long long		u64;

typedef char 					n8;
typedef short 					n16;
typedef long					n32;
typedef long long				n64;

typedef signed char				s8;
typedef signed short			s16;
typedef signed long				s32;
typedef signed long long		s64;

typedef unsigned int			uint;
typedef int						boolean;

// não podemos usar como typedef por causa do erro de compilaçãos dos tipos
//	delclarados typedef unsigned portLONG portTickType;
// principalmente no freeRTOS
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long


#if defined(CPU_AVR)
	#define portSHORT		int
	#define portSTACK_TYPE	unsigned portCHAR
	#define __STRINGSIZE__ 		32
	typedef volatile unsigned short regCpu;
#elif defined(LINUX)
	#define __STRINGSIZE__ 		256
#else
	#define portSHORT		short
	#define portSTACK_TYPE	unsigned portLONG
	#define __STRINGSIZE__ 		64
	typedef volatile unsigned long regCpu;
#endif

typedef unsigned long 			tTime;
typedef const char* 			cpchar;
typedef char* 					pchar;
typedef char 					string[__STRINGSIZE__];

typedef struct {
	int x,y;
}tPoint;

// Retangulo pos X e Y, e com largura e altura
typedef struct {
	int x, y, w ,h;
}tRect;

// Retangulo com centro, largura e altura
typedef struct {
	int cx, cy, w ,h;
}tRectC;

#define RECTX_XI(r)		(r.cx-r.w/2)
#define RECTX_XF(r)		(r.cx+r.w/2)
#define RECTX_YI(r)		(r.cy-r.h/2)
#define RECTX_YF(r)		(r.cy+r.h/2)
#define RECT_XF(r)		(r.cx+r.w)
#define RECT_YF(r)		(r.cy+r.h)


// ###################################################################################################################
// CÓDIGO DAS FUNÇÕES DE RETORNO. O TIPO DE RETORNO DAS FUNÇÕES DEVE SER int
// ###################################################################################################################
// 		Todas as routinas de retorno é do tipo int
// UC_LIBC. Deve ser entre -65535 a 65536, isto por causa dos uC de 8 bits onde int é de 16 bits

// GERAL
#define pdTRUE				( 1 )
#define pdFALSE				( 0 )
#define pdPASS				( 1 )
#define pdOK				( 1 )
#define pdNO				( 0 )
#define pdFAIL				( 0 )
#define pdON 				( 1 )
#define pdOFF 				( 0 )

#define TIMEROUT_1SEC		1000000L
#define TIMEROUT_2SEC		2000000L
#define TIMEROUT_3SEC		3000000L
#define TIMEROUT_4SEC		4000000L
#define TIMEROUT_5SEC		5000000L
#define TIMEROUT_6SEC		6000000L
#define TIMEROUT_7SEC		7000000L
#define TIMEROUT_8SEC		8000000L
#define TIMEROUT_9SEC		9000000L

#ifndef EOF
#define EOF 	(int)(-1)
#endif

#ifndef NULL
#define NULL    ((void *)0)
#endif

// -------------------------------------------------------------------------------------------------------------------
// CÓDIGOS DE ERROS
//		Se não haver erros retorne pdPASS

// FREE RTOS
typedef void (*pdTASK_CODE)( void * );
#define errQUEUE_EMPTY							(0)
#define errQUEUE_FULL							(0)
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY	(-1)
#define errNO_TASK_TO_RUN						(-2)
#define errQUEUE_BLOCKED						(-4)
#define errQUEUE_YIELD							(-5)
#define errQUEUE_CREATE_ERROR					(-6)
#define errSEMAPHORE_CREATE_ERROR				(-7)

#define errMALLOC_ERROR							(-8)
#define errREG_EMPTY							(-9)

#define errBUFFER_FULL							(-10)
#define errBUFFER_EMPTY							(-11)
#define errBUFFER_NO_EXIST						(-12)

#define errPASSWORD_INVALID						(-13)

#define errI2C_ERROR							(-20)
#define errI2C_DEVICE_BUSY						(-21)
#define errI2C_DEVICE_NO_DATA					(-22)

#define errSPI_DEVICE_NO_DATA					(-30)
#define errSPI_ERROR_SET_MODE					(-31)
#define errSPP0_ERROR_SET_MODE					(-31)
#define errSPP1_ERROR_SET_MODE					(-31)
	
#define errADC_LOST_DATA						(-40)

#define errTIMEROUT								(-50)

#define errCLI_EXED_ARGS						(-60)
#define errCLI_MANY_ARGS						(-61)
#define errCLI_FEW_ARGS							(-62)
#define errCLI_CMD_NOT_FOUND					(-63)

#define errMODEM_CANCEL_REMOTE					(-70)
#define errMODEM_USER_CANCEL					(-71)
#define errMODEM_SYNC							(-72)
#define errMODEM_MANY_RETRANSMISSION			(-73)
#define errMODEM_XINIT 							(-74)
#define errMODEM_NACK 							(-75)
#define errMODEM_FILE_EMPTY 					(-76)
#define errMODEM_NO_RESPONSE					(-77)
#define errMODEM_ACK							(-78)
#define errMODEM_CHECKSUM						(-79)
#define errMODEM_TIMEROUT						(-80)
#define	errMODEM_MEM_OVERFLOW					(-81)
#define errMODEM_UNKNOWN						(-82)
#define errMODEM_COMMAND_INVALID				(-83)

#define errSPI_FLASH_SEFTEST					(-90)
#define errSPI_FLASH_WRITE						(-91)
#define errSPI_FLASH_ERASE						(-92)

#define errIAP_COMMAND_INVALID         			(-93)
#define errIAP_SRC_ADDR			      			(-95)
#define errIAP_DST_ADDR			       			(-96)
#define errIAP_SRC_ADDR_NOT_MAPPED 				(-97)
#define errIAP_DST_ADDR_NOT_MAPPED 				(-98)
#define errIAP_COUNT			       			(-99)
#define errIAP_SECTOR_INVALID       			(-90)
#define errIAP_SECTOR_NOT_BLANK     			(-100)
#define errIAP_SECTOR_NOT_PREPARED 				(-101)
#define errIAP_COMPARE		        			(-102)
#define errIAP_BUSY                 			(-103)

#define errIAP_ADDR_INVALID           			(-104)
#define errIAP_SIZE_INVALID          			(-105)
#define errIAP_PARAM_INVALID           			(-106)
#define errIAP_NO_SAFE_REGION          			(-107)

#define errINTEL_HEX_DELIMITER 					(-108)
#define errINTEL_HEX_CHECKSUM 					(-109)
#define errINTEL_HEX_NUMBER_INVALID				(-110)
#define errINTEL_HEX_COMMAND_INVALID			(-111)

#define errNIC_NO_RESPONSE						(-112)

// MODBUS
#define errMODBUS_BUFRX_INCOMPLETE				(-120)
#define errMODBUS_BUFRX_OVERFLOW 				(-121)
#define errMODBUS_CRC 							(-122)
#define	errMODBUS_TX 							(-123)
#define errMODBUS_TIMEOUT 						(-124)
#define errMODBUS_BUFFER_OVERFLOW 				(-125)
#define errMODBUS_LENPACKET 					(-126)
#define errMODBUS_ID 							(-127)
#define errMODBUS_ADDR 							(-128)
#define errMODBUS_CMD 							(-129)
#define errMODBUS_EXCEPTION						(-130)
#define errMODBUS_LEN 							(-131)
#define errMODBUS_VALUE 						(-132)
#define errMODBUS_OPEN_UART 					(-133)
#define errMODBUS_BUSY 							(-134)

// USB HOST
#define errUSB_TD_FAIL              			(-150)
#define errUSB_DEVICE_NO_PRESENT   				(-151)

// USB HOST MASS STORE
#define errUSB_MS_CMD_FAILED       	   			(-152)
#define errUSB_MS_BAD_CONFIGURATION   			(-153)
#define errUSB_MS_NO_INTERFACE   				(-154)

// -------------------------------------------------------------------------------------------------------------------
// CPU_ARM_LPC
#define errIRQ_INSTALL							( -1000 )
#define errIRQ_SLOT_FULL						( -1001 )
#define errIRQ_SLOT_OUT_OF_RANGE				( -1002 )


// ###################################################################################################################
// CONTROLE DE DISCO DE ARMAZENAMENTOS DE DADOS
// ###################################################################################################################

// TIPO DE DISCO DE ARMAZENAMENTO DE DADOS
typedef enum {
  	DISK_TYPE_MMC = 0, 	// MMC ver 3
  	DISK_TYPE_SD1,		// SD ver 1
  	DISK_TYPE_SD2,		// SD ver 2
  	DISK_TYPE_SDC,		// SD
  	DISK_TYPE_SD_BLOCK,	// Endereçamen to por blocos
  	DISK_TYPE_UNKNOWN,
} disk_type_t;

// ESTRUTURA PARA GUARDAR DADOS ESPCIFICOS DO CARTÃO EM USO
typedef struct {
  	u32  block_number;
  	u32  block_size;
  	disk_type_t disk_type;
 	boolean write_protect;
} tDiskStatus;


// ###################################################################################################################
// CORES
// ###################################################################################################################
typedef unsigned long 			tColor;

#define RGB24(r,g,b)			(tColor)((u8)r|((u8)g<<8)|((u8)b<<16))
#define RGB16(r,g,b)			(tColor)((u8)((b>>3)&0x1F)|((u8)((g>>2)&0x3F)<<5)|((u8)((r>>3)&0x1F)<<11))

//						  __BBGGRR
#define clBlack 		RGB16(0x00,0x00,0x00)
#define clMaroon 		RGB16(0x00,0x00,0x80)
#define clGreen 		RGB16(0x00,0x80,0x00)
#define clOlive 		RGB16(0x00,0x80,0x80)
#define clNavy 			RGB16(0x80,0x00,0x00)
#define clPurple 		RGB16(0x80,0x00,0x80)
#define clTeal 			RGB16(0x80,0x80,0x00)
#define clGray 			RGB16(0x80,0x80,0x80)
#define clSilver 		RGB16(0xC0,0xC0,0xC0)
#define clRed 			RGB16(0x00,0x00,0xFF)
#define clLime 			RGB16(0x00,0xFF,0x00)
#define clYellow 		RGB16(0x00,0xFF,0xFF)
#define clBlue 			RGB16(0xFF,0x00,0x00)
#define clFuchsia 		RGB16(0xFF,0x00,0xFF)
#define clAqua 			RGB16(0xFF,0xFF,0x00)
#define clCyan 			RGB16(0xFF,0xFF,0x00)
#define clLtGray 		RGB16(0xC0,0xC0,0xC0)
#define clDarkGray 		RGB16(0x80,0x80,0x80)
#define clNone			0x1FFFFFFF
#define clDarkBlue 		RGB16(0x40,0x00,0x00)
#define clDarkYellow 	RGB16(0x00,0xC0,0xC0)
#define clWhite 		RGB16(0xFF,0xFF,0xFF)
/*
					RRGGBB
#define clMaroon	#800000
#define clDark red	#8B0000
#define clBrown	#A52A2A
#define clFireBrick	#B22222
*/
#define clCrimson		RGB16(0x3C,0x14,0xDC)
/*
#define clRed	#FF0000
#define clTomato	#FF6347
#define clCoral	#FF7F50
#define clIndianRed	#CD5C5C
#define clLightCoral	#F08080
#define clDarkSalmon	#E9967A
#define clSalmon	#FA8072
#define clLightSalmon	#FFA07A
*/
#define clOrangeRed		RGB16(0x00,0x45,0xFF)
#define clDarkOrange	RGB16(0x00,0x8C,0xFF)
#define clOrange		RGB16(0x00,0xA5,0xFF)
#define clGold			RGB16(0x00,0xD7,0xFF)
/*
#define clDark golden rod	#B8860B
#define clGolden rod	#DAA520
#define clPale golden rod	#EEE8AA
#define clDark khaki	#BDB76B
#define clKhaki	#F0E68C
#define clOlive	#808000
#define clYellow	#FFFF00
#define clYellowGreen	#9ACD32
#define clDarkOliveGreen	#556B2F
#define clOlive drab	#6B8E23
#define clLawn green	#7CFC00
#define clChart reuse	#7FFF00
#define clGreen yellow	#ADFF2F
#define clDarkGreen	#006400
#define clGreen	#008000
#define clForest green	#228B22
#define clLime	#00FF00
#define clLime green	#32CD32
#define clLight green	#90EE90
#define clPale green	#98FB98
#define clSarkSeaGreen	#8FBC8F
#define clMediumSpringGreen	#00FA9A
*/
#define clSpringGreen		RGB16(0x7F, 0xFF, 0x00)
/*
#define clSea green	#2E8B57
#define clMedium aqua marine	#66CDAA
#define clMedium sea green	#3CB371
#define cllight sea green	#20B2AA
#define cldark slate gray	#2F4F4F
#define clteal	#008080
#define cldark cyan	#008B8B
#define claqua	#00FFFF
#define clcyan	#00FFFF
#define cllight cyan	#E0FFFF
#define cldark turquoise	#00CED1
*/
#define clTurquoise			RGB16(0xD0, 0xE0, 0x40)
#define clMediumTurquoise	RGB16(0xCC, 0xD1, 0x48)
#define clPaleTurquoise		RGB16(0xEE, 0xEE, 0xAF)
#define clAquaMarine		RGB16(0xD4, 0xFF, 0x7F)
#define clPowderBlue		RGB16(0xE6, 0xE0, 0xB0)
#define clCadetBlue			RGB16(0xA0, 0x9E, 0x5F)
#define clSteelBlue			RGB16(0xB4, 0x82, 0xB4)
/*
#define clcorn flower blue	#6495ED
#define cldeep sky blue	#00BFFF
*/
#define clDodgerBlue		RGB16(0xFF, 0x90, 0x1E)
/*
#define cllight blue	#ADD8E6
#define clsky blue	#87CEEB
#define cllight sky blue	#87CEFA
*/
#define clMidNightBlue		RGB16(0x70, 0x19, 0x19)
/*
#define clnavy	#000080
#define cldark blue	#00008B
#define clmedium blue	#0000CD
#define clblue	#0000FF
#define clroyal blue	#4169E1
#define clblue violet	#8A2BE2
#define clindigo	#4B0082
#define cldark slate blue	#483D8B
#define clslate blue	#6A5ACD
#define clmedium slate blue	#7B68EE
#define clmedium purple	#9370DB
#define cldark magenta	#8B008B
#define cldark violet	#9400D3
#define cldark orchid	#9932CC
#define clmedium orchid	#BA55D3
#define clpurple	#800080
#define clthistle	#D8BFD8
#define clplum	#DDA0DD
#define clviolet	#EE82EE
#define clmagenta / fuchsia	#FF00FF
#define clorchid	#DA70D6
#define clmedium viole tred	#C71585
#define clpale violet red	#DB7093
#define cldeep pink	#FF1493
#define clhot pink	#FF69B4
#define cllight pink	#FFB6C1
#define clpink	#FFC0CB
#define clantique white	#FAEBD7
#define clbeige	#F5F5DC
#define clbisque	#FFE4C4
#define clblanched almond	#FFEBCD
#define clwheat	#F5DEB3
#define clcorn silk	#FFF8DC
#define cllemon chiffon	#FFFACD
#define cllight golden rod yellow	#FAFAD2
#define cllight yellow	#FFFFE0
#define clsaddle brown	#8B4513
#define clsienna	#A0522D
#define clchocolate	#D2691E
#define clperu	#CD853F
#define clsandy brown	#F4A460
#define clburly wood	#DEB887
#define cltan	#D2B48C
#define clrosy brown	#BC8F8F
#define clmoccasin	#FFE4B5
#define clnavajo white	#FFDEAD
#define clpeach puff	#FFDAB9
#define clmisty rose	#FFE4E1
#define cllavender blush	#FFF0F5
#define cllinen	#FAF0E6
#define clold lace	#FDF5E6
#define clpapaya whip	#FFEFD5
#define clsea shell	#FFF5EE
#define clmint cream	#F5FFFA
#define clslate gray	#708090
#define cllight slate gray	#778899
#define cllight steel blue	#B0C4DE
#define cllavender	#E6E6FA
#define clfloral white	#FFFAF0
#define clalice blue	#F0F8FF
#define clghost white	#F8F8FF
#define clhoneydew	#F0FFF0
#define clivory	#FFFFF0
#define clazure	#F0FFFF
#define clsnow	#FFFAFA
#define clblack	#000000
#define cldim gray / dim grey	#696969
#define clgray / grey	#808080
#define cldark gray / dark grey	#A9A9A9
#define clsilver	#C0C0C0
#define cllight_gray 	RGB16(#D3D3D3
#define clgainsboro		RGB16(#DCDCDC
#define clWhiteSmoke	RGB16(#F5F5F5

*/

// ###################################################################################################################
// TFT e TOUCH
// ###################################################################################################################
#define TFT_ROTATE_NONE		0 	// Não rotacionar TFT
#define TFT_ROTATE_UD 		1 	// Rotacionar TFT 180º deixando a tela de cabeça para baixo;
//#define TFT_ROTATE_CW		// Rotacionar TFT no sentido horário (90º);
//#define TFT_ROTATE_CCW - Rotacionar no sentido anti-horário (90º).

#define TOUCH_USE_AD 			0 	// Utiliza a porta AD do uC para leitura do touchscreen
#define TOUCH_USE_TSC2046 		1	// Utiliza o CI TSC2046 para leitura do touchscreen VIA SPI


// ###################################################################################################################
// DISPOSITIVOS STDIO
// ###################################################################################################################
#define STDIO_LPRINTF		1
#define STDIO_DEBUG			2
#define STDIO_ERROR			3

#define STDIO_DEV_UART0 	4
#define STDIO_DEV_UART1 	5
#define STDIO_DEV_UART2 	6
#define STDIO_DEV_UART3 	7
#define STDIO_DEV_LCD 		8

#endif
