/*
  main.cpp - main program.
  Br'Eyes, 2016.
  Released into the INSA domain.
  Ce client est configuré pour accéder à une page web précisé dans path. Notre objectif sera de formater ce path en fonction du bus choisi par la personne aveugle.
  Il est configuré pour accéder au réseau SFR (sl2sfr) avec mon code PIN (Il fallait bien ^^). On peut bien sûr modifier ces 2 paramêtres en fonction de la SIM insérée. 
  Basiquement, un client web envoi une requête GET vers un serveur WEB. Cette reqûete GET possède un format très précis, qui précise notamment le nom du serveur et la page demandé. 
  Un client web utilise le système de socket. C'est en fait un élement logiciel réseau qui agit comme l'interface entre la couche application et la couche transport du model OSI. 
  Tout programme réseau doit donc utiliser un socket pour utiliser le réseau. 
  Les sockets possèdent plusieurs fonctions associées, notemment:
  - connect pour se connecter au serveur
  - listen pour écouter les reqûete des clients (seulement sur les serveurs donc)
  - send pour envoyer les données
  - recv pour recevoir
  Sur arduino, les fonctions portent des noms différents mais le concept reste le même.

  Le main est codé comme une machine d'état (Mealy). Voir variable "state", et le switch plus loin. 
  Dans une première phase de test, l'état actuel est INIT, et les autres états sont annulés/commentés. 
  De cette façon on peut tester plus facilement 1) structure du programme 2) la connexion réseau 3) l'envoi des données GPS
*/

 // libraries

#include "GSM_manager.h"
#include "GPS_manager.h"
#include "SmartWAV_manager.h"

// Mealy machine states
enum state {INIT, BUS_STOP_RESEARCH, TRACK_SELECTION, NEXT_BUS_STOP, WAITING_FOR_ARRIVAL, GET_ON_CHECKING};
char actual_state, next_state;
boolean research_enable = true; //simulate a button to launch the closest bus stop research
char selection; //bus track selection

// Timer ressources variables

//GPS relative variables
GPS_coordinates actual_GPS_coordinates; 

//GSM relative variables

//SmartWAV relative variables

void setup() {
  actual_state = next_state = INIT;
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB port only
  Serial.println("Starting Arduino");
  GSM_connection_to_carrier();
}

void loop() {
  //Mealy machine
  switch(actual_state){
    /***************************************/  
    case INIT :
      if(research_enable){
        next_state = BUS_STOP_RESEARCH;
        actual_GPS_coordinates = GPS_get_coordinates();
        Serial.print("Coordinates [");
        Serial.print(actual_GPS_coordinates.longitude);
        Serial.print(" ; ");
        Serial.print(actual_GPS_coordinates.latitude);
        Serial.println("]");
        GSM_send_coordinates_to_proxy(actual_GPS_coordinates);
      }
      break;
    /***************************************/  
    case BUS_STOP_RESEARCH :
//      GSM_receive_data_from_proxy(); //waiting for the "bus dispo" file
//      
//      if(timer>20) { // il faut déclencher un compteur qui se réinitialise toutes les 20s et exécute cette condition, il faut aussi implémenter une fonction pour savoir si le fichier a été reçu en entier.
//        next_state = actual_state;
//        actual_GPS_cordinate = GPS_get_coordinates();
//        GSM_send_coordinates_to_proxy(actual_GPS_coordinates);
//        reset_timer();
//      } else {
//        next_state = BUS_STOP_SELECTION;
//        SmartWAV_avaible_buses_announcement();
//      }
      break;
    /***************************************/    
    case TRACK_SELECTION :
//      if(!selection){
//        next_state = NEXT_BUS_STOP;
//        GSM_send_selection_to_proxy(selection);
//      }
//      else {
//        next_state = actual_state;
//      }
      break; 
    /***************************************/    
    case NEXT_BUS_STOP :
//      GSM_receive_data_from_proxy(); //waiting for the "prohains passages" file
//      
//      if(timer>20) { // il faut déclencher un compteur qui se réinitialise toutes les 20s et exécute cette condition
//        next_state = actual_state;
//        actual_GPS_cordinate = GPS_get_coordinates();
//        GSM_send_coordinates_to_proxy(actual_GPS_coordinates);
//        reset_timer();
//      } else {
//        next_state = BUS_STOP_SELECTION;
//        SmartWAV_avaible_buses_announcement();
//      }
      break;
    /***************************************/    
    case WAITING_FOR_ARRIVAL :



    
      break;
    /***************************************/    
    case GET_ON_CHECKING :




    
      break;
    /***************************************/  
  }

  actual_state=next_state;

  //delay(1000);
  // do nothing forevermore:
    for (;;)
      ;
}


