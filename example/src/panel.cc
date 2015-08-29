#include <nan.h>
#include "uart/uart.h"
#include "_config_cpu_.h"
#include "app.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <v8.h>
#include <string.h>
#include <errno.h>
#include <time.h>

using namespace v8;

static  pthread_t id_thread;

tControl control; // Variável compartilhada entre dois processos

/* Implementação dos métodos para o objeto que representa o experimento

@function int status Setup(void)
	@description Configuração e inicialização do protocolo de comunicação Serial + modbus; 
	@return Retorna 0 se houve algum erro na abertura da porta serial, ou 1 se a configuração foi realizada com sucesso;
	@params Nenhum

@function int status Run(void) 
	@description Inicialização da thread responsável pela comunicação com a placa de aquisição e controle;
	@return Retorna 0 se houve 
	@params Nenhum
	
@function int status Update(int digitalOut) 
	@description Setar saídas do experimento somente;
	@return Retorna 1 se alterações da saída foram realizadas com sucesso, -1 se as entradas estão fora da faixa de valores permitida ou 0 se houve algum erro na atribuição das saídas;
	@params Inteiro representando as saídas digitais (representação binária)

@function string jsonFormattedString GetValues(void): 
	@description Coletar Inicialização da thread responsável pela comunicação com a placa de aquisição e controle;
	@return Retorna string com dados formatados em json para ser entregue ao cliente seguindo API de definição de dados particular de cada  experimento;
	@params Nenhum

@function int status Exit(void)
	@description Inicialização da thread responsável pela comunicação com a placa de aquisição e controle;
	@return Retorna 0 se houve algum problema 
	@params Nenhum

*/


NAN_METHOD(Setup) {
     NanScope();
     
     if (modbus_Init() == pdFAIL) {
   		fprintf(flog, "Erro ao abrir a porta UART"CMD_TERMINATOR);
		fprintf(flog, "Verifique se essa porta não esteja sendo usada por outro programa,"CMD_TERMINATOR);
		fprintf(flog, "ou se o usuário tem permissão para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
		NanReturnValue(NanNew(0));
     }
      
    NanReturnValue(NanNew(1)); // outra maneira NanNew<Numer>(1)
}

NAN_METHOD(Update) {
	NanScope();
	if (args.Length() <= 0 ){
		NanThrowTypeError("Wrong number of arguments");
		NanReturnUndefined();
    }
		
    if(!args[0]->IsNumber()) {
		printf("args[0] is not a number");
		NanThrowTypeError("Wrong type of first argument");
		NanReturnUndefined();
    }
	
    int switches = args[0]->NumberValue(); 

	if(switches < 0 || switches > 256){
		 NanReturnValue(NanNew(-1));
	}
	control.relays= switches;	
    NanReturnValue(NanNew(1));	
} 

NAN_METHOD(GetValues) {
	NanScope();
	int i; 
	std::string buffer = "{";
	std::string buffer_amp = "", buffer_volt = "";
	char value[10];
	buffer_amp = std::string("\"amperemeter\":[");
	buffer_volt = std::string("\"voltmeter\":[");
	
	for(i =0; i < nMULTIMETER_GEREN ; i++){
		if(control.multimeter[i].sts)
				sprintf( value, "%i", control.multimeter[i].value);
		else	
				sprintf( value, "%i", 0);
			
		
		if(control.multimeter[i].func){
			if(i == 0)
				buffer_amp = buffer_amp + std::string(value) ;
			else
				buffer_amp = buffer_amp +  std::string(",") + std::string(value) ;
			
			
		}else{
			if(i == nMULTIMETER_GEREN-1)
				buffer_volt = buffer_volt +  std::string(value) ;
			else
				buffer_volt = buffer_volt +  std::string(value) + std::string(",");
			
			
		}				
	}
	buffer_amp = buffer_amp + std::string("]");
    buffer_volt = buffer_volt + std::string("]");
	
    NanReturnValue(NanNew("{" + buffer_amp + "," + buffer_volt + "}"));	
}


NAN_METHOD(Run) {
	NanScope();

    init_control_tad();
	
        int rthr = pthread_create(&id_thread, NULL, modbus_Process, (void *) 0); // fica funcionando até que receba um comando via WEB para sair		
		if(rthr){
			printf("Unable to create thread void * modbus_Process");
			NanReturnValue(NanNew(0));
		}
	
	NanReturnValue(NanNew(1));
}


NAN_METHOD(Exit) {
	NanScope();
    if (args.Length() <= 0 ){
		NanThrowTypeError("Wrong number of arguments");
		NanReturnUndefined();
    }
		
    if(!args[0]->IsNumber()) {
		NanThrowTypeError("Wrong type of first argument");
		NanReturnUndefined();
    }
    
	init_control_tad();
    int switches = args[0]->NumberValue(); 
	
	printf("Relays: %i"CMD_TERMINATOR,switches);

    if(switches < 0 || switches > 256){
		 NanReturnValue(NanNew("{'error':'invalid input'}"));
	}
	control.relays= switches;
	printf("Relays: %i"CMD_TERMINATOR,switches);
	sleep(0.5);
	control.exit =1;
       
	printf("Fechando programa"CMD_TERMINATOR);
	fclose(flog);

	uart_Close();
	NanReturnValue(NanNew("1"));
}


void Init(Handle<Object> exports) {
	exports->Set(NanNew("run"), NanNew<FunctionTemplate>(Run)->GetFunction());
	exports->Set(NanNew("setup"), NanNew<FunctionTemplate>(Setup)->GetFunction());
	exports->Set(NanNew("update"), NanNew<FunctionTemplate>(Update)->GetFunction());
	exports->Set(NanNew("exit"), NanNew<FunctionTemplate>(Exit)->GetFunction());
	exports->Set(NanNew("getvalues"), NanNew<FunctionTemplate>(GetValues)->GetFunction());
}

NODE_MODULE(panel, Init)

