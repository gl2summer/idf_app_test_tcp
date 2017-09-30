#ifndef _WIFI_STATION_H__
#define _WIFI_STATION_H__





#ifndef IP_ARRAY
#define IP_ARRAY(a) (a)[0], (a)[1], (a)[2], (a)[3]
#define IP2STR_FORMAT "%d.%d.%d.%d"
#endif






void wifi_station_main(bool clean);


#endif //_WIFI_STATION_H__

