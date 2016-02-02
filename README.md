# Breyes

INSA domain

Commentez votre code le plus souvent possible ! Veillez à bien renseigner les champs titres et commentaires lors d'un commit ou d'un pull request. C'est essentiel pour que le travail puisse être collaboratif. 

Une tâche = Une fonction. Les fonctions sont regroupées dans 3 fichiers : GPS_management.cpp, GSM_management.cpp, WAV_management.cpp. 

.h : Déclaration des prototypes de fonction + structures + constante #define à faire dans les .h. On peut déclarer une variable dans un .h mais on ne la fixe pas dans le .h, sinon il y a conflit de déclaration.

.cpp : Implémentation des fonctions + déclaration des variables dans les .cpp. 

Problème #1 : Optimisation mémoire : la taille du code doit rester inférieur à 32Ko. Il faut donc penser à optimiser son code. Ne pas utiliser de fonctions trop évoluées. Essayez de ne pas utiliser de grandes variables (ex: un tableau de float[1000] fait déjà 4Ko. Une chaîne de caractère de 1000 caractères fait 1Ko. 
Pour l'instant :
main + GPS_manager ~= 17Ko. 
librairie Smart_WAV ~= 11.7Ko
librairie wire (pour gérer le GPS) ~= 4Ko
bootloader (programme de base/obligatoire sur la ROM de l'arduino ~= 0.5Ko)
TOTAL ~= 32.5Ko
Avec les données à recevoir + la suite du programme ça dépasse. Il va surement faloir optimiser les librairies utilisées.
On n'utilise pas les fonctions d'appel avec la librairie GSM par exemple ---> ça dégage. 

-------------------------------

Convention de nommage des variables : 
Les variables/fonctions commencent par une lettre minuscule. Les fonctions relatives aux fonctions GPRS, SmartWAV, et GPS commencent respectivement par GPS_, WAV_, GPS_. Si plusieurs mots, séparation par le caractère "_". Les variables doivent avoir un nom qui a du sens : évitez les variables du genre : temp, a1, xdfkef.. Si les variables ont suffisamment de sens, pas besoin de surcharger avec des commentaires. 

------------------------------

Documentation : 

GitHub:
https://guides.github.com/activities/hello-world/
https://guides.github.com/introduction/flow/index.html
https://desktop.github.com/
Ces pages expliquent comment se servir des actions de base de git : créer une branche, commit (add/edit/delete un fichier), le pull request (feedback des autres utilisateurs sur votre/vos commits + comparaison de votre code avec le code principale), merge (intégration de votre travail dans le flux principal). 
Le dernier lien est relatif à l'utilisation de Git en logiciel sur Windows/Mac. 


Proxy/sockets:
http://beej.us/guide/bgnet/output/html/multipage/clientserver.html#simpleserver
Cette page présente comment coder un serveur web TCP et un client web TCP. TCP est le protocole de la couche transport qui nous permet de recevoir et transmettre des infos sans erreur. TCP oblige le client et le serveur à effectuer une phase de connexion. On souhaite que nos données soient fiable donc on utilisera ce système. 

Sockets: http://beej.us/guide/bgnet/
Il y a ici tout ce qu'il faut savoir sur l'utilisation des socket, et donc tout ce qu'il faut savoir pour programmer une application réseau. 

Arduino GPS : http://www.farnell.com/datasheets/1712631.pdf
https://www.arduino.cc/en/Reference/Wire

Arduino GSM : https://www.arduino.cc/en/Guide/ArduinoGSMShield#toc4
https://www.arduino.cc/en/uploads/Main/Quectel_M10_datasheet.pdf

Arduino SmartWAV : http://playground.arduino.cc/SmartWAV/SmartWAV
