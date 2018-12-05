/*
 * RTOS_Test2.c
 *
 * Created: 12/1/2018 4:50:57 PM
 * Author : BigJetPlane
 */ 


#include "sam.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "uart.h"
#include "myprintf.h"
#include "stdbool.h"
#include "RTCCONTROL.h"



#define LCD_PINCFG	0x50000FC8
#define LCD_PINMASK 0x00020400

#define PIN13L	0x00020000

#define SLAVE_ADDR 0x68u
#define BUF_SIZE 7

uint8_t SEGT, MINT,HOUT,DAYT,DATET,MOT,YEART;
uint8_t SEGR,MINR,HOUR,DAYR,DATER,MOR,YEARR;




sec_type SEG;
min_type MIN;
hour_type HOU;
day_type DAY;
date_type DATE;
month_type MON;
year_type YEAR;


uint8_t *tx_buf[BUF_SIZE]={&SEGT,&MINT,&HOUT,&DAYT,&DATET,&MOT,&YEART};
uint8_t *rx_buf[BUF_SIZE]={&SEGR,&MINR,&HOUR,&DAYR,&DATER,&MOR,&YEARR};
	
void ClockInit();
void printStamp();
void escrtTiempo();	
void initUARTRasp(void);
void UART_SendChar(void);


 const uint32_t Baud = 57600;

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
		SYSCTRL->OSC8M.bit.PRESC = 0;
		PORT->Group[0].WRCONFIG.reg = LCD_PINCFG;
	initUART();
	UART_Initialize(Baud);
	initI2C();
	ClockInit();	
	//myprintf("holahola");
	
	xTaskCreate(escrtTiempo, (signed char *) "RTCpull", 1024, NULL, 1, NULL );
	xTaskCreate(UART_SendChar, (signed char *) "Timestampsend", 1024, NULL, 2, NULL );
	xTaskCreate(printStamp, (signed char *) "Serialprint", 1024, NULL, 2, NULL );

	vTaskStartScheduler();

    /* Replace with your application code */
    
}

void ClockInit(){
	
	SEG.field.un_sec = 0x0;
	SEG.field.dec_sec= 0x0;
	MIN.field.un_min= 0x3;
	MIN.field.dec_min= 0x2;
	HOU.field.un_hour=0x4;
	HOU.field.dec_hour=0x0;
	HOU.field.am_pm= 0x1;
	HOU.field.h1224=0x1 ;
	DAY.field.day= 0x2;
	DATE.field.un_date= 0x6 ;
	DATE.field.dec_date= 0x1;
	MON.field.un_month= 0x0;
	MON.field.dec_mont= 0x1;
	YEAR.field.un_year= 0x8;
	YEAR.field.dec_year= 0x1;
	
	SEGT= SEG.reg;
	MINT= MIN.reg;
	HOUT= HOU.reg;
	DAYT= DAY.reg;
	DATET= DATE.reg;
	MOT= MON.reg;
	YEART= YEAR.reg;
	sendI2CDataArray(SLAVE_ADDR,tx_buf , BUF_SIZE,SEC_ADDR);
	StopCond();

}

void printStamp(){
	
	while(1){
	//Date/Month/Year/Hours/Minutes/Seconds
	myprintf("%d%d/%d%d/20%d%d/", DATE.field.dec_date,DATE.field.un_date,MON.field.dec_mont,MON.field.un_month,YEAR.field.dec_year,YEAR.field.un_year);
	if(HOU.field.h1224 == 1){
		if(HOU.field.am_pm==1){
			myprintf("%d%d:%d%d:%d%d PM\n",HOU.field.dec_hour,HOU.field.un_hour,MIN.field.dec_min,MIN.field.un_min,SEG.field.dec_sec,SEG.field.un_sec);
		}
		else{
			myprintf("%d%d:%d%d:%d%d AM\n",HOU.field.dec_hour,HOU.field.un_hour,MIN.field.dec_min,MIN.field.un_min,SEG.field.dec_sec,SEG.field.un_sec);
		}
	}
	else{
		
		myprintf("%d%d:%d%d:%d%d/\n",HOU.field.dec_hour|(HOU.field.am_pm<<1),HOU.field.un_hour,MIN.field.dec_min,MIN.field.un_min,SEG.field.dec_sec,SEG.field.un_sec);
	}
	vTaskDelay(1000);
	}
	
}
void escrtTiempo(){
	//uint8_t SEGR,MINR,HOUR,DAYR,DATER,MOR,YEARR;
	while(1){
	ptrReloc(SEC_ADDR, SLAVE_ADDR);
	receiveI2CDataArray(SLAVE_ADDR,rx_buf,BUF_SIZE);
	
	SEG.reg=SEGR;
	MIN.reg=MINR;
	HOU.reg=HOUR;
	DAY.reg=DAYR;
	DATE.reg=DATER;
	MON.reg=MOR;
	YEAR.reg=YEARR;
	vTaskDelay(250);
	}
	
}

void initUARTRasp(void) {
   PORT->Group[0].DIRSET.reg = (1 << 8);       // Set TX Pin direction to output
   PORT->Group[0].PINCFG[8].reg |= PORT_PINCFG_INEN;    // Set TX Pin config for input enable (required for usart)
   PORT->Group[0].PINCFG[8].reg |= PORT_PINCFG_PMUXEN;   // enable PMUX
   PORT->Group[0].PMUX[8>>1].bit.PMUXE = PORT_PMUX_PMUXE_D_Val; // Set the PMUX bit (if pin is even, PMUXE, if odd, PMUXO)
 
   PORT->Group[0].DIRCLR.reg = (1 << 9);       // Set RX Pin direction to input
   PORT->Group[0].PINCFG[9].reg |= PORT_PINCFG_INEN;    // Set RX Pin config for input enable
   PORT->Group[0].PINCFG[9].reg &= ~PORT_PINCFG_PULLEN;   // enable pullup/down resistor
   PORT->Group[0].PINCFG[9].reg |= PORT_PINCFG_PMUXEN;   // enable PMUX
   PORT->Group[0].PMUX[9>>1].bit.PMUXO = PORT_PMUX_PMUXE_D_Val; // Set the PMUX bit (if pin is even, PMUXE, if odd, PMUXO)
 
   PM->APBCMASK.reg |= PM_APBCMASK_SERCOM2;      // Set the PMUX for SERCOM3 and turn on module in PM

   // Generic clock “SERCOM3_GCLK_ID_CORE” uses GCLK Generator 0 as source (generic clock source can be
   // changed as per the user needs), so the SERCOM3 clock runs at 8MHz from OSC8M
   GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM2_GCLK_ID_CORE) | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);

   // By setting the DORD  bit LSB is transmitted first and setting the RXPO bit as 1
   // corresponding SERCOM PAD[1] will be used for data reception, PAD[0] will be used as TxD
   // pin by setting TXPO bit as 0, 16x over-sampling is selected by setting the SAMPR bit as
   // 0, Generic clock is enabled in all sleep modes by setting RUNSTDBY bit as 1,
   // USART clock mode is selected as USART with internal clock by setting MODE bit into 1.
   SERCOM2->USART.CTRLA.reg = SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK | SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(0/*PAD0*/);
 
   // 8-bits size is selected as character size by setting the bit CHSIZE as 0,
   // TXEN bit and RXEN bits are set to enable the Transmitter and receiver
   SERCOM2->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);
 
   /* synchronization busy */
   while(SERCOM2->USART.SYNCBUSY.bit.CTRLB);

   uint64_t br = (uint64_t)65536 * (8000000 - 16 * 9600) / 8000000; // Variable for baud rate

   // baud register value corresponds to the device communication baud rate
   SERCOM2->USART.BAUD.reg = (uint16_t)br;

   // SERCOM3 peripheral enabled
   SERCOM2->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

   /* synchronization busy */
   while(SERCOM2->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);
}

void UART_SendChar(void) {
	// Wait until buffer is available
	static char TxBuff[18] ;
	while(1){
		
		TxBuff[0] = SEG.field.un_sec + '0';
		TxBuff[1] = SEG.field.dec_sec + '0';
		TxBuff[2] = ':';
		TxBuff[3] = MIN.field.un_min + '0';
		TxBuff[4] = MIN.field.dec_min + '0';
		TxBuff[5] = ':';
		TxBuff[6] = HOU.field.un_hour + '0';
		TxBuff[7] = HOU.field.dec_hour + '0';
		TxBuff[8] = '__';
		TxBuff[9] = YEAR.field.un_year + '0';
		TxBuff[10] = YEAR.field.dec_year + '0'; 
		TxBuff[11] = '-';
		TxBuff[12] = MON.field.un_month + '0';
		TxBuff[13] = MON.field.dec_mont + '0';
		TxBuff[14] = '-';
		TxBuff[15] =  DAY.field.day + '0';
		TxBuff[16] = '\n';
		TxBuff[17] = '\r';
		
		for (int i = 17; i>=0 ; i--){	
			while (! ( SERCOM5->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE ) );
			// Send data
			SERCOM5->USART.DATA.reg = (char)TxBuff[i]; //TxBuff[i];
			myprintf("%d",i);
		}
		vTaskDelay(1000);
		
		
	}
}