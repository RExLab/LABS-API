/*
Atualizado em 17/03/2015

Autor Diego Fontana

leia-me

	o calculo do CRC eh feito em cima dos polinomios, como por exemplo o
MODBUS, ele utiliza o polinomio "X^16+X^15+X^2+X^0" que em hexa é 0x8005,
as rotinas de calculos de CRC precisam de um valor inicial par ao CRC
é do polinomio, o CRC é calculado em cima de cada BYTE do pacote.

	Existem duas forma de calcular o CRC, uma rotacionando para diretira
e outra para esquerda, aqui temos duas duas rotinas genéricas:
	crc16_Right: calcula o CRC de modo Rigth Shifted
	crc16_Left: calcula o CRC de modo Left Shifted

	Por algum motivo que eu ainda nao sei, cada sistema utiliza sua proprio
combinação de crc16_Right ou crc16_Left, polinomio e valor inicial.

	O detalhe na rotina crc16_Right (Right Shifted) é que o polinomio utilizado
deve estar espelhado, por exemplo o MODBUS utiliza o valor 0x8005, portando
o polinomio real a ser calculado na rotina é 0xA001. Por isso a função crc16_Poly
faz o espelhamento do polinomio, ajustando para o uso correto.

	O calculo do CRC para DNP3 é um pouco mais estranho, se for avaliado
o CRC de byte a byte ele fecha com a tabela encontrada neste site
http://www.experts-exchange.com/Programming/Languages/Visual_Basic/Q_11722859.html
mas o valor final utilizado deve ser aplicado complemento "~" e os BYTES da word
estão com os valores ALTO e BAIXO trocados.

	[REFERENCIAS]

	http://en.wikipedia.org/wiki/Cyclic_redundancy_check
	http://www.lammertbies.nl/comm/info/crc-calculation.html
	http://ghsi.de/CRC/

*/

#include "crc.h"

static u16 crc16_Poly(u16 poly);
static u16 crc16_Right(void* data, int length, u16 poly, u16 init);
static u16 crc16_Left(void* data, int length, u16 poly, u16 init);

//cria espelho do polinomio
static u16 crc16_Poly (u16 poly) {
    u16 i, r = 0;
    for (i=0;i<16;i++) {
        r = r << 1;
        r = r | (poly&1);
        poly = poly >> 1;
    }
    return r;
}

//16bits CRC right shifted
//data = buffer de bytes a serem calculados
//length = quantidade de bytes
//poly = polinomio original
//init = valor inicial do crc
static u16 crc16_Right(void* data, int length, u16 poly, u16 init) {
	u16 i, crc = init;
	u8 * p = (u8*)data;
	poly = crc16_Poly(poly);
	while (length--) {
		crc = crc ^ (*p++);
		for (i = 0; i < 8; i++)
			crc = (crc & 0x0001) ? (crc >> 1) ^ poly : (crc >> 1);
	}
	return crc;
}

//16bits CRC left shifted
//data = buffer de bytes a serem calculados
//length = quantidade de bytes
//poly = polinomio original
//init = valor inicial do crc
static u16 crc16_Left(void* data, int length, u16 poly, u16 init) {
	u16 i, crc = init;
	u8 * p = (u8*)data;
	while (length--) {
		crc = crc ^ (*p++<<8);
		for (i = 0; i < 8; i++)
			crc = (crc & 0x8000) ? (crc << 1) ^ poly : (crc << 1);
	}
	return crc;
}

//---------------------------------------------------------------------
// CRC: aplicações práticas
//---------------------------------------------------------------------

//poly = x16 + x15 + x2 + x0 = 0x8005
u16 crc16_NBR14522(void * data, int length) {
	return crc16_Right( data, length, 0x8005, 0x0000);
}

//poly = x16 + x15 + x2 + x0 = 0x8005
u16 crc16_MODBUS(void * data, int length) {
	return crc16_Right( data, length, 0x8005, 0xFFFF );
}

//poly = x16 + x12 + x5 + x0 = 0x1021
u16 crc16_XMODEM(void * data, int length) {
	return crc16_Left( data, length, 0x1021, 0x0000 );
}

//poly = x16 + x13 + x12 + x11 + x10 + x8 + x6 + x5 + x2 + x0  = 0x3D65
u16 crc16_DNP3(void * data, int length) {
	//calcula e depois aplica o complemento
    u16 r = ~crc16_Right( data, length, 0x3D65, 0x0000 );
    //troca os bytes
    return (r>>8)|(r<<8);
}

//texe(33F): 350us/100bytes [data 18.01.12]
//texe(32MX): 137us/100bytes [data 24.01.12]
//X^7+X^3+X^0 = 0x09
u8 crc7(void * data, int length) {
	u8 i, c;
	u8 crc;
	u8 * p = (u8*)data;
	crc = 0x00;
	while (length--) {
		c = *p++;
		for (i = 0; i < 8; i++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x80) crc = crc ^ 0x09;
			c = c << 1;
		}
		crc = crc & 0x7F;
	}
	return crc * 2 + 1;
}


//padrão de checksum de arquivos tipo "intel hex"
u8 crc8_HEX(void * data, int length) {
	u8 r = 0;
	u8 * p = (u8*)data;
	while (length--) r += *p++;
	return -r;
}

