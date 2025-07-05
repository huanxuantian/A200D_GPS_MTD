
#include "arduino_compat.h"
#ifndef MICRONMEA_H
#define MICRONMEA_H

#define MICRONMEA_VERSION "1.0.4"
#include <limits.h>
#include <ctype.h>
/*
 * A simple Arduino class to process MicroNMEA sentences from GPS and GNSS
 * receivers.
 */
#define MILPH_TO_KMPH (1.60934)

typedef struct __DateTime {
	uint16_t _year;
	uint8_t _month;
	uint8_t _day;

	uint8_t _hour;
	uint8_t _minute;
	uint8_t _second;
	//
	uint8_t _week;
}DateTime;

typedef struct __DegLocation {
	int16_t _deg;//0-180 for longitude  0-90 for latitude
	uint8_t _mins;//0-60
	uint8_t _sec;//0-60
	uint16_t _millsec;//0-999
}DegLocation;


typedef struct
{
	    // Sentence buffer and associated pointers
   uint8_t _bufferLen;
   char* _buffer;
   char *_ptr;

  // Information from current MicroNMEA sentence
   char _talkerID;
   char _messageID[6];

  // Variables parsed and kept for user
   char _navSystem;
   bool _isValid;
   long _latitude, _longitude; // In millionths of a degree
  long _altitude; // In millimetres
   bool _altitudeValid;
   long _speed, _course;


   unsigned long _utc;
   int8_t _zone;
   DateTime _local_datetime;
   DegLocation _latdeg;
   DegLocation _longdeg;

   uint16_t _year;
   uint8_t _month, _day, _hour, _minute, _second, _hundredths,_week;
   uint8_t _numSat;
   uint8_t _hdop;
 }MicroNMEA_info_t;


#define LOCAL_ZONE	8
  
  // Object with no buffer allocated, must call setBuffer later
  void MicroNMEA_dec(void);
  
  // User must decide and allocate the buffer
  void MicroNMEA_buffer(void* buffer, uint8_t len);

  void MicroNMEA_buffer_zone(void* buf, uint8_t len,int8_t timezone);
  
  void setBuffer(void* buf, uint8_t len);
  
  // Clear all fix information. isNmeaValid() will return false, Year,
  // month and day will all be zero. Hour, minute and second time will
  // be set to 99. Speed, course and altitude will be set to
  // LONG_MIN; the altitude validity flag will be false. Latitude and
  // longitude will be set to 999 degrees.
  void NmeaClear(void);


  bool IsLeap_d(void);
	unsigned long makeDatetimeUTC(DateTime datetime);
  unsigned long makeUTC(void);
  DateTime makeDateTime(void);
  bool IsLeap(uint16_t year);
  DateTime makeDateTime_utc(unsigned long utc);
  DateTime makeDateTime_zone(int8_t zone);
  DateTime makeDateTime_utc_zone(unsigned long utc, int8_t zone);
	
  DegLocation builddeg(double deg);
	
 
  
  //const char* parseTime(const char* s);
  //const char* parseDate(const char* s);

  bool processGGA(const char *s);
  bool processRMC(const char* s);

	bool nmea_process(char c);
	
	char getNavSystem(void);
	uint8_t getNumSatellites(void);
	uint8_t getHDOP(void);
	
	bool isNmeaValid(void);
	long getLatitude(void);
	long getLongitude(void);
	bool getAltitude(long* alt);
	
	const char* getGnssMessageID(void);
	
	uint16_t getYear(void);
	uint8_t getMonth(void);
	uint8_t getDay(void);
	uint8_t getHour(void);
	uint8_t getMinute(void);
	uint8_t getSecond(void);
	uint8_t getWeek(void);
	
	unsigned long getUtcSecont(void);
	void setTimeZone(int8_t localzone);
	int8_t getTimeZone(void);
	DateTime getLocalTime(void);
	uint8_t getHundredths(void);
	
	long getSpeed(void);
  long getCourse(void);	
	const char* getSentence(void);
	char getTalkerID(void);
	
	DegLocation getLatdeg(void);
	DegLocation getLongdeg(void);


#endif
