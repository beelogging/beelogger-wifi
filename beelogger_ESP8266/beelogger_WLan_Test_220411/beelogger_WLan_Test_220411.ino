/*
   (C) 2022 R.Schick

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// beelogger.de - Arduino Datenlogger für Imker
// Erläuterungen dieses Programmcodes unter http://beelogger.de
// Version 11.04.2022

//----------------------------------------------------------------
// Konfiguration
//----------------------------------------------------------------

#define DEBUG__ESP 1

// Name des WLAN-Zugangspunkts
const char  Access_Point[] PROGMEM = "AP_Name";

// Passwort des Zugangspunktes
const char AP_Passwort[] PROGMEM =   "AP_Passwort";

// Domainname zum Webserver mit beelogger-Skript, Bsp:"meineDomain.de";
const char serverName[] PROGMEM = "community.beelogger.de";

// GET mit Verzeichnis  Webserver-Skript und das PHP Skript für den jeweiligen beelogger
// Bsp: "GET /USER_X/beelogger6/beelogger_log.php?"
// Achtung: Stringlänge muss zur Variable "parameter" in send_data_via_wlan() passen
// Anzahl Byte:                       10        20        30        40        50
const char pfad[] PROGMEM = "GET /USERX/beelogger1/beelogger_log.php?";   //USERX = meinOrdner bzw. Name, Y = beelogger Nummer  1...n

// Passwort vom Webserver-Skript
const char Passwort[] PROGMEM =  "Log"; // Log = Default


//----------------------------------------------------------------
// Ende Konfiguration
//----------------------------------------------------------------

#include <Wire.h>
#include <Sodaq_DS3231.h>

//----------------------------------------------------------------
// Pin-Belegung beelogger
//----------------------------------------------------------------
#define HX711_SCK     A0    // HX711 S-Clock
#define Modul_CS      10    // Ethernet abschalten
#define Power_Pin      4    // Power On
#define _DS3231_ADDRESS 0x68
//----------------------------------------------------------------

#define Seriell_Baudrate 9600

//----------------------------------------------------------------
// ESP-8266 WLAN Modul
//----------------------------------------------------------------
byte ESP_RX = 8;
byte ESP_TX = 9;
#define ESP_Baudrate 9600

#define ESP_RESET A2

#include "ESP_beelogger.h"
C_WLAN_ESP01 my_esp;

const char Str_Http[]  PROGMEM = " HTTP/1.1\r\n";
const char Str_Con[]   PROGMEM = "Connection: close\r\n\r\n";
//----------------------------------------------------------------


// Test-Messdaten (regulär kommen die Messdaten von den Sensoren)
float TempIn = 25.00;
float TempOut = 35.00;
float FeuchteIn = 40.00;
float FeuchteOut = 60.00;
float Licht = 50000.00;
float Gewicht = 25.0;
float Batteriespannung = 3.70;
float Solarspannung = 1.70;
float Service = 0.0;

uint8_t OK = true;
char parameter[80], data[64];
uint8_t first_run = 1;
uint8_t Ds3231_ok = 0;


void setup() {
  digitalWrite (HX711_SCK, HIGH); //HX711 aus
  pinMode(HX711_SCK, OUTPUT);

  digitalWrite(Modul_CS, HIGH);   // Modul Ethernet usw. aus
  pinMode(Modul_CS, OUTPUT);

  digitalWrite(Power_Pin, HIGH);  // Power an
  pinMode(Power_Pin, OUTPUT);

  digitalWrite(ESP_RESET, HIGH);  // ESP8266 an
  pinMode(ESP_RESET, OUTPUT);

  Serial.begin(Seriell_Baudrate);
  while (!Serial ) {};
  delay(100);
  Serial.println(F("ESP 8266 Test Sketch 11.04.2022"));
  Serial.flush();
  delay(1000);


  Serial.println(F("\nTeste DS3231 vorhanden?"));
  Serial.flush();
  Wire.begin();
  Wire.beginTransmission(_DS3231_ADDRESS);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    Ds3231_ok = 1;
    Serial.println("DS3231 gefunden\n");
    rtc.begin();
  }
  else {
    Serial.println("nein\n");
  }
  Serial.println(F("Abfrage Modul ... "));
  if (my_esp.init(ESP_Baudrate)) {
    Serial.println("i.O.");
    my_esp.mode();      // Station Mode permanent
    Serial.println(" ");
    Serial.println(F("ESP Firmware:"));
    my_esp.firmware();  // firmware abfragen
    Serial.println(my_esp.buffer);
    Serial.println(" ");
    Serial.flush();
    WLan_Info();
  }
  else {
    OK = false;
    Serial.println(F("Modul konnte nicht abgefragt werden"));
    Serial.println(my_esp.buffer);
    Serial.println(" ");
    Serial.flush();
    ESP_Abschaltung();
  }

  strcpy_P(parameter, pfad);

  char *p_buf = strstr(parameter, "/beelogger");  // search first /
  uint8_t n = atoi(p_buf + 10);
  //Serial.println(p_buf + 10);
  if (n == 0) {
    Serial.println(parameter);
    Serial.println(F("\nWLan Test Sketch nur mit beeloggerX,  X = 1...n"));
    Serial.println(F("keine DUO, Triple, Quad, usw."));
    Serial.flush();
    OK = false;
    ESP_Abschaltung();
  }

  if (OK) {
    strcpy_P(parameter, Access_Point);
    Serial.print(F("Verbinde zum WLAN: "));
    Serial.print(parameter);
    Serial.println(" ... ");
    Serial.flush();
    strcpy_P(data, AP_Passwort);

    if (my_esp.join(parameter, data)) {
      Serial.println(F("WLAN Verbindung OK"));
      Serial.println("\n");
      Serial.flush();
    }
    else {
      OK = false;
      Serial.println(F("Verbindungsfehler!\n"));
      Serial.flush();
      ESP_Abschaltung();
    }
  }

  if (OK) {
    Serial.println(F("Verbinde mit Server ... "));
    strcpy_P(parameter, serverName);
    Serial.print(parameter);
    if (my_esp.Connect(parameter)) {
      Serial.println(F("  ... connect: OK\n"));
      Serial.flush();
    }
    else {
      OK = false;
      Serial.println(F(" connect fehlgeschlagen\n"));
      Serial.println(my_esp.buffer);
      Serial.println(" ");
      Serial.flush();
      ESP_Abschaltung();
    }
  }
}




//#########################################
void loop() {
  char KonvertChar[15];
  char data[80];
  char parameter[64];

  if ((!first_run) && (OK)) {
    Serial.print("ESP ... ");
    Serial.flush();
    if (my_esp.init(ESP_Baudrate)) {  // baud rate
      Serial.println("o.k.");
      Serial.flush();
      strcpy_P(parameter, Access_Point);
      strcpy_P(data, AP_Passwort);
      if (my_esp.join(parameter, data)) {
        Serial.println("..");
        Serial.flush();
        strcpy_P(parameter, serverName);
        if (my_esp.Connect(parameter)) {
          Serial.println(".");
          Serial.flush();
          OK = true;
        }
      }
    }
  }
  else  first_run = 0;

  if (OK) {

    float Check = round(Service + TempIn + TempOut + FeuchteIn + FeuchteOut + Licht + Gewicht + Solarspannung + Batteriespannung);

    if (my_esp.prep_send(300)) { // fiktive Länge, senden wird mit 0x00 gestartet
      Serial.println(F("Uebertrage Daten zum Webserver ..."));
      Serial.flush();

      //"GET " + url + php + "?" + Daten + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
      // Wichtig: der String darf von der url bis Ende der Daten keine Leerzeichen enthalten

      strcpy_P(parameter, pfad);
      Serial.println(parameter);
      Serial.flush();
      my_esp.send(parameter);

      // erstelle Daten
      my_esp.send("Passwort=");
      strcpy_P(parameter, Passwort);
      my_esp.send(parameter);
      if (Ds3231_ok == 1) {
        my_esp.send("&Z=2");
      }
      my_esp.send("&TempIn=");     dtostrf(TempIn, 4, 2, KonvertChar);     my_esp.send(KonvertChar);
      my_esp.send("&TempOut=");    dtostrf(TempOut, 4, 2, KonvertChar);    my_esp.send(KonvertChar);
      my_esp.send("&FeuchteIn=");  dtostrf(FeuchteIn, 4, 2, KonvertChar);  my_esp.send(KonvertChar);
      my_esp.send("&FeuchteOut="); dtostrf(FeuchteOut, 4, 2, KonvertChar); my_esp.send(KonvertChar);
      my_esp.send("&Licht=");      dtostrf(Licht, 4, 2, KonvertChar);      my_esp.send(KonvertChar);
      my_esp.send("&Gewicht=");    dtostrf(Gewicht, 1, 1, KonvertChar);    my_esp.send(KonvertChar);

      my_esp.send("&VBatt=");      dtostrf(Batteriespannung, 4, 2, KonvertChar); my_esp.send(KonvertChar);
      my_esp.send("&VSolar=");     dtostrf(Solarspannung, 4, 2, KonvertChar);    my_esp.send(KonvertChar);

      my_esp.send("&S=");          dtostrf(Service, 4, 2, KonvertChar);      my_esp.send(KonvertChar);
      my_esp.send("&Check=");      dtostrf(Check, 6, 2, KonvertChar);        my_esp.send(KonvertChar);
      // ende Daten
      //
      strcpy_P(parameter, Str_Http);
      my_esp.send( parameter);
      my_esp.send("Host: ");
      strcpy_P(parameter, serverName);
      my_esp.send( parameter);
      my_esp.send("\r\n");
      strcpy_P(parameter, Str_Con);
      my_esp.send( parameter);
      my_esp.send(0x00); // Startkommando senden

      OK = false;
      /*
            if (my_esp.sendCommand(0, 500, "SEND OK")) {
              Serial.println(F("SEND OK"));
            }
      */
      if (my_esp.sendCommand(0, 2000, "200 OK")) {
        Serial.println(F("HTTP OK"));
        if (my_esp.sendCommand(0, 5000, "ok *")) {
          OK = true;

          Serial.print(my_esp.buffer);
          Serial.println(" ");
          Serial.flush();

          char *p_buf = strstr(my_esp.buffer, "ok *");  // search string start
          uint8_t pos = 0;
          *p_buf = 0; //Stringende
          do {
            p_buf --;    // search start of line
            pos++;
          }
          while ( (!(*p_buf == '\n')) && (pos < 20));

          int n;
          if (char *p_bf = strchr(p_buf, 'T') ) {
            n = atoi(p_bf + 1);  // Konvertiere str in int

            char x = *(p_buf + 1);
            if ((x > 0x2F) && (x < 0x3A)) { // first char a number?
              long l_tm = atol(p_buf);
              l_tm = l_tm - 946684800;  // EPOCH_TIME_OFFSET
              if (    Ds3231_ok == 1) {
                rtc.setDateTime(l_tm);
                Serial.println("Uhrzeit in DS3231 gesetzt.  ");
                display_time();
              }
            }
          }
          else {
            n = atoi(p_buf); // Konvertiere str in int
          }
          if (n > 0) {
            Serial.print("Intervallvorgabe vom Server:  ");
            Serial.print(n);
            Serial.println(" Minuten ");
          }
          else {
            Serial.print("Kein Intervall vom Server vorgegeben. ");
            Serial.println(my_esp.buffer);
            Serial.println(" ");
            Serial.println(n);
          }
          Serial.print("Vorgabe Sendezyklus vom Server:  ");
          uint8_t std = 0;
          if (strchr(p_buf, 'A') ) {
            std = 1; Serial.print(" eine ");
          }
          else if (strchr(p_buf, 'B') ) {
            std = 1; Serial.print(" zwei ");
          }
          else if (strchr(p_buf, 'C') ) {
            std = 1; Serial.print(" vier ");
          }
          else if (strchr(p_buf, 'D') ) {
            std = 1; Serial.print(" acht ");
          }
          if (std) {
            Serial.println(" Stunde(n)");
          } else {
            Serial.print(" keine ");
          }
          Serial.println(" ");
          Serial.println(F("\nÜbertragung erfolgreich"));
          Serial.println(" ");
        }
        else {
          Serial.println(F("Quittung Fehler!"));
          Serial.println(my_esp.buffer);
          OK = false;
        }
      }
      else {
        Serial.println(F("\nFehler in:"));
        strcpy_P(parameter, serverName);
        Serial.println(parameter);
        strcpy_P(parameter, pfad);
        Serial.println(parameter);
        Serial.flush();
      }
      if (my_esp.quit()) {
        Serial.println(F("."));
      }
      else {
        OK = false;
      }
      Serial.flush();
      delay(2000);
    } // prep_send

    if (OK) {
      // Test-Messdaten um 1 erhöhen
      TempIn++;
      TempOut++;
      TempOut++;
      FeuchteIn++;
      FeuchteOut++;
      FeuchteOut++;
      Licht++;
      Gewicht++;
      Batteriespannung++;
      Solarspannung++;
      Service = 1.0;
      TempOut = TempOut - 3.0;
      FeuchteIn = FeuchteIn - 2.0;

      // Abschaltung nach 5 Datenübertragungen
      if (TempIn >= 27.0) {
        OK = false;
        ESP_Abschaltung();
      }
      WLan_Pegel();
      Serial.println(F(" ... Senden in 5 Minuten"));
      Serial.flush();
      for (int i = 0; i < 300; i++) {
        delay(1000);
      }
    }
    else {
      Serial.println(F("Fehler im Test!"));
      ESP_Abschaltung();
    }
  } // if OK
}
//#########################################

#include <avr/sleep.h>
void ESP_Abschaltung() {
  Serial.println(F("Test beendet."));
  Serial.println(" ");
  my_esp.disConnect();
  my_esp.quit();
  delay(100);
  my_esp.end();
  Serial.end();
  digitalWrite (HX711_SCK, LOW); //HX711 aus
  digitalWrite(Power_Pin, LOW);  // Power aus
  digitalWrite(ESP_RESET, LOW);  // ESP8266 aus
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  cli();
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();
}
//----------------------------------------
void WLan_Info() {
  uint8_t st;
  Serial.println(F("WLAN Information:\n"));
  Serial.flush();
  st = my_esp.espState;
  my_esp.espState = ESP_CONNECTED;
  my_esp.send("AT+CWLAP\r\n");
  my_esp.check(10000);
  my_esp.espState = st;
  Serial.println("\n");
  Serial.flush();

  delay(1000);
}
//----------------------------------------
void WLan_Pegel() {
  strcpy_P(parameter, Access_Point);
  Serial.print(parameter);
  Serial.print(F("   Signalstaerke: "));
  Serial.flush();
  if (my_esp.signal(parameter)) {         // hole Signalstärke
    strcpy(parameter, my_esp.buffer);
    char cc = ',';
    char *pos = strchr(parameter, cc); // suche ','
    if (pos) {
      int svc = atoi(pos + 1);
      if ((svc > -10)) {
        Serial.println(F("\n###########################"));
        Serial.println(F("Signalstärke unklar: "));
      }
      if ((svc < -88)) {
        Serial.println(F("\n###########################"));
        Serial.println(F("Signal schwach: "));
      }
      Service = (float)svc;
      Serial.print(svc);
      Serial.println(F(" dBm\n"));
      Serial.flush();
    }
  }
  else {
    Serial.println(F("\n###########################"));
    Serial.println(F("Signalabfrage Fehler"));
  }
  Serial.flush();
}
//----------------------------------------------------------------
// Uhrzeit anzeigen
//----------------------------------------------------------------
void  display_time() {
  DateTime jetzt = rtc.now();
  if (jetzt.year() > 2100) {
    Serial.println(F("\nDS3231?"));
  }
  else {
    Serial.print(jetzt.date());
    Serial.print('.');
    Serial.print(jetzt.month());
    Serial.print('.');
    Serial.print(jetzt.year());
    uint8_t h = jetzt.hour();
    if (h < 10) {
      Serial.print("  ");
    }
    else Serial.print(" ");
    Serial.print(h);
    Serial.print(':');
    if (jetzt.minute() < 10) Serial.print("0");
    Serial.print(jetzt.minute());
    Serial.print(':');
    if (jetzt.second() < 10) Serial.print("0");
    Serial.print(jetzt.second());
  }
  Serial.println(" ");
  Serial.flush();
}
//----------------------------------------------------------------
