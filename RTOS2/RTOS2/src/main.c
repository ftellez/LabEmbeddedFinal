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
    SystemInit();                     //Initicializacion del sistema
		SYSCTRL->OSC8M.bit.PRESC = 0;		
	initUART();							//Inicializacion de la comunicacion UART por el port  0 (my print)
	UART_Initialize(Baud);				//Inicializacion de la comunicacion UART por el port 5 (usb)
	initI2C();							// Inicializacion del protocolo i2c
	ClockInit();						// Inicicalizacion del los registros del RTC

	
	xTaskCreate(escrtTiempo, (signed char *) "RTCpull", 1024, NULL, 1, NULL ); //Task que implementa la lectura de los registros del RTC
	xTaskCreate(UART_SendChar, (signed char *) "Timestampsend", 1024, NULL, 2, NULL );// Task que implementa el print a travez de los pines 0 y 1
	xTaskCreate(printStamp, (signed char *) "Serialprint", 1024, NULL, 2, NULL ); // Task que implementa el print a travez del usb

	vTaskStartScheduler();

    
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

	while(1){
	ptrReloc(SEC_ADDR, SLAVE_ADDR);  //Relocalizacion del apuntador de los registros
	receiveI2CDataArray(SLAVE_ADDR,rx_buf,BUF_SIZE); //Lectura del modulo RTC
	
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



void UART_SendChar(void) {

	static char TxBuff[18] ;
	while(1){
		
		TxBuff[0] = SEG.field.un_sec + '0';
		TxBuff[1] = SEG.field.dec_sec + '0';o
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