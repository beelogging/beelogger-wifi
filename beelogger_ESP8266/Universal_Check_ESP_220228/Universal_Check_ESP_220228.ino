/*
   (C) 2020 R.Schick - beelogger.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// beelogger.de - Arduino Datenlogger für Imker
// Erläuterungen dieses Programmcodes unter http://beelogger.de


//----------------------------------------------------------------
// Konfiguration
//----------------------------------------------------------------

// nothing


//----------------------------------------------------------------
// Ende Konfiguration
//----------------------------------------------------------------


char ID[] = "ESP Check 28.02.2022";

#include <AltSoftSerial.h>
AltSoftSerial altSerial;


#define Test_Baudrate 9600    //  Baudrate für die serielle Kommunikation/Monitor

#define ESP_RESET A2
#define POWER 4

void setup() {

  pinMode(POWER, OUTPUT);
  pinMode(ESP_RESET , OUTPUT);

  esp_test();

}

void loop() {
  delay(30000);
  digitalWrite(ESP_RESET , LOW);
  digitalWrite(POWER, LOW);
}



// check communication, read firmware version, set Default-Mode, do some tests
void esp_test() {
  digitalWrite(POWER, HIGH);
  digitalWrite(ESP_RESET , HIGH);
  delay(500);

  Serial.begin(Test_Baudrate);
  while (!Serial) ; // wait for Arduino Serial Monitor to open
  Serial.println(" ");
  Serial.println(ID);
  Serial.println(" ");
  Serial.println(F("ESP Test Begin"));
  Serial.print(F("Baudrate: "));
  Serial.println(Test_Baudrate);
  Serial.println(F("\nSende Abfrage Firmware/Version an ESP8266: 'AT+GMR' "));
  Serial.flush();
  delay(1000);
  altSerial.begin(Test_Baudrate);
  altSerial.println("AT");
  read_ser(200);
  altSerial.println("AT+GMR");
  read_ser(1000);

  Serial.println(F("Setze Betriebsart 'Station' in ESP8266"));
  Serial.flush();
  delay(50);
  altSerial.println("AT+CWMODE_DEF=1");
  read_ser(2000);

  Serial.println(F("\n\nSende suche Access Points an ESP8266: 'AT+CWLAP'"));
  Serial.flush();
  delay(50);
  altSerial.println("AT+CWLAP");
  read_ser(8000);
  altSerial.println("AT+CWLAPOPT=1,6");
  read_ser(2000);
  altSerial.println("AT+CWLAP");
  read_ser(8000);
  altSerial.end();
  Serial.println("\n ... end ...");
  Serial.flush();
  Serial.end();
  digitalWrite(POWER, LOW);
}


void read_ser(unsigned long timeout) {
  unsigned long t = millis();
  char c;
  uint8_t f = 0;
  do {
    if (Serial.available()) {
      c = Serial.read();
      altSerial.print(c);
    }
    if (altSerial.available()) {
      c = altSerial.read();
      Serial.print(c);
      if (f == 0) {
        if (c == 'O') f = 1;
      }
      else if (f == 1) {
        if (c == 'K') f = 2;
        else f = 0;
      }
      else if (f == 2) {
        if (c == '\r') break;
        else f = 0;
      }
    }
  } while ((millis() - t) < timeout);
}
