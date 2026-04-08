/////////////////////////////////////////////////
//
//  PROGRAMME PAV  MàJ du 06/04/2026 by PhA
//  v2.0  La lecture de la tension batterie n'est pas opérationnelle
//  v2.1  Modif luminosité 0-2 devient 1-3   1 faible, 2 moyen, 3 fort
// 
/////////////////////////////////////////////////

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include "definitions.h"
#include "cdipswitch.h"
#include "cneopixel.h"

/////////////////////////////////////////
// VARIABLES ET INSTANCIATION GLOBALES
/////////////////////////////////////////

//
// A mettre à jour selon le contexte d'utilisation
//
const char *tcpAddress = "192.168.0.10";
const char *ssid = "AP-ESTOM";
const char *password = "ESTOM2025";
//const char *tcpAddress = "192.168.4.62";
//const char *ssid = "STS_C12";
//const char *password = "PervasioN";
const uint16_t tcpPort = 5005;
//
// FIN METTRE A JOUR
//

T_ETATSPAV etatsPAV=VIDE; // par défaut au démarrage état du PAV
//
// instanciations
//
CDipSwitch ds;
CNeoPixel afficheur(NUM_LEDS, LED_PIN, DELAYVAL);
WiFiClient clientTcp;
IPAddress adrIpLocale;
//
// etats et valeurs de l'application
//
byte lastOctet = 1; // par défaut
// TODO prévoir si batterie faible alors luminosite revient à faible : 0
uint8_t luminosite=0; // par défaut faible  1 moyen  2 fort
uint8_t dsCouleur=1;  // no des switch sw2 et 3, donc entre 0-3
int type=0;  // type de device : PAV ou abri bus (sw1 = 1)
int c=0; // pour le clignotement de la couleur

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/* IR
 * Set library modifiers first to set output pin etc.
 */
#include "PinDefinitionsAndMore.h"
//#define IR_OUTPUT_IS_ACTIVE_LOW
#define IRSND_IR_FREQUENCY          38000
#define IRSND_PROTOCOL_NAMES        1 // Enable protocol number mapping to protocol strings - requires some FLASH.
#include <irsndSelectMain15Protocols.h>
// or disable #include <irsndSelectMain15Protocols.h> and use only one protocol to save programming space
//#define IRSND_SUPPORT_NEC_PROTOCOL        1
//#define IRSND_SUPPORT_NEC_PROTOCOL        1
/* IR
 * After setting the definitions we can include the code and compile it.
 */
#include <irsnd.hpp>
IRMP_DATA irsnd_data;
#if defined(SEND_SAMSUNG)
union WordUnion{
    struct {
        uint8_t LowByte;
        uint8_t HighByte;
    } UByte;
    struct {
        int8_t LowByte;
        int8_t HighByte;
    } Byte;
    uint8_t UBytes[2];
    int8_t Bytes[2];
    uint16_t UWord;
    int16_t Word;
    uint8_t *BytePointer;
};
#endif

////////////////////////////////////////////////////////////////////////////
void connectToWiFi() {
  Serial.print("Connexion au WiFi");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while ( (WiFi.status()!=WL_CONNECTED) && ( (millis()-startAttemptTime)<10000) ) {
    delay(500);
    Serial.print(".");
  } //wh
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnecté au WiFi");
    Serial.print("Adresse IP locale : ");
    Serial.println(WiFi.localIP());
    adrIpLocale = WiFi.localIP();
    lastOctet = adrIpLocale[3];  // dernier octet
    Serial.print("\ndernier octet :");
    Serial.println(adrIpLocale[3]);
  } else {
    Serial.println("\nÉchec de connexion WiFi. RESTART !");
    ESP.restart();  // redémarrage logiciel
  } // else
} // fonction
///////////////////////////////////////////////////////////////////////////////
bool connectToServer() {
  Serial.println("Connexion au serveur TCP...");
  if (clientTcp.connect(tcpAddress, tcpPort)) {
    Serial.println("Connexion TCP réussie.");
    return true;
  } else {
    Serial.println("Échec de connexion TCP.");
    return false;
  } // else
} // fonction
///////////////////////////////////////////////////////////////////////////////
void sendTCPMessageToServer(T_TYPETRAME trame) {
  if (!clientTcp.connected()) {
    Serial.println("Connexion perdue pour envoi, tentative de reconnexion...");
    if (!clientTcp.connect(tcpAddress, tcpPort)) {
      Serial.println("Échec de reconnexion pour envoi.");
      return;
    } // if
  } // if
  DynamicJsonDocument doc(256);
  switch(trame) {
    case BONJOUR: 
      doc["status"] = "0";
      doc["type"] = "PAV";
      doc["couleur"] = String(dsCouleur);
    break;
    default:  // A effacer dans le futur pour les tests seulement
      doc["status"] = 99;
    break;
  } // sw
  serializeJson(doc, clientTcp);  // envoi json vers le serveur
  clientTcp.println(); // envoi saut de ligne vers le serveur
  Serial.print("Infos envoyées : ");
  serializeJson(doc, Serial);
  Serial.println();
} // fonction

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200); // init terminal de communication
  // connexion WIFI
  connectToWiFi(); // reset si problème connexion
  // connexion au serveur TCP
  while (!connectToServer()) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nÉchec de connexion WiFi. RESTART !");
      ESP.restart();
    } // if
  } // wh

  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));
  irsnd_init();
  irmp_irsnd_LEDFeedback(true); // Enable send signal feedback at LED_BUILTIN
  Serial.println(F("Send IR signals at pin " STR(IRSND_OUTPUT_PIN)));

#if defined(SEND_SAMSUNG)
    /*
     * Send Samsung32
     */
    irsnd_data.protocol = IRMP_SAMSUNG32_PROTOCOL;
    irsnd_data.address = 0x0707;
    irsnd_data.command = 0xFB04; // For my Samsung, the high byte is the inverse of the low byte
    irsnd_data.flags = 0; // repeat frame 0 time
#else
    /*
     * Send NEC
     */
    irsnd_data.protocol = IRMP_NEC_PROTOCOL;
    irsnd_data.address = 0x25DA; // Addresse avec octet et octet complémenté 
    irsnd_data.command = 0x01; // The required inverse of the 8 bit command is added by the send routine.
    irsnd_data.flags = 1; // repeat frame
#endif

  // true = wait for frame and trailing space/gap to end. This stores timer state and restores it after sending.
  if (!irsnd_send_data(&irsnd_data, true)) {
      Serial.println(F("Protocol not found")); // name of protocol is printed by irsnd_data_print()
  }
  irsnd_data_print(&Serial, &irsnd_data);
  
  // lecture des sw au démarrage seulement
  ds.setup();  // DIP SW
  dsCouleur = ds.getDsCouleur();
  type = ds.getType();   // 1 abri bus   0 PAV avec couleur issue de SW1 et SW2
  Serial.print("Couleur=");
  Serial.println(dsCouleur);
  Serial.print("Type=");
  Serial.println(type);

  // init couleur et afficheur LED
  afficheur.begin(); // require to intialize object
  afficheur.on(dsCouleur, luminosite);
  delay(400);
  afficheur.off();

  // témoin d'émission IR
  pinMode(21,OUTPUT); // Pilotage USER_LED jaune
  digitalWrite(21, HIGH);

  sendTCPMessageToServer(BONJOUR);  // Envoi initial après connexion
  delay(500); // attente réponse du serveur
} // setup

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // test Connexion WIFI et au serveur ESTOM sinon restart
  if (!clientTcp.connected()) {
    Serial.println("\nÉchec de connexion au serveur. RESTART !");
    ESP.restart();
  } // if

  // test si trame reçue et lecture trame TCP from serveur
  if (clientTcp.available()) {
    String trameRec = clientTcp.readStringUntil('\n');
    Serial.println(trameRec);
    StaticJsonDocument<512> data;
    DeserializationError error = deserializeJson(data, trameRec);
    if (error) {
      Serial.print("Erreur JSON : ");
      Serial.println(error.f_str());
      return;
    } // if
    if (!data.containsKey("ordre")) {
      Serial.println("Trame invalide (pas de clé 'Ordre').");
      return;
    } // if

    int ordre = data["ordre"];
    Serial.print("Ordre reçu : ");
    Serial.println(ordre);
    switch (ordre) {
      case 1: // début partie
      case 13: // trame annulation transfert
        etatsPAV = PLEIN; break;
        etatsPAV = PLEIN; break;
      case 11: // trame départ transfert
        etatsPAV = VIDAGE; break;
      case 0: // trame init
      case 2: // trame fin partie
      case 12: // trame fin transfert
        if (data.containsKey("luminosite")) 
          luminosite = data["luminosite"];
        etatsPAV = VIDE; break;
      default:
        Serial.println("Ordre inconnu."); break;
    } // sw
    Serial.print("EtatPAV=");
    Serial.println(etatsPAV);
  } // if available

  // TODO Lire Batterie faible
  // TODO Si batterie faible alors luminosite=0

  // contrôle de l'afficheur LED
  switch(etatsPAV) {
    case VIDE: 
      afficheur.off(); 
      break;
    case PLEIN: 
      afficheur.on(dsCouleur, luminosite);
      break;
    case VIDAGE: 
      if (c==0) c=20; else c=0; 
      afficheur.clignote(dsCouleur, luminosite, c);
      break;
    default: Serial.println("état inconnu."); break;
  } // sw

  // contrôle de l'IR avec émission data
  switch(etatsPAV) {
      case PLEIN:
      case VIDAGE:
        digitalWrite(21, LOW);
        // Utilisation de la bibliothèque IRMP
        irsnd_data.command=(lastOctet<<3)|(dsCouleur<<1)|type;
// ou       irsnd_data.command=((dsCouleur<<1)|type);
        irsnd_send_data(&irsnd_data, true);
        irsnd_data_print(&Serial, &irsnd_data);
        //delay(100);
        digitalWrite(21, HIGH);
        break;
  } // sw
} // fonction
