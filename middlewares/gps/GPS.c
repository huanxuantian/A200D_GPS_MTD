#define DBG_LVL    DBG_WARNING
#define DBG_TAG    "GPS"
#include <rtdbg.h>

#include <utils.h>

#include "gps.h"
#include <MicroNMEA.h>

//#include <math.h>


static char gps_buffer[250];
uint8_t GpsFlag = 2;
GPSINFO GpsInfo;
uint32_t last_gps_msg_time=0;
uint32_t last_check_time=0;
uint32_t last_pps_time=0;

uint32_t get_last_gps_msg_time(void)
{
	if(last_gps_msg_time==0) last_gps_msg_time = OSTimeGet();
	return last_gps_msg_time;
}

void set_last_gps_msg_time(void)
{
	last_gps_msg_time = OSTimeGet();
}

GPSINFO get_gps_info(){
	return GpsInfo;
}

void update_gps_info(){
		long temp;
	DateTime time;
	if(isNmeaValid()){
		GpsInfo.Statue =1;
		GpsInfo.SatelliteNum = getNumSatellites();
		//trans lon and lat
		temp = getLatitude();
		if(temp<0){
			GpsInfo.LatitudeNS=1;
			temp=0-temp;//abs
		}
		GpsInfo.Latitude =get_gb905_lat();
		//trans lon and lat
		temp = getLongitude();
		if(temp<0){
			GpsInfo.LongitudeEW=1;
			temp=0-temp;//abs
		}
		GpsInfo.Longitude=get_gb905_lon();
		//altitude (10m)
		if(getAltitude(&temp))
		{
			GpsInfo.Altitude = temp/1000;
		}else{
			GpsInfo.Altitude=0;
		}
		//speed(0.1km/h)
		temp = getSpeed();
		GpsInfo.Speed= ((int)(temp*MILPH_TO_KMPH*10.0))&0xFFFF;
		//course
		temp = getCourse();
		//dir(0-359)
		GpsInfo.dir= (temp)&0xFFFF;

		//update utc time
		time = getLocalTime();
		
	}else{
		GpsInfo.Statue =0;
	}
}

int gps_paser_callback(uint8_t *read_buf,int read_len){
	int i=0;
				#if GPS_DBG
		 LOG_W("parse req rec_len=%d",read_len);
		 for(i=0;i<read_len;i++){
			 if(i==0){LOG_W("data recv size=%d[",read_len);}
			 LOG_RAW("%c",*(read_buf+i));
			 if(i==(read_len-1)){LOG_W("]");}
			}
			#endif
	for(i=0;i<read_len;i++){
		if(nmea_process((char)(*(read_buf+i))&0x7F))
		{
			//GpsFlag=1;
			last_gps_msg_time = OSTimeGet();
			if(rt_strncmp(getGnssMessageID(), "RMC", 3) == 0){
				update_gps_info();
			}
		}
		else{
			#if 0
		 LOG_W("parse req rec_len=%d",read_len);
		 for(i=0;i<read_len;i++){
			 if(i==0){LOG_W("data recv size=%d[",read_len);}
			 LOG_RAW("%c",*(read_buf+i));
			 if(i==(read_len-1)){LOG_W("]");}
			}
			#endif
		}
	}

	return 0;
}

void GpsDataInit(void)
{
	memset(&GpsInfo,0,sizeof(GPSINFO));
	MicroNMEA_buffer(gps_buffer,sizeof(gps_buffer));
	
}
 float LatToRad(uint8_t *Lat)
 {
 	float Rad;
	uint16_t Data;
	 uint8_t i;
	 
	 for(i=0;i<4;i++)
	 {
		 if(isdigit(Lat[i]&0xff))
		 {
				continue;
		 }
		 	Rad = 0.0;
		  return Rad;
	 }
	 for(i=0;i<4;i++)
	 {
		 if(isdigit(Lat[i+5]&0xff))
		 {
				continue;
		 }
		 	Rad = 0.0;
		  return Rad;
	 }
	 
	Data=(Lat[0]-'0')*10+(Lat[1]-'0');
	Rad=(Lat[2]-'0')*10+(Lat[3]-'0')+(Lat[5]-'0')*0.1+(Lat[6]-'0')*0.01+(Lat[7]-'0')*0.001+(Lat[8]-'0')*0.0001;
	Rad=Rad/60;
	Rad=Rad+Data;
	return Rad;
				
 }
float LonToRad(uint8_t *Lon)
{
 	float Rad;
	uint16_t Data;
	uint8_t i;
		for(i=0;i<5;i++)
	 {
		 if(isdigit(Lon[i]&0xff))
		 {
				continue;
		 }
		 	Rad = 0.0;
		  return Rad;
	 }
	 	for(i=0;i<4;i++)
	 {
		 if(isdigit(Lon[i+6]&0xff))
		 {
				continue;
		 }
		 	Rad = 0.0;
		  return Rad;
	 }
	Data=(Lon[0]-'0')*100+(Lon[1]-'0')*10+(Lon[2]-'0');
	Rad=(Lon[3]-'0')*10+(Lon[4]-'0')+(Lon[6]-'0')*0.1+(Lon[7]-'0')*0.01+(Lon[8]-'0')*0.001+(Lon[9]-'0')*0.0001;
	Rad=Rad/60;
	Rad=Rad+Data;
	return Rad;
}
#if 0
double DistanceCal(float LatFrom,float LonFrom,float LatTo,float LonTo)
{
	double LatFrom1,LonFrom1,LatTo1,LonTo1,LonDiff;
	double Temp1,Temp2,Temp3;
	double Distance;
	LatFrom1=LatFrom*PI/180;
	LonFrom1=LonFrom*PI/180;
	LatTo1=LatTo*PI/180;
	LonTo1=LonTo*PI/180;
	LonDiff=LonTo1-LonFrom1;
	Temp1=cos(LatTo1)*sin(LonDiff);	
	Temp1=Temp1*Temp1;
	Temp2=cos(LatFrom1)*sin(LatTo1)-sin(LatFrom1)*cos(LatTo1)*cos(LonDiff);
	Temp2=Temp2*Temp2;
	Temp3=sin(LatFrom1)*sin(LatTo1)+cos(LatFrom1)*cos(LatTo1)*cos(LonDiff);
	Distance=atan(sqrt(Temp1+Temp2)/Temp3);
	Distance=EARTHR*Distance;
	return Distance ;		
}
#endif

uint8_t get_gps_status(void)
{
	uint8_t status=0;
	
	if(GpsInfo.Statue>=1)
	{
		status=1;
	}
	
	return status;
}


TIME_A get_str_time(uint8_t* atime)
{
	TIME_A time;
	//u16 sec_coount=0;
	char tmp[3];
	
	
	time.mode =0;
	
	//sec_coount = atoi((char*)atime);
	tmp[0]=atime[0];
	tmp[1]=atime[1];
	tmp[2]='\0';
	time.hour = atoi(tmp)+8;//sec_coount/3600;
	
	if(time.hour>=24)
	{
		time.hour =time.hour-24;
		time.mode =1;
	}
	
	tmp[0]=atime[2];
	tmp[1]=atime[3];
	tmp[2]='\0';
	time.min = atoi(tmp);//sec_coount/60;
	
	tmp[0]=atime[4];
	tmp[1]=atime[5];
	tmp[2]='\0';
	time.sec = atoi(tmp);//sec_coount%60;
	
	return time;
}

DATE_A get_str_date(uint8_t* adate)
{
	DATE_A date;
	//TIME_A time;
	char tmp[3];
	date.mode =1;
	//time = get_str_time(GpsInfo.UtcTime);
	
	tmp[0]=adate[0];
	tmp[1]=adate[1];
	tmp[2]='\0';
	date.day = atoi(tmp);
	
	tmp[0]=adate[2];
	tmp[1]=adate[3];
	tmp[2]='\0';
	date.mon = atoi(tmp);
	
	tmp[0]=adate[4];
	tmp[1]=adate[5];
	tmp[2]='\0';
	date.year = 2000 + atoi(tmp);
	
	return date;
}
int sec_day=86400;

char day_max_month[13]={31,28,31,30,31,30,31,31,30,31,30,31,29};

DATE_A date_add_oneday(DATE_A old_date)
{
	DATE_A new_date;
	memset(&new_date,0,sizeof(DATE_A));
	
	if(old_date.mon==2&&(old_date.year%4==0&&old_date.year%400!=0))//2‘¬»ÚƒÍº∆À„
	{
		if(old_date.day==day_max_month[12])//next day is 3/1
		{
			new_date.day = 1;
			new_date.mon = 3;
		}
		else//next day in 2/?
		{
			new_date.day = old_date.day + 1;
			new_date.mon = old_date.mon;
		}
		new_date.year = old_date.year;
	}
	else if(old_date.mon==12)
	{
		if(old_date.day==day_max_month[11])//next day is 1/1 and year add 
		{
			new_date.day = 1;
			new_date.mon = 1;
			new_date.year = old_date.year + 1;
		}
		else//next day in 2/?
		{
			new_date.day = old_date.day + 1;
			new_date.mon = old_date.mon;
			new_date.year = old_date.year;
		}
	}
	else if(old_date.mon<12 && old_date.mon>0)
	{
		if(old_date.day==day_max_month[(new_date.mon<=0?1:new_date.mon)-1])//next day is n/1
		{
			new_date.day = 1;
			new_date.mon = new_date.mon+1;
		}
		else//next day in 2/?
		{
			new_date.day = old_date.day + 1;
			new_date.mon = old_date.mon;
		}
		new_date.year = old_date.year;
	}
	else{
			new_date.day = old_date.day + 1;
			new_date.mon = old_date.mon;
			new_date.year = old_date.year;
	}
	
	return new_date;
}

utc_time get_utc(void)
{
	utc_time date_time;
	DateTime datetime_utc;

	datetime_utc = makeDateTime();

	
	date_time.date.year = datetime_utc._year;
	date_time.date.mon = datetime_utc._month;
	date_time.date.day = datetime_utc._day;
	
	date_time.time.hour = datetime_utc._hour;
	date_time.time.min = datetime_utc._minute;
	date_time.time.sec = datetime_utc._second;
	
	return date_time;
}

float get_real_lat(void)
{
	float lat_r;
	DegLocation lat_d;
	lat_d = getLatdeg();
	lat_r = lat_d._deg + (float)((double)(lat_d._mins/60.0) +(double)(lat_d._sec/3600.0) +(double)(lat_d._millsec/3600000.0));
	return lat_r;
}

float get_real_lon(void)
{
	float lon_r;
	DegLocation lon_d;
	lon_d = getLongdeg();
	lon_r = lon_d._deg + (float)((double)(lon_d._mins/60.0) +(double)(lon_d._sec/3600.0) +(double)(lon_d._millsec/3600000.0));
	return lon_r;
}

uint32_t get_gb905_lat(void){
	unsigned int lat_gb;//1/10000 cent
	DegLocation lat_d;
	lat_d = getLatdeg();	
	lat_gb = lat_d._deg*(60*10000) + lat_d._mins*10000 + 
		(unsigned int)(lat_d._sec*(10000/60.0)) +(unsigned int)(lat_d._millsec*(10/60.0));
	return lat_gb;
}


uint32_t get_gb905_lon(void){
	unsigned int lon_gb;//1/10000 cent
	DegLocation lon_d;
	lon_d = getLongdeg();	
	lon_gb = lon_d._deg*(60*10000) + lon_d._mins*10000 + 
		(unsigned int)(lon_d._sec*(10000/60.0)) +(unsigned int)(lon_d._millsec*(10/60.0));
	return lon_gb;
}

void print_gps_info(){
	DateTime datetime_utc;

	datetime_utc = makeDateTime();
	LOG_I("gps info flag=%d",GpsInfo.Statue);
	LOG_I("lon/lat = %ld/%ld(0.00001deg)",(long)(get_real_lon()*100000.0),(long)(get_real_lat()*100000.0));
	LOG_I("speed:%ld,getCourse:%ld",getSpeed(),getCourse());
	LOG_I("Altitude:%ld,SatelliteNum=%d",GpsInfo.Altitude,GpsInfo.SatelliteNum);
	LOG_I("datetime:%04d-%02d-%02d %02d:%02d:%02d/WEEK:%d",
		datetime_utc._year,datetime_utc._month,datetime_utc._day,
		datetime_utc._hour,datetime_utc._minute,datetime_utc._second,
		datetime_utc._week);
}
void sync_gpsinfo(){
	if(OSTimeGet() - last_gps_msg_time >300){
		LOG_I("gps data parse timeout");
		//print_gps_info();
		NmeaClear();
		update_gps_info();
		set_last_gps_msg_time();
	}
	else if(GpsFlag!=GpsInfo.Statue){
		LOG_I("gps data parse state change");
		print_gps_info();
		GpsFlag = GpsInfo.Statue;
		last_check_time = OSTimeGet();
	}
	else if(OSTimeGet() - last_check_time >30){
		LOG_I("gps data print timeout");
		print_gps_info();
		last_check_time = OSTimeGet();
		
	}
	#if 0
	if(last_pps_time==0||OSTimeGet() - last_pps_time >5){
		last_pps_time = OSTimeGet();
		change_pps();
	}
	#endif
	//check ant short and break here
}

