#include "stm32f10x.h" 
#include <string>
#include <stdlib.h>
#include <ctype.h>

#define RX_BUFFER_SIZE (u8)(0xFF)

char RX_BUFFER[RX_BUFFER_SIZE]; //global buffer variable

volatile u16 PULSE_PERIOD = 1000; 
volatile u16 PULSE_WIDTH  = 0;
volatile u8  PASSWORD     = 0;

class usart2;

extern "C" void USART1_IRQHandler(void);
void cmd(char* buffer);
	
class usart2 {
	public:	
		void begin(uint32_t baudrate);
		
	  void println(char* str);
	  void print(char* str);
	  
	  void println(int var);
	  void print(int var);
};

void usart2::begin(uint32_t baudrate) {
	
	uint32_t _BRR = 8000000 / (baudrate);
	
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; 
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	
	GPIOA->CRH &=~ (GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
	GPIOA->CRH |=  (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_1); //TX
	
	GPIOA->CRH &=~ (GPIO_CRH_CNF10 | GPIO_CRH_MODE10); 
	GPIOA->CRH |=   GPIO_CRH_CNF10_0; // RX
	
	USART1->BRR = _BRR;
	USART1->CR1 = 0x0;
	USART1->CR2 = 0x0;
	
	USART1->CR1 |= USART_CR1_TE;
	USART1->CR1 |= USART_CR1_RE;
	USART1->CR1 |= USART_CR1_UE;
	
	USART1->CR1 |= USART_CR1_RXNEIE;
	NVIC_EnableIRQ(USART1_IRQn);
}

void usart2::print(char* str) {
	
	for(uint16_t i=0; i < std::strlen(str); i++) {
		while(!(USART1->SR & USART_SR_TC));
		USART1->DR = str[i];
	}
}

void usart2::println(char* str) {
	
	for(uint16_t i=0; i < std::strlen(str); i++) {
		while(!(USART1->SR & USART_SR_TC));
		USART1->DR = str[i];
	}
	
	while(!(USART1->SR & USART_SR_TC));
	USART1->DR = '\n';
}

void usart2::print(int var){
 
	char* str;
  
	std::sprintf(str, "%d", var);
	
	usart2 usart;
	
	usart.print(str);
}

void usart2::println(int var){
 
	char *str;
  
	std::sprintf(str, "%d", var);
	
  usart2 usart;
	
	usart.println(str);
}

void USART1_IRQHandler(void) {
	
	if ((USART1->SR & USART_SR_RXNE) != 0) { 
	  
		u8 tmp = USART1->DR;
		
		if(tmp == 0x0D) {
			cmd(RX_BUFFER);
			std::memset(RX_BUFFER, 0,RX_BUFFER_SIZE);
		  return;
	  }
			 
		RX_BUFFER[std::strlen(RX_BUFFER)] = tmp;
	}
}

char* uppercase(char* str) {
	
	char* upper;
	
	for(u16 i=0; i < std::strlen(str); i++) {
		upper[i] = std::toupper(str[i]); 
	}
	
	return upper;
}

void cmd(char* command) {
	
	if (!std::strlen(command))
		return;
	
	char* value1 = std::strtok(command, "-");
	char* value2 = std::strtok(NULL, "-");
	
	char* type = uppercase(value1);
	int number = std::atoi(value2); 
	
	
	usart2 usart;
	
	if(!(!std::strcmp("INFO?", type)  +
		   !std::strcmp("PASS", type)   +
		   !std::strcmp("PERIOD", type) +
		   !std::strcmp("WIDTH", type))) {
			 
		usart.print("unknown command: ");
		usart.println(type);
				 
		return;
	}
		
	if (!std::strcmp("INFO?", type)) {
		usart.print("pulse period: "); usart.println(PULSE_PERIOD);
		usart.print("pulse width: "); usart.println(PULSE_WIDTH);
		usart.print("block: "); usart.println(PASSWORD ? "no" : "yes");
		return;		
	}

	if (!std::strcmp("PASS", type)) {
		if (number == 5467){
			PASSWORD ^= 1;
			usart.println("ok");
		} else {
			usart.println("invalid password");
		}
	}

	if (!PASSWORD) {
		usart.println("you are not authorized\n");
		return;
	}

	if (!std::strcmp("PERIOD", type)) {
		PULSE_PERIOD = number;
		
		usart.print("current pulse period: ");
		usart.println(PULSE_PERIOD);
	}

	if (!std::strcmp("WIDTH", type)) {
		if (number > PULSE_PERIOD){
			usart.println("error: pulse width > pulse period");
		} else {
			PULSE_WIDTH  = number;
		}
		usart.print("current pulse width: ");
		usart.println(PULSE_WIDTH);
	}
}

int main(){

	usart2 usart;
	
	usart.begin(9600);
		
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
 	
	GPIOA->CRH &=~ (GPIO_CRH_CNF8 | GPIO_CRH_MODE8);
	GPIOA->CRH |=   GPIO_CRH_MODE8;
	
	GPIOA->CRL &=~ (GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
	GPIOA->CRL |=   GPIO_CRL_MODE0;
	
	while(1) {
		
			u32 high = PULSE_WIDTH;
		  u32 low  = PULSE_PERIOD - PULSE_WIDTH;
			
		  if (!high) {
				GPIOA->BSRR = GPIO_BSRR_BR8;
				GPIOA->BSRR = GPIO_BSRR_BR0;
			} else {
				GPIOA->BSRR = GPIO_BSRR_BS8;
				GPIOA->BSRR = GPIO_BSRR_BS0;
				for(uint32_t i=0; i < high; i++);

				GPIOA->BSRR = GPIO_BSRR_BR8;
				GPIOA->BSRR = GPIO_BSRR_BR0;
				for(uint32_t i=0; i < low; i++);
			}
	}
}


