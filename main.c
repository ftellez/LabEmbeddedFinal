#include "samd21.h"
#include "myprintf.h"

#define RXBUFSIZE 0x400
#define LENGTH_R1 0x03
#define LENGTH_R7 0x07

void initSPI(void);
void initUART(void);
void initUARTRasp(void);
uint8_t spiSend(uint8_t data);
uint8_t data,counter=0;

int main(void)
{
  /* Initialize the SAM system */
  SystemInit();
  /* Switch to 8MHz clock (disable prescaler) */
  SYSCTRL->OSC8M.bit.PRESC = 0;
  initUART();
  initUARTRasp();
  //initSPI();
  /*REG_PORT_OUTSET0 = PORT_PA17;
  PORT->Group[0].DIRSET.reg = PORT_PA17;
  REG_PORT_OUTCLR0 = PORT_PA07;*/
  //sendToRasp("Hola Rasp\n");
  //myprintf("YO SI SIRVO ");
  //spiSend(0xFF);
 //REG_PORT_OUTSET0 = PORT_PA07;
  //myprintf("Ya no sirve\n");
  
  data = receiveFromRasp();
  while(data == 0) {data = receiveFromRasp();}
  while(data != '\n') {data = receiveFromRasp();}
  myprintf("Received from Raspberry start signal\n");
  myprintf("Sending ACK to Raspberry\n");
  sendToRasp("ACK\n");
  //myprintf("ACK\n");
  data = receiveFromRasp();
  while(data == 0) {
	sendToRasp("ACK\n");
	data = receiveFromRasp();
  }
  while(data != '\n') {data = receiveFromRasp();}
  myprintf("Connection established...\n");

  myprintf("Sending data to Raspberry\n");
  sendToRasp("I want to be in Google Spreadsheet!\n");
  //myprintf("ACK\n");
  data = receiveFromRasp();
  while(data == 0) {
	sendToRasp("I want to be in Google Spreadsheet!\n");
	data = receiveFromRasp();
  }
  while(data != '\n') {data = receiveFromRasp();}
  myprintf("Received ACK from Raspberry\n");

  while (1) {}
}

void initUART(void) {
	/* port mux configuration*/
	PORT->Group[0].DIR.reg |= (1 << 10);                  /* Pin 10 configured as output */
	PORT->Group[0].PINCFG[PIN_PA11].bit.PMUXEN = 1;       /* Enabling peripheral functions */
	PORT->Group[0].PINCFG[PIN_PA10].bit.PMUXEN = 1;       /* Enabling peripheral functions */
	
	/*PMUX: even = n/2, odd: (n-1)/2 */
	PORT->Group[0].PMUX[5].reg |= 0x02;                   /* Selecting peripheral function C */
	PORT->Group[0].PMUX[5].reg |= 0x20;                   /* Selecting peripheral function C */
	
	/* APBCMASK */
	//PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;			  /* SERCOM 0 enable*/
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;

	/*GCLK configuration for sercom0 module: using generic clock generator 0, ID for sercom0, enable GCLK*/

	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM0_GCLK_ID_CORE) |
	GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);

	
	/* configure SERCOM0 module for UART as Standard Frame, 8 Bit size, No parity, BAUDRATE:9600*/

	SERCOM0->USART.CTRLA.reg =
	SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK |
	SERCOM_USART_CTRLA_RXPO(3/*PAD3*/) | SERCOM_USART_CTRLA_TXPO(1/*PAD2*/);
	
	uint64_t br = (uint64_t)65536 * (8000000 - 16 * 9600) / 8000000;
	
	SERCOM0->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);

	SERCOM0->USART.BAUD.reg = (uint16_t)br;

	SERCOM0->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
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
   SERCOM2->USART.CTRLA.reg = SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK | SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) |        SERCOM_USART_CTRLA_TXPO(0/*PAD0*/);
 
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

uint8_t spiSend(uint8_t data) {
  uint8_t temp;
  while(SERCOM1->SPI.INTFLAG.bit.DRE == 0){};
  SERCOM1->SPI.DATA.reg = data;
  while(SERCOM1->SPI.INTFLAG.bit.TXC == 0){};
  while(SERCOM1->SPI.INTFLAG.bit.RXC == 0){};
  temp = SERCOM1->SPI.DATA.reg;
  myprintf(" %x", temp);
  return temp;
}

void initSPI(void) {
           PM->APBCMASK.bit.SERCOM1_ = 1;
           GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_SERCOM1_CORE;
           while(GCLK->STATUS.bit.SYNCBUSY);
           const SERCOM_SPI_CTRLA_Type ctrla = {
                  .bit.DORD = 0, // Set as MSB first
                  .bit.CPHA = 0, // Set as Mode 0
                  .bit.CPOL = 0,  // Keep this as 0
                  .bit.FORM = 0x0, // Set as SPI frame
                  .bit.DIPO = 0x3, // Set as MISO on PAD[3]
                  .bit.DOPO = 0x0, // Set as MOSI on PAD[0], SCK on PAD[1], SS_ on PAD[2]
                  .bit.MODE = 0x3  // Set as Master
           };
           SERCOM1->SPI.CTRLA.reg = ctrla.reg;
           const SERCOM_SPI_CTRLB_Type ctrlb = {
                  .bit.RXEN = 1,   // Set as RX enabled
                  .bit.MSSEN = 0,  // Set as HW SS
                  .bit.CHSIZE = 0x0  // Set as 8-bit
           };
           SERCOM1->SPI.CTRLB.reg = ctrlb.reg;
 
           SERCOM1->SPI.BAUD.reg = 0xFF; // Rate is clock / 2
           // Mux for SERCOM1 PA16,PA17,PA19
           const PORT_WRCONFIG_Type wrconfig = {
           .bit.WRPINCFG = 1,
           .bit.WRPMUX = 1,
           .bit.PMUX = MUX_PA16C_SERCOM1_PAD0,
           .bit.PMUXEN = 1,
           .bit.HWSEL = 1,
           .bit.INEN = 1,
           .bit.PINMASK =(uint16_t)((PORT_PA16|PORT_PA17|PORT_PA19) >> 16)
           };
           PORT->Group[0].WRCONFIG.reg = wrconfig.reg;

           SERCOM1->SPI.CTRLA.bit.ENABLE = 1;
           while(SERCOM1->SPI.SYNCBUSY.bit.ENABLE);
}
