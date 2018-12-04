/*
 * RTCControl.c
 *
 * Created: 10/7/2018 9:26:02 PM
 *  Author: BigJetPlane
 */ 

#include "sam.h"
#include <stdio.h>

void initI2C(){
	 /* port mux configuration */
	 PORT->Group[0].PINCFG[PIN_PA22].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN; /* SDA */
	 PORT->Group[0].PINCFG[PIN_PA23].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN; /* SCL */
	 
	 /* PMUX: even = n/2, odd: (n-1)/2 */
	 PORT->Group[0].PMUX[11].reg |= 0x02u;
	 PORT->Group[0].PMUX[11].reg |= 0x20u;
	 
	 /* APBCMASK */
	 PM->APBCMASK.reg |= PM_APBCMASK_SERCOM3;

	 /*GCLK configuration for sercom3 module*/
	 GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID (SERCOM3_GCLK_ID_CORE) |
	 GCLK_CLKCTRL_ID (SERCOM3_GCLK_ID_SLOW) |
	 GCLK_CLKCTRL_GEN(4) |
	 GCLK_CLKCTRL_CLKEN;
	 GCLK->GENCTRL.reg |= GCLK_GENCTRL_SRC_OSC8M |
	 GCLK_GENCTRL_GENEN|
	 GCLK_GENCTRL_ID(4);

	 /* set configuration for SERCOM3 I2C module */
	 SERCOM3->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN; /* smart mode enable */
	 while (SERCOM3->I2CM.SYNCBUSY.reg); /* waiting loading */
	 /* calculate BAUDRATE */
	 uint64_t tmp_baud =((8000000/100000)-10-(8000000*250 /1000000000))/2;
	 SERCOM3->I2CM.BAUD.bit.BAUD = SERCOM_I2CM_BAUD_BAUD((uint32_t)tmp_baud);
	 while (SERCOM3->I2CM.SYNCBUSY.reg); // waiting loading
	 /* value equals 0x22 or decimal 34 */
	 
	 SERCOM3->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_ENABLE   |/* enable module */
	 SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |		/* i2c master mode */
	 SERCOM_I2CM_CTRLA_SDAHOLD(3);		 /* SDA hold time to 600ns */
	 while (SERCOM3->I2CM.SYNCBUSY.reg);  /* waiting loading */

	 SERCOM3->I2CM.STATUS.reg |= SERCOM_I2CM_STATUS_BUSSTATE(1); /*set idle */
	 while (SERCOM3->I2CM.SYNCBUSY.reg);
	
	
	
}



void sendI2CDataArray(int SlAddr,uint8_t **ptrData, int Size,uint8_t RTCPtr){
	/*writes the data located in the array to the RTC
	SlAddr is the slave address, ptrData is a pointer to the 8-bit data array and
	Size is the number of elements in the array */
	volatile int i;
	 /* Acknowledge section is set as ACK signal by writing 0 in ACKACT bit */
	 SERCOM3->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	 while(SERCOM3->I2CM.SYNCBUSY.reg);  // waiting loading
	 
	 /* slave address with write signal (0) */
	 SERCOM3->I2CM.ADDR.reg = (SlAddr << 1) | 0;
	 while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){}; /* MB=1 if slave NACKS address */
	SERCOM3->I2CM.DATA.reg = RTCPtr;
	while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){};

	for(i=0; i< Size; i++) {
		/* placing the data from transmitting buffer to DATA register*/
		SERCOM3->I2CM.DATA.reg = *ptrData[i];
		while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){};/*MB=1 if slave NACKS address */
	}
	 
	 /* After transferring the last byte stop condition will be sent */
	 SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;
	
	
}


void SendI2CData(int SlAddr,uint8_t Data){
//SlAddr is the slave address, Data is the 8-bit data to send.

	SERCOM3->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	while(SERCOM3->I2CM.SYNCBUSY.reg);  // waiting loading
	
	/* slave address with write signal (0) */
	SERCOM3->I2CM.ADDR.reg = (SlAddr << 1) | 0;
	while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){};
		
	// data
	SERCOM3->I2CM.DATA.reg = Data;
	while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){};
	
	/* After transferring the last byte stop condition will be sent */
	


	}
	
void StopCond() {
//Function to generate a stop condition.

SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;
}




void receiveI2CDataArray(int SlAddr, uint8_t **ptrData ,int Size){
	
	 SERCOM3->I2CM.ADDR.reg = (SlAddr << 1) | 1;
	 while(SERCOM3->I2CM.INTFLAG.bit.SB==0){};

	 /* Acknowledge section is set as ACK signal by writing 0 in ACKACT bit */
	 SERCOM3->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;

	 for(int i=0; i< Size-1; i++) {
		 *ptrData[i] = SERCOM3->I2CM.DATA.reg;
		 while(SERCOM3->I2CM.INTFLAG.bit.SB==0){};
	 }
	 /* NACK should be send before reading the last byte*/
	 SERCOM3->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
	 SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;
	 *(ptrData[Size-1]) = SERCOM3->I2CM.DATA.reg;
	 
	
	
}

void ptrReloc(int Address,int slvAdd){
	
	 SERCOM3->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	 while(SERCOM3->I2CM.SYNCBUSY.reg); // waiting load
	 /* slave address with write signal (0) */
	 SERCOM3->I2CM.ADDR.reg = (slvAdd << 1) | 0; /*Send slave addr write */
	 while(SERCOM3->I2CM.INTFLAG.bit.MB ==0); /* MB=1 if slave NACKS address */
	 SERCOM3->I2CM.DATA.reg = Address; /* Send address for internal pointer */
	 while(SERCOM3->I2CM.INTFLAG.bit.MB ==0){};/* MB=1 if slave NACKS the addr */
	 SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;     /* Sending stop condition */
	 
}