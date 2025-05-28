
#ifndef __PKG_GNSS_SK_GPS_H_
#define __PKG_GNSS_SK_GPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include<stdio.h>
#include<string.h>

#define NMEA_COUNT_MAX 2800
#ifndef PI 
#define PI 3.1415926535898
#endif
#define EARTHR 6371004 
typedef struct
{
	uint8_t bcdTime[9]; //YYYYMMDDhhmmssnnnn
	uint8_t Statue;   //1(OK),0(NOT LOCATION)
	uint32_t  Latitude;	//1/10000 cent
	uint8_t LatitudeNS; //N:0,S:1
	uint32_t Longitude;	 //1/10000 cent
	uint8_t LongitudeEW;//E:0,S:1
	uint16_t Speed;	  //0.1km/h
	uint16_t dir;	 //0-359
	uint16_t Altitude;   //high 0.1m
	uint8_t SatelliteNum;//
}GPSINFO;



typedef struct 
{
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t mode;
}TIME_A;
typedef struct
{
	uint16_t year;
	uint8_t mon;
	uint8_t day;
	uint8_t mode;
}DATE_A;

typedef struct
{
	DATE_A date;
	TIME_A time;
}utc_time;


int gps_paser_callback(uint8_t *read_buf,int read_len);

uint32_t get_last_gps_msg_time(void);
void set_last_gps_msg_time(void);

void GpsDataInit(void);

float LatToRad(uint8_t *Lat);
float LonToRad(uint8_t *Lon);

double DistanceCal(float LatFrom,float LonFrom,float LatTo,float LonTo);

uint8_t get_gps_status(void);
GPSINFO get_gps_info();

TIME_A get_str_time(uint8_t* atime);

DATE_A get_str_date(uint8_t* adate);

utc_time get_utc(void);

float get_real_lat(void);
float get_real_lon(void);
uint32_t get_gb905_lat(void);
uint32_t get_gb905_lon(void);
void sync_gpsinfo();
#ifdef __cplusplus
}
#endif

#endif /* __PKG_GNSS_SK_H_ */