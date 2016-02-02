/*
GSM_manager.h - Library for managing the network as client.
Br'Eyes, 2016.
Released into the INSA domain.
*/
#ifndef GSM_manager_h
#define GSM_manager_h

#include "GPS_manager.h" //in order to use the struct GSM_coordinates
#include <GSM.h>

// PIN Number
#define PINNUMBER      "1969"

// APN data 
#define GPRS_APN       "sl2sfr" // replace your GPRS APN
#define GPRS_LOGIN     ""       // replace with your GPRS login
#define GPRS_PASSWORD  ""       // replace with your GPRS password

// URL, path & port ***********************/
#define SERVER         "109.10.39.111" //public IP address
#define PORT           3490 //the same port is used within the proxy


boolean GSM_connection_to_carrier(void);
boolean GSM_connection_to_proxy(void);
void GSM_send_coordinates_to_proxy(GPS_coordinates coordinates);
void GSM_send_selection_to_proxy(char selection);
char GSM_receive_data_from_proxy(void);
void GSM_server_disconnecting(void);

#endif
