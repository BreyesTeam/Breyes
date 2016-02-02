/*
GSM_manager.cpp - Library for managing the network as client.
Br'Eyes, 2016.
Released into the INSA domain.
*/
#include "GSM_manager.h"

// initialize the library instance
GSMClient client;
GPRS gprs;
GSM gsmAccess;

boolean GSM_connection_to_carrier(void){
// connection state
  boolean notConnected = true;

  // After starting the modem with GSM.begin()
  // attach the shield to the GPRS network with the APN, login and password
  while (notConnected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      notConnected = false;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  return notConnected;
}

boolean GSM_connection_to_proxy(void){
  boolean notConnected = true;
  Serial.println("connecting...");
  // if you get a connection, report back via serial
  if (client.connect(SERVER, PORT)) {
    Serial.println("connected");
    notConnected = "false";
  } else {
    // if you didn't get a connection to the proxy
    Serial.println("connection failed");
  }
  return notConnected;
}

void GSM_send_coordinates_to_proxy(GPS_coordinates coordinates){
  Serial.print("Send coordinates [");
  Serial.print(coordinates.latitude);
  Serial.print(" ; [");
  Serial.print(coordinates.longitude);
  Serial.println("]...");
  
  client.println("C"); //inform the proxy of the GSP imminent coordinates sending
  client.println(coordinates.latitude);
  client.println(coordinates.longitude);
}

void GSM_send_selection_to_proxy(char selection){
  Serial.print("Send selection (bus number ");
  Serial.print(selection);
  Serial.println(")...");
  
  client.println(selection); //inform the proxy of the bus selected
}

char GSM_receive_data_from_proxy(void){
  char c;
  // if there are incoming bytes available from the proxy, read them and print them
  if (client.available()) {
    c = client.read();
    Serial.print(c);
  }
  return c; // à remplacer par la vraie chaine de caractère JSON
}

void GSM_server_disconnecting(void){
  // if the server's disconnected, stop the client
  if (!client.available() && !client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
}



