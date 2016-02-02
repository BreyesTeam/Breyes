/*
GPS_manager.h - Library for getting the latitude and longitude.
Br'Eyes, 2016.
Released into the INSA domain.
*/
#ifndef GPS_manager_h
#define GPS_manager_h

//GPS structure ***************************/
struct GPS_coordinates{
  float latitude;
  float longitude;
};

GPS_coordinates GPS_get_coordinates(void);

#endif

