
#include "MicroNMEA.h"
#include <stdlib.h>
#include <string.h>

 static inline bool isEndOfFields(char c) {
    return c == '*' || c == '\0' || c == '\r' || c == '\n';
  }
 MicroNMEA_info_t nmea_info;

static long nmea_exp10(uint8_t b)
{
  long r = 1;
  while (b--)
    r *= 10;
  return r;
}


static char toHex(uint8_t nibble)
{
  if (nibble >= 10)
    return nibble + 'A' - 10;
  else
    return nibble + '0';

}


const char* skipField(const char* s)
{
  if (s == NULL)
    return NULL;

  while (!isEndOfFields(*s)) {
    if (*s == ',') {
      // Check next character
      if (isEndOfFields(*++s))
	break;
      else
	return s;
    }
    ++s;
  }
  return NULL; // End of string or valid sentence
}


static unsigned int parseUnsignedInt(const char *s, uint8_t len)
{
  int r = 0;
  while (len--)
    r = 10 * r + *s++ - '0';
  return r;
}


static long parseFloat(const char* s, uint8_t log10Multiplier, const char** eptr)
{
  int8_t neg = 1;
  long r = 0;
  while (isspace(*s))
    ++s;
  if (*s == '-') {
    neg = -1;
    ++s;
  }
  else if (*s == '+')
    ++s;
  
  while (isdigit(*s))
    r = 10*r + *s++ - '0';
  r *= nmea_exp10(log10Multiplier);

  if (*s == '.') {
    ++s;
    long frac = 0;
    while (isdigit(*s) && log10Multiplier) {
      frac = 10 * frac + *s++ -'0';
      --log10Multiplier;
    }
    frac *= nmea_exp10(log10Multiplier);
    r += frac;
  }
  r *= neg; // Include effect of any minus sign

  if (eptr)
    *eptr = skipField(s);

  return r; 
}


static long parseDegreeMinute(const char* s, uint8_t degWidth,
			     const char **eptr)
{
  if (*s == ',') {
    if (eptr)
      *eptr = skipField(s);
    return 0;
  }
  long r = parseUnsignedInt(s, degWidth) * 1000000L;
  s += degWidth;
  r += parseFloat(s, 6, eptr) / 60;
  return r;
}


static const char* parseField(const char* s, char *result, int len)
{
  if (s == NULL)
    return NULL;

  int i = 0;
  while (*s != ',' && !isEndOfFields(*s)) {
    if (result && i++ < len)
      *result++ = *s;
    ++s;
  }
  if (result && i < len)
    *result = '\0'; // Terminate unless too long

  if (*s == ',')
    return ++s; // Location of start of next field
  else
    return NULL; // End of string or valid sentence
}


static const char* generateChecksum(const char* s, char* checksum)
{
  uint8_t c = 0;
  // Initial $ is omitted from checksum, if present ignore it.
  if (*s == '$')
    ++s;
  
  while (*s != '\0' && *s != '*')
    c ^= *s++;

  if (checksum) {
    checksum[0] = toHex(c / 16);
    checksum[1] = toHex(c % 16);
  }
  return s;
}


static bool testChecksum(const char* s)
{
  char checksum[2];
  const char* p = generateChecksum(s, checksum);
  return *p == '*' && p[1] == checksum[0] && p[2] == checksum[1]; 
}


void MicroNMEA_dec(void)
{
  setBuffer(NULL, 0);
  NmeaClear();
}
void MicroNMEA_buffer(void* buf, uint8_t len)
{
  setBuffer(buf, len);
  nmea_info._zone = LOCAL_ZONE;
  NmeaClear();
}

void MicroNMEA_buffer_zone(void* buf, uint8_t len,int8_t timezone)
{
  setBuffer(buf, len);
  nmea_info._zone = timezone;
  NmeaClear();
}

void setBuffer(void* buf, uint8_t len)
{
  nmea_info._bufferLen = len;
  nmea_info._buffer = (char*)buf;
   nmea_info._ptr = nmea_info._buffer;
  if (nmea_info._bufferLen) {
    *(nmea_info._ptr) = '\0';
    nmea_info._buffer[nmea_info._bufferLen - 1] = '\0';
  }
}


void NmeaClear(void)
{
  nmea_info._navSystem = '\0';
  nmea_info._numSat = 0;
  nmea_info._hdop = 255;
  nmea_info._isValid = false;
  nmea_info._latitude = 0L;
  nmea_info._longitude = 0L;
  nmea_info._altitude = nmea_info._speed = nmea_info._course = 0;
  nmea_info._altitudeValid = false;
  nmea_info._year = nmea_info._month = nmea_info._day = 0;
  nmea_info._hour = nmea_info._minute = nmea_info._second = 0;
  nmea_info._hundredths = 0;
  }

bool nmea_process(char c)
{
  if (nmea_info._buffer == NULL || nmea_info._bufferLen == 0)
    return false;
  if (c == '\0' || c == '\n' || c == '\r') {
    // Terminate buffer then reset pointer
    *(nmea_info._ptr) = '\0'; 
    nmea_info._ptr = nmea_info._buffer;
    
    if (*(nmea_info._buffer) == '$' && testChecksum(nmea_info._buffer)) {
      // Valid message
      const char* data;
      if (nmea_info._buffer[1] == 'G') {
	nmea_info._talkerID = nmea_info._buffer[2];
	data = parseField(&nmea_info._buffer[3], &nmea_info._messageID[0], sizeof(nmea_info._messageID)); 
      }
      else {
	nmea_info._talkerID = '\0';
	data = parseField(&nmea_info._buffer[1], &nmea_info._messageID[0], sizeof(nmea_info._messageID)); 
      }
      
      if (strcmp(&nmea_info._messageID[0], "GGA") == 0)
	return processGGA(data);
      else if (strcmp(&nmea_info._messageID[0], "RMC") == 0)
	return processRMC(data);
      else 
      return true; // New valid MicroNMEA sentence
    }
    else {
      return false; // Not valid MicroNMEA sentence
    }
  }
  else {
    *(nmea_info._ptr) = c;
    if (nmea_info._ptr < &nmea_info._buffer[nmea_info._bufferLen - 1])
    {
      ++(nmea_info._ptr);
    }
    else if(nmea_info._ptr==&nmea_info._buffer[nmea_info._bufferLen - 1])
    {
      
    	nmea_info._ptr = nmea_info._buffer;
    	nmea_info._buffer[nmea_info._bufferLen - 1] = '\0';
    }
  }
  
  return false;
}

static const char* parseTime(const char* s)
{
  nmea_info._hour = parseUnsignedInt(s, 2);
  nmea_info._minute = parseUnsignedInt(s + 2, 2);
  nmea_info._second = parseUnsignedInt(s + 4, 2);
  nmea_info._hundredths = parseUnsignedInt(s + 7, 2);
  return skipField(s + 9);   
}

static const char* parseDate(const char* s)
{
  nmea_info._day = parseUnsignedInt(s, 2);
  nmea_info._month = parseUnsignedInt(s + 2, 2);
  nmea_info._year = parseUnsignedInt(s + 4, 2) + 2000;
  return skipField(s + 6);
}

bool IsLeap_d(void)
{
	return IsLeap(nmea_info._year);
}
bool IsLeap(uint16_t year)
{
	if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))
	{
		return true;
	}
	return false;
}

unsigned long makeUTC(void)
{
  DateTime datetime;
  datetime._year = nmea_info._year;
  datetime._month = nmea_info._month;
  datetime._day = nmea_info._day;
  datetime._hour = nmea_info._hour;
  datetime._minute = nmea_info._minute;
  datetime._second = nmea_info._second;
  return makeDatetimeUTC(datetime);
}
unsigned long makeDatetimeUTC(DateTime datetime)
{
	//----------------dayofMonth_normal----->01-02-03-04-05-06-07-08-09-10-11-12<-
	static const char dayofMonth_normal[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	//----------------dayofMonth_leap----->01-02-03-04-05-06-07-08-09-10-11-12<-
	static const char dayofMonth_leap[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
	unsigned long utc_temp = 0;
	int i = 0;
	int RunYearNum = 0;
	int day = 0;//day of year
	const char* day_month = NULL;
	for (i = 1970;i < datetime._year; i++)
	{
		if (IsLeap(i))
		{
			RunYearNum++;
		}
	}
	

	utc_temp = RunYearNum * 366 * 24 * 60 * 60;
	utc_temp += (datetime._year - 1970 - RunYearNum) * 365 * 24 * 60 * 60;

	if (IsLeap(datetime._year))
	{
		day_month = dayofMonth_leap;
	}
	else
	{
		day_month = dayofMonth_normal;
	}

	for (i = 1; i < datetime._month; i++)
	{
		day+=day_month[i - 1];
	}
	utc_temp += day * 24 * 60 * 60;

	/* day */
	utc_temp += (datetime._day - 1) * 24 * 60 * 60;

	/* hour */
	utc_temp += (datetime._hour) * 60 * 60;

	/* min */
	utc_temp += (datetime._minute) * 60;

	/* sec */
	utc_temp += datetime._second;

	//datetime._week = (((unsigned long)utc_temp / (24 * 60 * 60))+4) % 7;

	return utc_temp;
}

DateTime makeDateTime(void)
{
	return makeDateTime_utc_zone(nmea_info._utc,nmea_info._zone);
}

DateTime makeDateTime_zone(int8_t zone)
{
	return makeDateTime_utc_zone(nmea_info._utc, zone);
}

DateTime makeDateTime_utc(unsigned long utc)
{
	return makeDateTime_utc_zone(utc,nmea_info._zone);
}

DateTime makeDateTime_utc_zone(unsigned long utc, int8_t zone)
{
	DateTime _datetime;
	uint16_t years = 0;
	unsigned long local_time_t = utc + (zone * 3600);
	uint32_t secs = 0;
	uint32_t mins = 0;
	uint32_t hours = 0;
	uint32_t days = 0;
	uint32_t months =0;
	bool leap = false;
	//----------------dayofMonth_normal----->01-02-03-04-05-06-07-08-09-10-11-12<-
	static const char dayofMonth_normal[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	//----------------dayofMonth_leap----->01-02-03-04-05-06-07-08-09-10-11-12<-
	static const char dayofMonth_leap[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
	const char* dayofmonth = NULL;

	secs = local_time_t % 60;
	mins = local_time_t / 60;
	hours = (mins / 60);
	days = (hours / 24);

	_datetime._second = (uint8_t)secs;
	_datetime._minute = (uint8_t)(mins % 60);
	_datetime._hour = (uint8_t)(hours % 24);

	if (_datetime._hour || _datetime._minute || _datetime._second)
	{
		_datetime._week = (days + 1 +3) % 7;
	}
	else
	{
		_datetime._week = (days +3) % 7;
	}

	years = 1970;

	while (1)
	{
		leap = IsLeap(years);

		if (days < (leap ? 366 : 365))
		{
			break;
		}

		days -= (leap ? 366 : 365);
		years++;
	}

	_datetime._year = years;

	if (IsLeap(years))
	{
		dayofmonth = dayofMonth_leap;
	}
	else
	{
		dayofmonth = dayofMonth_normal;
	}

	for (months = 0; months < 12; months++)
	{
		if (days < (uint32_t)dayofmonth[months])
		{
			break;
		}
		days -= dayofmonth[months];
	}

	_datetime._day = days + 1;
	_datetime._month = months + 1;

	return _datetime;
}

DegLocation builddeg(double deg)
{
	DegLocation deg_pack;
	double temp_data = 0;
	//double temp_data1 = 0;
	//double temp_data2 = 0;
	//double temp_data3 = 0;
	deg_pack._deg = (int16_t)deg;

	temp_data = (double)((deg - (double)deg_pack._deg)*60.0);
	deg_pack._mins = (uint8_t)(temp_data) % 60;
	//temp_data1 = temp_data;

	temp_data = ((temp_data - (double)deg_pack._mins)*60.0);
	//temp_data2 = temp_data;

	deg_pack._sec = (uint8_t)(temp_data) % 60;

	temp_data = ((temp_data - (double)deg_pack._sec)*1000.0);
	//temp_data3 = temp_data;

	deg_pack._millsec = (uint16_t)temp_data;

	return deg_pack;

}

bool processGGA(const char *s)
{
  // If GxGSV messages are received _talker_ID can be changed after
  // other MicroNMEA sentences. Compatibility modes can set the talker ID
  // to indicate GPS regardless of actual navigation system used.
  nmea_info._navSystem = nmea_info._talkerID;
  
  s = parseTime(s);
  // ++s;
  nmea_info._latitude = parseDegreeMinute(s, 2, &s);
  if (*s == ',')
    ++s;
  else {
    if (*s == 'S')
      nmea_info._latitude *= -1; 
    s += 2; // Skip N/S and comma
  }
  
  nmea_info._longitude = parseDegreeMinute(s, 3, &s);
  if (*s == ',')
    ++s;
  else {
    if (*s == 'W')
      nmea_info._longitude *= -1;
    s += 2; // Skip E/W and comma
  }
  nmea_info._isValid = (*s == '1' || *s == '2');
  s += 2; // Skip position fix flag and comma
  nmea_info._numSat = parseFloat(s, 0, &s);
  nmea_info._hdop = parseFloat(s, 1, &s);
  nmea_info._altitude = parseFloat(s, 3, &s);
  nmea_info._altitudeValid = true;
  // That's all we care about
  return true;
}


bool processRMC(const char* s)
{
  // If GxGSV messages are received _talker_ID can be changed after
  // other MicroNMEA sentences. Compatibility modes can set the talker
  // ID to indicate GPS regardless of actual navigation system used.
  nmea_info._navSystem = nmea_info._talkerID;
  
  s = parseTime(s);
  nmea_info._isValid = (*s == 'A');
  s += 2; // Skip validity and comma
  nmea_info._latitude = parseDegreeMinute(s, 2, &s);
  if (*s == ',')
    ++s;
  else {
    if (*s == 'S')
      nmea_info._latitude *= -1; 
    s += 2; // Skip N/S and comma
  }
  nmea_info._longitude = parseDegreeMinute(s, 3, &s);
  if (*s == ',')
    ++s;
  else {
    if (*s == 'W')
      nmea_info._longitude *= -1;
    s += 2; // Skip E/W and comma
  }
  nmea_info._speed = parseFloat(s, 3, &s);
  nmea_info._course = parseFloat(s, 3, &s);
  s = parseDate(s);
  nmea_info._utc = makeUTC();
  nmea_info._local_datetime = makeDateTime();
  // That's all we care about
  return true;
}

  // Navigation system, N=GNSS, P=GPS, L=GLONASS, A=Galileo, '\0'=none
  char getNavSystem(void) {
    return nmea_info._navSystem;
  }

  uint8_t getNumSatellites(void) {
    return nmea_info._numSat;
  }

  // Horizontal dilution of precision, in tenths
  uint8_t getHDOP(void) {
    return nmea_info._hdop;
  }

  // Validity of latest fix
  bool isNmeaValid(void) {
    return nmea_info._isValid;
  }

  // Latitude in millionths of a degree. North is positive.
  long getLatitude(void) {
    return nmea_info._latitude;
  }
  
  // Longitude in millionths of a degree. East is positive.
  long getLongitude(void) {
    return nmea_info._longitude;
  }

  // Altitude in millimetres.
  bool getAltitude(long* alt)  {
    if (nmea_info._altitudeValid)
      *alt = nmea_info._altitude;
    return nmea_info._altitudeValid;
  }
  
  uint16_t getYear(void) {
    return nmea_info._year;
  }
  
  uint8_t getMonth(void)  {
    return nmea_info._month;
  }
  
  uint8_t getDay(void)  {
    return nmea_info._day;
  }
  
  uint8_t getHour(void)  {
    return nmea_info._hour;
  }
  
  uint8_t getMinute(void) {
    return nmea_info._minute;
  }
  
  uint8_t getSecond(void) {
    return nmea_info._second;
  }

  uint8_t getWeek(void) {
	  return nmea_info._week;
  }

  unsigned long getUtcSecont(void) {
	  return nmea_info._utc;
  }

  void setTimeZone(int8_t localzone) {
	  nmea_info._zone = localzone;
  }
  int8_t getTimeZone(void) {
	  return nmea_info._zone;
  }
  DateTime getLocalTime(void)
  {
	  return nmea_info._local_datetime;
  }
  uint8_t getHundredths(void) {
    return nmea_info._hundredths;
  }

  long getSpeed(void) {
    return nmea_info._speed;
  }

  long getCourse(void) {
    return nmea_info._course;
  }

  bool nmea_process(char c);

  // Current MicroNMEA sentence. 
  const char* getSentence(void) {
    return nmea_info._buffer;
  }

  // Talker ID for current MicroNMEA sentence
  char getTalkerID(void) {
    return nmea_info._talkerID;
  }

  // Message ID for current MicroNMEA sentence
  const char* getGnssMessageID(void) {
    return (const char*)nmea_info._messageID;
  }
  DegLocation getLatdeg(void)
  {
	  nmea_info._latdeg = builddeg((double)(nmea_info._latitude / 1e6));
	  return nmea_info._latdeg;
  }
  DegLocation getLongdeg(void)
  {
	  nmea_info._longdeg = builddeg((double)(nmea_info._longitude) / 1e6);
	  return nmea_info._longdeg;
  }


