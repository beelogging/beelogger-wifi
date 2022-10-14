/*
   (C) 2022 R.Schick - beelogger.de

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


//--------------------------------------------------------------
#include <LowPower.h>
void setup(void) {

#define Power_Pin 4
#define ESP_Power_Pin A2

  Serial.begin(115200);
  while (!Serial) {};
  Serial.println(F("Sleep 115200\n\r"));
  Serial.flush();
  Serial.end();
  delay(100);
  Serial.begin(9600);
  while (!Serial) {};
  Serial.println(F("Sleep 9600\n\r"));
  Serial.flush();
  Serial.end();

  digitalWrite(Power_Pin, HIGH);
  pinMode(Power_Pin, OUTPUT);
  delay(5);
  digitalWrite(ESP_Power_Pin, HIGH);
  pinMode(ESP_Power_Pin, OUTPUT);
  delay(5);

  digitalWrite(1, LOW);
  pinMode(1, INPUT);
}
//--------------------------------------------------------------
void loop(void) {
  // nothing to see here
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
//--------------------------------------------------------------
