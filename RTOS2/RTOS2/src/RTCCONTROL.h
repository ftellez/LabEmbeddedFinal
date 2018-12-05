/*
 * RTCCONTROL.h
 *
 * Created: 10/7/2018 8:33:47 PM
 *  Author: BigJetPlane
 */ 

#include "sam.h"

#ifndef RTCCONTROL_H_
#define RTCCONTROL_H_


#define SEC_ADDR 0x00
#define MIN_ADDR 0x01
#define HOUR_ADDR 0x02
#define DAY_ADDR 0x03
#define DATE_ADDR 0x04
#define MONTH_ADDR 0x05
#define YEAR_ADDR 0x06



typedef union {
	struct  {
		uint8_t un_sec    :4, // units of seconds 
		dec_sec    :3, // tens of seconds
		CH :1; // not used
	} field;
	uint8_t reg; // declares the structure as a whole
} sec_type;

typedef union {
	struct  {
		uint8_t un_min    :4, // units of the minutes
		dec_min    :3, // tens of the mins
		 :1; // not used
	} field;
	uint8_t reg; // declares the structure as a whole
} min_type;

typedef union {
	struct  {
		uint8_t un_hour    :4, // hour units
		dec_hour    :1, // tens of hour
		am_pm :1, // is it am or pm? also tells if hour is in its twenties 
		h1224 :1, // tells if the hour is in one or the other format
		:1;
	} field;
	uint8_t reg;
} hour_type;

typedef union {
	struct  {
		uint8_t day    :3, // tells the day of the week
		:5;
	} field;
	uint8_t reg;
} day_type;


typedef union {
	struct  {
		uint8_t  un_date   :4, // units of the date
		dec_date :2, // decimals of the date
			:2;
	} field;
	uint8_t reg;
} date_type;

typedef union {
	struct  {
		uint8_t un_month    :4, // units of the month
		dec_mont    :1, // tens of the month
		RESERVED :3;
	} field;
	uint8_t reg;
} month_type;


typedef union {
	struct  {
		uint8_t un_year    :4, // units of the year
		dec_year    :4; // tens of the year
	} field;
	uint8_t reg;
} year_type;





void sendI2CDataArray(int SlAddr, uint8_t **ptrData, int Size,uint8_t RTCPtr);
void SendI2CData(int SlAddr,uint8_t Data);
void StopCond();
void receiveI2CDataArray(int SlAddr,uint8_t **ptrData,int Size);
void ptrReloc(int Address, int slvAdd);
void initI2C();






#endif /* RTCCONTROL_H_ */