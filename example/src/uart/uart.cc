#include "uart.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

// Criado em 2015/04/27
// Atualizado em 2015/04/29
// para mais informações consultar
//	http://www.cmrr.umn.edu/~strupp/serial.html
//	http://man7.org/linux/man-pages/man4/tty_ioctl.4.html

static int uartFD;									// Id da porta UART
static struct termios uartAttrOld, uartAttr;       	// Vars para atributos da porta UART
#define lenBUFFER 8*1024
static u8 rxbuf[lenBUFFER];
static int rxcnt,rxpos;

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Abre a porta UART e salva as configurações atuais antes de ser usada por essa lib
// Parametros:	port: Qual porta UART deve ser aberta. Ex: 'dev/ttyS1'
//				baudrate: 	B1200, B2400, B4800, B9600, B19200, B38400,
//							B57600, B115200, B230400, B460800, B500000, B576000,
//							B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
// Retorna:		pdPASS se a porta foi aberta com sucesso
//				pdFAIL se houve erro ao abrir a porta, isto pode ser:
//					- Porta esteja sendo usada por outro programa
//					- O usuário não tem permissão para usa essa porta: chmod a+rw /dev/ttyS1
//					- Não existe este nó de porta UART
// -------------------------------------------------------------------------------------------------------------------
int uart_Init(const char* port, uint baudrate) {
	// AJUSTAR A PORTA
	
	// OPEN THE UART
	// The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY = O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.

	uartFD = open(port, O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uartFD == -1) return pdFAIL;

	// CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)

	tcgetattr(uartFD, &uartAttrOld); // save current port settings
	tcgetattr(uartFD, &uartAttr); 	// capturar atributos da uart para nossas configurações

	//uartAttr.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
	uartAttr.c_cflag = CS8 | CLOCAL | CREAD;
	cfsetospeed(&uartAttr, baudrate);
    cfsetispeed(&uartAttr, baudrate);

	uartAttr.c_iflag = IGNPAR; //| ICRNL;
	uartAttr.c_oflag = 0;
	uartAttr.c_lflag = 0;
	tcflush(uartFD, TCIFLUSH); // limpa buffer de recepção sem ser lido
	tcsetattr(uartFD, TCSANOW, &uartAttr);
	
	rxcnt = 0;
	rxpos = 0;

	return pdPASS;
}

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Fecha a porta UART e restaura as configurações anteriores antes de ser usada por essa lib
// Parametros:	Nenhum
// Retorna:		Nada
// -------------------------------------------------------------------------------------------------------------------
void uart_Close(void) {
	tcsetattr(uartFD, TCSANOW, &uartAttrOld);
	close(uartFD);
}

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Envia uma string para FIFO de transmissão da UART. 
//				OBS1: Não usar strings longas que ultrapassam o tamanho da FIFO
//				OBS2: A string deve estar finalizada com o caractere nulo '\0' ou NULL
// Parametros:	buf: Ponteiro do buffer
//				size: Quantidade de dados do buffer que será enviado a FIFO
// Retorna:		Retorna o código da operação. Se for valor negativo houve erro na escrita da FIFO da UART
// -------------------------------------------------------------------------------------------------------------------
int uart_SendString(const char* buf) {
	return write(uartFD, buf, strlen(buf));		//Filestream, bytes to write, number of bytes to write
}

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Envia um buffer para FIFO de transmissão da UART. 
//				OBS: Não usar buffer longos que ultrapassam o tamanho da FIFO
// Parametros:	buf: Ponteiro do buffer
//				size: Quantidade de dados do buffer que será enviado a FIFO
// Retorna:		Retorna o código da operação. Se for valor negativo houve erro na escrita da FIFO da UART
// -------------------------------------------------------------------------------------------------------------------
//#include "stdio_uc.h"
//#include <stdio.h>
int uart_SendBuffer(u8* buf, u16 size) {
//	printf("UART: TX: ");
//	int x; for (x=0;x<size;x++) printf("0x%x ", buf[x]);
//	printf(CMD_TERMINATOR);
	return write(uartFD, buf, size);		//Filestream, bytes to write, number of bytes to write
}

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Envia um byte para FIFO de transmissão da UART
// Parametros:	ch: byte a ser enviado
// Retorna:		Retorna o código da operação. Se for valor negativo houve erro na escrita da FIFO da UART
// -------------------------------------------------------------------------------------------------------------------
int uart_PutChar(n16 ch) {
	return write(uartFD, &ch, 1);		//Filestream, bytes to write, number of bytes to write
}

// -------------------------------------------------------------------------------------------------------------------
// Descrição: 	Retorna com um byte da FIFO de recepção da UART
// Parametros:	ch: Ponteiro para a variável char par retorno do byte da FIFO
// Retorna:		pdPASS se um byte foi lido da FIFO
//				pdFAIL se não bytes na FIFO da UART ou na leitura da FIFO da UART
// -------------------------------------------------------------------------------------------------------------------
//int uart_GetChar(u8* ch) {
//	int ret = read(uartFD, (void *)ch, 1);
//	if (ret <= 0) return pdFAIL;
//	return pdPASS;
//}

// -----------------------------------------------------------------------------
// LÊ UM CARACTERE DO BUFFER DE RECEPÇÃO
// Parametro: byte com o retorno do caractere
// Retorna TRUE se o caractere foi recuperado, se retornar FALSE o buffer
//              está vazio ou  houve algum erro de leitura
// -----------------------------------------------------------------------------
int uart_GetChar(u8* ch) {
    // SE O BUFFER RX ESTIVER VAZIO VAMOS CARREGÁ-LO DA FIFO DE RECEPÇÃO DA UART
	if (rxcnt <= 0) { // Checa se o buffer RX está vazio
		rxpos = 0;
		rxcnt = read(uartFD, rxbuf, lenBUFFER);
		if (rxcnt <= 0) return pdFAIL; // erro de leitura? cancela...
	}

	// recupera o dado do buffer RX
	*ch = rxbuf[rxpos];
	rxpos++;
	rxcnt--;
	return pdPASS;
}


// -----------------------------------------------------------------------------------------------------------------
// Descrição: 	Limpa o buffer de recepção
// Parametros:	Nenhum
// Retorna:		Nada
// -----------------------------------------------------------------------------------------------------------------
void uart_ClearBufferRx(void) {
	tcflush(uartFD, TCIFLUSH);
	rxcnt = 0;
	// tcflush() discards data written to the object referred to by fd but not transmitted, or data received but not read, depending on the value of queue_selector:
	//		TCIFLUSH: flushes data received but not read.
	//		TCOFLUSH: flushes data written but not transmitted.
	//		TCIOFLUSH: flushes both data received but not read, and data written but not transmitted.
	// tcflow() suspends transmission or reception of data on the object referred to by fd, depending on the value of action: TCOOFF
}

// -----------------------------------------------------------------------------------------------------------------
// Descrição: 	Limpa o buffer de recepção
// Parametros:	Nenhum
// Retorna:		Nada
// -----------------------------------------------------------------------------------------------------------------
void uart_ClearBufferTx(void) {
	tcflush(uartFD, TCOFLUSH);
	rxcnt = 0;
	// tcflush() discards data written to the object referred to by fd but not transmitted, or data received but not read, depending on the value of queue_selector:
	//		TCIFLUSH: flushes data received but not read.
	//		TCOFLUSH: flushes data written but not transmitted.
	//		TCIOFLUSH: flushes both data received but not read, and data written but not transmitted.
	// tcflow() suspends transmission or reception of data on the object referred to by fd, depending on the value of action: TCOOFF
}

// -----------------------------------------------------------------------------------------------------------------
// Descrição: 	Retorna com a quantidade de bytes recebidos no buffer de recepção
// Parametros:	Nenhum
// -----------------------------------------------------------------------------------------------------------------
int uart_BufferQtdRx(void) {
	int nBytes;
	ioctl(uartFD, FIONREAD, &nBytes);
	return nBytes;
}


// -----------------------------------------------------------------------------------------------------------------
// Descrição: 	Retorna com a quantidade de bytes ainda no buffer de transmissão que estão sendo transmitidos
// Parametros:	Nenhum
// -----------------------------------------------------------------------------------------------------------------
// NÃO ESTÁ FUNCIONANDO CORRETAMENTE
//int uart_BufferQtdTx(void) {
//	int nBytes = 0;
//	int rc;
//	rc = ioctl(uartFD, TIOCOUTQ, &nBytes);
//	return rc;
//}


//int uart_GetBuffer (char* buf, int countRX) {

	// Read up to 255 characters from the port if they are there
//		unsigned char rx_buffer[256];
//		int rx_length = read(uartFD, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
//		if (rx_length > 0) {
//			//Bytes received
//			rx_buffer[rx_length] = '\0';
//			printf("%i bytes read : %s\n", rx_length, rx_buffer);
//		}
//}
