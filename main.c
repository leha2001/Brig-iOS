#include "math.h"
#include "main.h"
//#include "syscalls.h"
//#include "stdlib.h"
//#include <string.h>

uint32_t clock_frequency_measure();

const uint32_t sys_time = 1533128400;
uint32_t alarm;
uint8_t transmit_buf[256];
uint8_t transmit_queue_index=0;
uint8_t receive_buf[256];

//----- Byte staffing protocol------------------
#define END 0
#define END_change 0xFD
#define END_change_change 0xFC
void add_END_to_transmit();
void add_to_transmit(uint8_t num);
void add_to_transmit_uint16(uint16_t num);
void add_to_transmit_uint32(uint32_t num);
void add_to_transmit_str(uint8_t *str);

void cmd_perform(char *str);

uint32_t recent_time, recent_alarm;

int main()
{
    RCC->APB1ENR |= RCC_APB1Periph_BKP | RCC_APB1Periph_PWR;
	PWR->CR |= PWR_CR_DBP; //Unblock RTC & BKP register for write access
	RCC->BDCR |= RCC_BDCR_RTCSEL_LSE | RCC_BDCR_RTCEN | RCC_BDCR_LSEON; //
	while(!(RCC->BDCR & RCC_BDCR_LSERDY)); //wait for LSE stable
	RTC->CRH = RTC_CRH_ALRIE;   // Enable "Alarm interrupt" for RTC peripherial
	//RTC->CRH |= RTC_CRH_SECIE;   // Enable "Second interrupt" for RTC peripherial (WakeUP vector (#3))
	//INTERRUPT_ENABLE(3); // Second interrupt come to WakeUp vector (3)
	while(!(RTC->CRL & RTC_CRL_RTOFF)); // wait for write are terminated
//	RTC->CRL |= RTC_CRL_CNF;     // unblock write access for PRL, CNT, DIV  register of RTC
//	RTC->PRLL = 0x7FFF; //RTC preloader for 1 second period increment Timer
//	RTC->CNTH = sys_time>>16;
//	RTC->CNTL = sys_time;
//	RTC->ALRH = *((uint16_t*)&alarm+1);//RTC->ALRH = alarm>>16;
//	RTC->ALRL = *(uint16_t*)&alarm;    //RTC->ALRL = alarm;
//	RTC->CRL &= ~RTC_CRL_CNF;//  for write protect PRL, CNT, DIV
	INTERRUPT_ENABLE(41);//Enable ALARM RTC interrupt in NVIC
	EXTI->IMR |= 1<<17; // RTC_ALARM interrupt enable
	EXTI->RTSR|= 1<<17; // RTC_ALARM rising edge
//	String for parser test:
//	cron_add_tab("0 15-20,30,40 1-15/3,*/20,30-35 * * * 200015e1,20000002,20000003,20000004 20000005,20000006 V1,2\0");
//	Tables for tests:
//	cron_add_tab("0,10,20,30,40,50 * * * * * 40011010 V100\0");
//	cron_add_tab("5,15,25,35,45,55 * * * * * 40011014 V100\0");
//	cron_add_tab("2,12,22,32,42,52 * * * * * 40011010 V200\0");
//	cron_add_tab("3,13,23,33,43,53 * * * * * 40011014 V200\0");
//	cron_add_tab("*/6 * * * * * 40013804 V35\0");
//	cron_add_tab("1-60/6 * * * * * 40013804 V36\0");
//	cron_add_tab("2-60/6 * * * * * 40013804 V37\0");
//	cron_add_tab("3-60/6 * * * * * 40013804 V38\0");
//	cron_add_tab("4-60/6 * * * * * 40013804 V39\0");
//	cron_add_tab("5-60/6 * * * * * 40013804 V40\0");
//	cron_add_tab("* * * * * * 40013804 4000281c\0");
//	Tables for greenhouse: 
	//cron_add_tab("0 0 5,7-19,21 * * * 40011010 V100\0");
	crontab[0] = "0 0 5,7-19,21 * * * D40011010 V100";
	crontab[1] = "0 1 5,7-19,21 * * * D40011014 V100";
	crontab[2] = "0 30 10-16 * * * D40011010 V100";
	crontab[3] = "0 31 10-16 * * * D40011014 V100";
	crontab[4] = "1 1 5,7-19,21 * * * D40011010 V200";
	crontab[5] = "1 2 5,7-19,21 * * * D40011014 V200";
	crontab[6] = "1 31 10-16 * * * D40011010 V200";
	crontab[7] = "1 32 10-16 * * * D40011014 V200";
	crontab[8] = "*/15 * * * * * D40003000 VAAAA";
//	crontab[9] = "0 * * * * * D40011014 V200";
//	crontab[10] = "0 * * * * * D40011014 V200";
//	crontab[11] = "0 * * * * * D40011014 V200";

	RCC->CSR |= RCC_CSR_LSION;
	while(!(RCC->CSR & RCC_CSR_LSIRDY));
	IWDG->KR = 0xCCCC;
	IWDG->KR = 0x5555;
	IWDG->PR = 0b111;
	//DBGMCU->CR |= DBGMCU_IWDG_STOP;

	RCC->APB2ENR |= RCC_APB2Periph_GPIOC;
	SETMASK(GPIOC->CRH, GPIO_CRH_CNF8|GPIO_CRH_MODE8, 0b0001); 
	SETMASK(GPIOC->CRH, GPIO_CRH_CNF9|GPIO_CRH_MODE9, 0b0001); 
	RCC->APB2ENR |= RCC_APB2Periph_GPIOB;
	SETMASK(GPIOB->CRL, GPIO_CRL_CNF0|GPIO_CRL_MODE0, 0b0001); 
	SETMASK(GPIOB->CRL, GPIO_CRL_CNF1|GPIO_CRL_MODE1, 0b0001); 
//	SETMASK(GPIOB->CRL, GPIO_CRL_CNF2|GPIO_CRL_MODE2, 0b0001); 
//	SETMASK(GPIOB->CRL, GPIO_CRL_CNF3|GPIO_CRL_MODE3, 0b0001); 
//	SETMASK(GPIOB->CRL, GPIO_CRL_CNF4|GPIO_CRL_MODE4, 0b0001); 
	SETMASK(GPIOB->CRL, GPIO_CRL_CNF5 |GPIO_CRL_MODE5 , 0b0001); 
	SETMASK(GPIOB->CRL, GPIO_CRL_CNF6 |GPIO_CRL_MODE6 , 0b0001); 
	SETMASK(GPIOB->CRL, GPIO_CRL_CNF7 |GPIO_CRL_MODE7 , 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF8 |GPIO_CRH_MODE8 , 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF9 |GPIO_CRH_MODE9 , 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF10|GPIO_CRH_MODE10, 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF11|GPIO_CRH_MODE11, 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF12|GPIO_CRH_MODE12, 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF13|GPIO_CRH_MODE13, 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF15|GPIO_CRH_MODE15, 0b0001); 
	SETMASK(GPIOB->CRH, GPIO_CRH_CNF15|GPIO_CRH_MODE15, 0b0001); 
	GPIOB->BSRR = 0b1;
	GPIOB->BRR = 0b1111100010;
	if (GPIOC->ODR & (1<<9))
		sbi(GPIOB->BSRR,13);
	if (GPIOC->ODR & (1<<8))
		sbi(GPIOB->BSRR,12);
	if (RCC->CSR & (RCC_CSR_PINRSTF))
		sbi(GPIOB->BSRR,1);
	if (RCC->CSR & (RCC_CSR_PORRSTF))
		sbi(GPIOB->BSRR,5);
	if (RCC->CSR & (RCC_CSR_SFTRSTF))
		sbi(GPIOB->BSRR,6);
	if (RCC->CSR & (RCC_CSR_IWDGRSTF))
		sbi(GPIOB->BSRR,7);
	if (RCC->CSR & (RCC_CSR_WWDGRSTF))
		sbi(GPIOB->BSRR,8);
	if (RCC->CSR & (RCC_CSR_LPWRRSTF))
		sbi(GPIOB->BSRR,9);
	
sys_clock=clock_frequency_measure();
//---------UART-----------------------
    RCC->APB2ENR |= RCC_APB2Periph_USART1;//Включение тактовой USART (на APB1 шине висит)
#define baudrate 115200
	uint16_t ratio = sys_clock/baudrate;
	if (ratio<16) {
		USART1->CR1 |= USART_CR1_OVER8;
		USART1->BRR = ((ratio<<1) & (~0b1111)) | (ratio & 0b111); // (ratio/8<<4) | (ratio%8)
	}
	else {
		USART1->CR1 &= ~USART_CR1_OVER8;
		USART1->BRR = ratio;
	}
	USART1->CR1 |= USART_CR1_UE	| USART_CR1_TE | USART_CR1_RE;
	RCC->APB2ENR |= RCC_APB2Periph_GPIOA;
	SETMASK(GPIOA->CRH, GPIO_CRH_CNF10|GPIO_CRH_MODE10, 0b0100); 
	SETMASK(GPIOA->CRH, GPIO_CRH_CNF9|GPIO_CRH_MODE9, 0b1010); 
	//INTERRUPT_ENABLE(37);

//-----------DMA1---------------------------
	//---- DMA for transmit buffer-----------------
//	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
//	DMA1_Channel4->CPAR = (uint32_t)&USART1->DR;
//	DMA1_Channel4->CMAR = (uint32_t)transmit_buf;
//	DMA1_Channel4->CNDTR = sizeof(transmit_buf);
//	DMA1_Channel4->CCR = DMA_CCR4_MINC | DMA_CCR4_DIR;

	//---- DMA for recieve to buffer-----------------
//	DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;
//	DMA1_Channel5->CMAR = (uint32_t)recieve_buf;
//	DMA1_Channel5->CNDTR = sizeof(recieve_buf);
//	DMA1_Channel5->CCR = DMA_CCR4_MINC | DMA_CCR4_CIRC | DMA_CCR4_EN;
//	USART1->CR3 = USART_CR3_DMAT | USART_CR3_DMAR; 
//	USART1->SR = 0;

	set_alarm(next_alarm());

	uint8_t transmited=0;
	uint8_t received=0;
	while(1){
		if (transmit_queue_index!=transmited)
			if (USART1->SR & USART_SR_TC){
				USART1->DR = transmit_buf[transmited];
				transmited++;
			}
		if (USART1->SR & USART_SR_RXNE){
			receive_buf[received] = USART1->DR;
			if (receive_buf[received] == END){
				cmd_perform((char *)receive_buf);
				received = 0;
			}
			else if ((receive_buf[received] == END_change) && (receive_buf[received-1] == END_change))
				receive_buf[received-1] = END;
			else if ((receive_buf[received] == END_change_change) && (receive_buf[received-1] == END_change))
				receive_buf[received-1] = END_change;
			else received++;
		}
		if (RTC->CNTH!=0){
			if (RTC->CNTL!=(uint16_t)recent_time){
				recent_time = RTC->CNTL + (RTC->CNTH<<16);
				recent_alarm= RTC->ALRL + (RTC->ALRH<<16);
				if (bit_is_set(GPIOB->ODR,0))
					sbi(GPIOB->BRR,0);
				else
					sbi(GPIOB->BSRR,0);
			}
		}
		else if (recent_time!=0){
			sbi(GPIOB->BSRR,11);
			RTC->CRL |= RTC_CRL_CNF;     // unblock write access for PRL, CNT, DIV  register of RTC
			RTC->CNTH = recent_time>>16;
			RTC->CNTL = (uint16_t)recent_time;
			RTC->ALRH = recent_alarm>>16;
			RTC->ALRL = (uint16_t)recent_alarm+1;
			RTC->CRL &= ~RTC_CRL_CNF;//  for write protect PRL, CNT, DIV
		}
	}

//-----------SPI---------------------------
//	RCC->APB2ENR |= RCC_APB2Periph_SPI1;  //Включение тактовой SPI
//	SPI1->CR2 = SPI_CR2_RXNEIE | SPI_CR2_SSOE;
//	SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SPE;
//	SETMASK(SPI1->CR1, SPI_CR1_BR, 0b10);
//	SETMASK(GPIOA->CRL, GPIO_CRL_CNF4|GPIO_CRL_MODE4, 0b0001); 
//	sbi(GPIOA->BSRR,4);
//	SETMASK(GPIOA->CRL, GPIO_CRL_CNF5|GPIO_CRL_MODE5, 0b1011); 
//	SETMASK(GPIOA->CRL, GPIO_CRL_CNF6|GPIO_CRL_MODE6, 0b0100); 
//	SETMASK(GPIOA->CRL, GPIO_CRL_CNF7|GPIO_CRL_MODE7, 0b1011); 
//	INTERRUPT_ENABLE(35);
//
//	RCC->APB2ENR |= RCC_APB2Periph_GPIOC;
//	SETMASK(GPIOC->CRH, GPIO_CRH_CNF8|GPIO_CRH_MODE8, 0b0001); 
//	SETMASK(GPIOC->CRH, GPIO_CRH_CNF9|GPIO_CRH_MODE9, 0b0001); 
	

/*	RCC->APB1ENR = RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3;
	TIM2->CR1 = TIM_CR1_CEN;
	TIM3->CR1 = TIM_CR1_CEN;
	SETMASK(TIM2->CR2, TIM_CR2_MMS, 0b010);
	SETMASK(TIM3->SMCR, TIM_SMCR_TS, 0b001);
	SETMASK(TIM3->SMCR, TIM_SMCR_SMS,0b111);

	RCC->APB2ENR |= RCC_APB2Periph_ADC1;
	SETMASK(ADC1->SMPR1,ADC_SMPR1_SMP16,0b110);// 111 in SMP16
	ADC1->SQR3 = 16;
	ADC1->CR2 = ADC_CR2_TSVREFE;
	ADC1->CR1 = ADC_CR1_EOCIE;

	RCC->APB1ENR = RCC_APB1Periph_TIM4;
	TIM4->PSC = 100; //100 - 0.8s timer period(65535) for 8 MHz
	TIM4->CR1 = TIM_CR1_OPM;
	TIM4->DIER = TIM_DIER_UIE;
		
	
	INTERRUPT_ENABLE(18);//Enable ADC interrupt in NVIC
	INTERRUPT_ENABLE(30);//Enable TIM4 interrupt
	//INTERRUPT_ENABLE(3);// Enable interrupt for RTC wakeup vector in NVIC
	SCB->SCR |= SCB_SCR_SEVONPEND;

    RCC->APB1ENR |= RCC_APB1Periph_USART2;//Включение тактовой USART (на APB1 шине висит)
    RCC->APB1ENR |= RCC_APB1Periph_SPI2;  //Включение тактовой SPI

*/}

void cmd_perform(char *str)
{
	if (!_strncmp(str,"RT",sizeof("ST")-1)){ //Read time(system) UTC
		add_to_transmit_str((uint8_t*)"TM:");
		add_to_transmit_uint16(RTC->CNTH);
		add_to_transmit_uint16(RTC->CNTL);
		add_END_to_transmit();
	}
	if (!_strncmp(str,"ST",sizeof("ST")-1)){ //Set time(system) UTC
		const uint8_t base = sizeof("ST")-1;
		while(!(RTC->CRL & RTC_CRL_RTOFF)); // wait for write are terminated
		RTC->CRL |= RTC_CRL_CNF;     // unblock write access for PRL, CNT, DIV  register of RTC
		RTC->CNTH = ((uint8_t)str[base]<<8) + (uint8_t)str[base+1];
		RTC->CNTL = ((uint8_t)str[base+2]<<8)+(uint8_t)str[base+3];
		RTC->CRL &= ~RTC_CRL_CNF;//  for write protect PRL, CNT, DIV
		add_to_transmit_str((uint8_t*)"OK");
		add_END_to_transmit();
		while(!(RTC->CRL & RTC_CRL_RTOFF)); // wait for write are terminated
		set_alarm(next_alarm());
		RCC->CSR |= 1<<24; //Clear all reset flags
		GPIOB->BRR=0xFFFF;
	}
}

void add_END_to_transmit()
{
	transmit_buf[transmit_queue_index++]=END;
}

void add_to_transmit(uint8_t num)
{
	switch (num) {
		case END:
			transmit_buf[transmit_queue_index++]=END_change;
			transmit_buf[transmit_queue_index++]=END_change;
			break;
		case END_change:
			transmit_buf[transmit_queue_index++]=END_change;
			transmit_buf[transmit_queue_index++]=END_change_change;
			break;
		default:
			transmit_buf[transmit_queue_index++]=num;
			break;
	}
}

void add_to_transmit_uint16(uint16_t num)
{
	add_to_transmit(num>>8);
	add_to_transmit((uint8_t)num);
}

void add_to_transmit_uint32(uint32_t num)
{
	for(uint8_t i=3;i<4;i--)
		add_to_transmit((uint8_t)(num>>(8*i)));
}

void add_to_transmit_str(uint8_t *str)
{
	while(*str){
		add_to_transmit(*str);
		str++;
	}
}

uint32_t clock_frequency_measure()
{
	RCC->APB1ENR |= RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3;
	TIM2->CR1 = TIM_CR1_CEN;
	TIM3->CR1 = TIM_CR1_CEN;
	SETMASK(TIM2->CR2, TIM_CR2_MMS, 0b010);
	SETMASK(TIM3->SMCR, TIM_SMCR_TS, 0b001);
	SETMASK(TIM3->SMCR, TIM_SMCR_SMS,0b111);

	RTC->CRL &= ~RTC_CRL_SECF;
	while (!(RTC->CRL & RTC_CRL_SECF));
	TIM2->CNT=0;
	TIM3->CNT = 0;
	RTC->CRL &= ~RTC_CRL_SECF;
	while (!(RTC->CRL & RTC_CRL_SECF));
	TIM2->CR1 =0;
	TIM3->CR1= 0;
	uint32_t frequency = (TIM3->CNT<<16)+TIM2->CNT;
	RCC->APB1ENR &= ~(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3);
	return frequency;
}
	