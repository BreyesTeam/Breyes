/*
GPS_manager.cpp - Library for getting the latitude and longitude.
Br'Eyes, 2016.
Released into the INSA domain.
*/
#include "GPS_manager.h"

GPS_coordinates GPS_get_coordinates(void){
  GPS_coordinates coordinates;  
  //pour l'instant je simule ces données fictives
  coordinates.latitude = -0.15545454;
  coordinates.longitude = 0.54788844;
  
  return coordinates;
}

