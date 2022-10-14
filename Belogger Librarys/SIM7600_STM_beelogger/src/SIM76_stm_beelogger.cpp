/*************************************************************************
* SIM7600E TCPIP Library
* based on SIM800 GPRS/HTTP Library
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
* For more information, please visit http://arduinodev.com
*
* Modified for use with SIM7600E
*
* fixed reading signal level
* Version 1.0.1  10102021 R.Schick
*    wait til port ready
*
* Version 1.0.2 01122021 R.Schick
*    add delay when purging data
*
* Version 1.0.3 21022022 R.Schick
*         CGPADDR response  
*
*************************************************************************/


#include "SIM76_STM_beelogger.h"

// the software serial interface														
// Serial2
HardwareSerial SIM_SERIAL(USART2);   // or HardWareSerial Serial2 (PA3, PA2);


const char SIM7600_UP[] PROGMEM  	 = "PB DONE";	// SIM7600 up and ready
const char SIM7600_CGS[] PROGMEM  	 = "AT+CGSOCKCONT=1,\"IP\",\"";	// open IP-Socket
const char SIM7600_ADR[] PROGMEM  	 = "AT+CGPADDR";				// check local IP-Adress
const char SIM7600_MODE[] PROGMEM  	 = "AT+CCHMODE=1";				// select Mode Transparent
const char SIM7600_SET[] PROGMEM  	 = "AT+CCHSET=1";				// set Mode
const char SIM7600_START[] PROGMEM   = "AT+CCHSTART";				// Start SSL
const char SIM7600_CFG[] PROGMEM  	 = "AT+CCHSSLCFG=0,0";			// Set SSL Config to session
const char SIM7600_OPEN[] PROGMEM  	 = "AT+CCHOPEN=0,\"";			// Open Session to server
const char SIM7600_CON[] PROGMEM  	 = "CONNECT 96";				// check Connect 9600
const char SIM7600_CLOSE[] PROGMEM   = "AT+CCHCLOSE=0";				// Close Session
const char SIM7600_STOP[] PROGMEM  	 = "AT+CCHSTOP";				// Stop SSL
const char SIM7600_NETSTA[] PROGMEM  = "AT+CNETSTART";
const char SIM7600_NETSTO[] PROGMEM  = "AT+CNETSTOP";

/**************************************************************************/
/*!
    @brief   setup Serial and check SIM7600E booting up
*/
/**************************************************************************/
bool CLTE_SIM76_STM::init(unsigned int timeout)
{
	#define try_count 8
	char buffer[16];
	SIM_SERIAL.begin(9600);
	while (!SIM_SERIAL); // Wait for serial port open
    delay(250);
	
#if DEB_data
	Serial.println("SSL 10-10-21");
	Serial.flush();
#endif

	//pinMode(GSM_RX, INPUT); // Turn pullup off 	pinMode(INPUT_CAPTURE_PIN, INPUT_PULLUP);

	strcpy_P(buffer, SIM7600_UP);
	int i =0;
	do { 
		if (sendCommand(0,6000,buffer)) {
			break;
		}
		i++;
	} 
	while (i < try_count);
	if(i == try_count) return(false);
#if DEB_data
	Serial.println(F("SIM7600 is up"));
	Serial.flush();
#endif
	uint32_t t = millis();
	do {
		if (sendCommand("AT")) {
			sendCommand("ATE0");
			sendCommand("AT+CFUN=1", 10000);
			return true;
		}
	}
	while (millis() - t < timeout);
	return false;
}

/**************************************************************************/
/*!
    @brief    turn off services and end Serial 
*/
/**************************************************************************/
void CLTE_SIM76_STM::shutdown()
{
	sendCommand("AT+CFUN=0", 10000);
	SIM_SERIAL.end();  // stop listening
}

/**************************************************************************/
/*!
    @brief   check network and connect to APN
*/
/**************************************************************************/
uint8_t CLTE_SIM76_STM::start(const char* apn,const char* usr,const char* pw)
{
	bool success = false;
	uint8_t n = 0;
	do {
		if (sendCommand("AT+CREG?", 2000)) {  // are we registerd to a gsm-network
			char *p = strstr(buffer, "0,");
			if (p) {
				char mode = *(p + 2);
#if DEB_more
			Serial.print(F("Mode:"));
			Serial.println(mode);
			Serial.flush();
#endif
				if (mode == '1' || mode == '5') {  // net or roaming
					success = true;
				}
			}
		}
	delay(3000);
	n++;
	} while ((n < 30) && (success == false));

	if (!success) return 1;  // not registered to network

	n = 0;
	do {
		if (sendCommand("AT+CGREG?", 2000,"0,1")) {  // are we registerd to a gsm-network
			success = true;
		}
		delay(1000);
		n++;
	} while ((n < 5) && (success == false));

	if (!success) return 2;  // not registered to gprs network

//Configure Context with APN
	char buf[64];
	strcpy_P(buf, SIM7600_CGS);
	strcat(buf, apn);
	strcat(buf, "\"");
	if (sendCommand(buf, 10000)) {
		delay(1000);  // wait on SIM7600 to get ready
		if(strlen(usr) > 0){
			strcpy(buf, "AT+CGAUTH=1,3,\"");
			strcat(buf,usr);
			strcat(buf,"\",\"");
			strcat(buf,pw);
			strcat(buf,"\"");
			sendCommand(buf, 5000);
			delay (500);
		}
		strcpy_P(buf, SIM7600_ADR);
		n = 0;
		do {
			if (sendCommand(buf, 5000, ".")) {  // +CGPADDR: 1,0.0.0.0, are we done internally?
				char *s = strstr(buffer, "1,");
				if (s) {
					char ip_ = *(s + 2);
#if DEB_data
					Serial.print(buffer);
					sendCommand(0, 2000, ",");
					Serial.println(buffer);
					Serial.flush();
#endif
					if(	ip_ != '0'){  // ip adress starts with 1...9
					return(0);
					}
				}
			}
			delay(1000);
			n++;
		} while (n < 5);
		return(4);  // no IP Adress
	}
	return(3);  // no APN
}


/**************************************************************************/
/*!
    @brief  SSL functions, transparent Mode
*/
/**************************************************************************/
uint8_t CLTE_SIM76_STM::ssl_init()  //prepare SSL, set transparent mode

{
	char buf[16];
	strcpy_P(buf,SIM7600_MODE);
	if(sendCommand(buf, 3000)){  // 1 = transparent Mode
		delay(500);  // give SIM7600 some time to proceed
		strcpy_P(buf,SIM7600_SET);
		if(sendCommand(buf, 3000)){
			delay(500);  // give SIM7600 some time to proceed
			return(1);
		}
	}
	return(0);
}
//start SSL, activate PDP context
uint8_t CLTE_SIM76_STM::ssl_start()
{
	char buf[20];
	strcpy_P(buf,SIM7600_START);
	if(sendCommand(buf, 3000)){
		delay(500);  // give SIM7600 some time to proceed
		strcpy_P(buf,SIM7600_CFG);
		if(sendCommand(buf, 3000)){
			delay(500);  // give SIM7600 some time to proceed
			return(1);
		}
	}
	return(0);
}
//stop SSL
uint8_t CLTE_SIM76_STM::ssl_stop()
{
	char buf[16];
	strcpy_P(buf,SIM7600_STOP);
	if(sendCommand(buf, 3000)){
		delay(500);  // give SIM7600 some time to proceed
		return(1);
	}
	return(0);
}
//connect SSL
uint8_t CLTE_SIM76_STM::ssl_open(const char* url)
{
	char buf[80];  // AT+CCHOPEN=0," server_name ",443,2
	char cmp[16];
	strcpy_P(cmp,SIM7600_CON);  // CONNECT
	strcpy_P(buf,SIM7600_OPEN);
	strcat(buf, url);
	strcat(buf,"\",443,2");
	
	if(sendCommand(buf,10000,cmp)){  // check CONNECT
		// Check CONNECT "FAIL" 
		//strcpy(cmp, "FAIL");
		//if(sendCommand(0,1000,cmp) == 0){
			return(1);  // FAIL not found -> success
		//}
	}
	return(0);
}
//close connection
uint8_t CLTE_SIM76_STM::ssl_close()
{
	char buf[16];
	strcpy_P(buf,SIM7600_CLOSE);
	if(sendCommand(buf, 500)){
		return(1);
	}
	return(0);
}

//write data to module GET / ...
void CLTE_SIM76_STM::ssl_data(const char* data)
{
#if DEB_data
    Serial.println(data);
    Serial.flush();
#endif
    SIM_SERIAL.print(data);
    SIM_SERIAL.flush();
}


/**************************************************************************/
/*!
    @brief   get signal quality on the given entwork
*/
/**************************************************************************/
int CLTE_SIM76_STM::getSignalQuality()
{
  char buf[8] = "AT+CSQ";
  sendCommand(buf,10000,","); // suche nach Komma zw. rssi und ber
  char *p = strstr(buffer, "CSQ: ");
  if (p) {
    int n = atoi(p + 5);
    if (n == 99 || n == -1) return -1;
    return n * 2 - 114;
  } 
  return -2; 
}
/**************************************************************************/
/*!
    @brief   get location data of the base station connected to
*/
/**************************************************************************/
// Location command:
//AT+CLBS=4
//OK
//+CLBS: 0,31.228525,121.380295,500,2025/06/07,10:49:08
//
bool CLTE_SIM76_STM::getLocation(GSM_LOCATION* loc)
{
	bool x_val = false;	
	strcpy_P(buffer,SIM7600_NETSTA);
	if (sendCommand(buffer, 15000)) {
		delay(1000);  // wait for SIM7600 to get ready
		if (sendCommand("AT+CLBS=4", 15000,"+CLBS")){
			uint32_t t = millis();
			uint8_t n = 0;
			do { // read til EOL
				if (SIM_SERIAL.available()) {
					char c = SIM_SERIAL.read();			
					buffer[n++] = c;
					if (c == '\r') {break;}  // EOL
				}
			} while ((n < 50) && (millis() - t < 3000));
			buffer[n] = 0;
			char *p = buffer;
			do {
				if (!(p = strchr(p, ','))) break;
				loc->lon = atof(++p);
				if (!(p = strchr(p, ','))) break;
				loc->lat = atof(++p);
			/*
			if (!(p = strchr(p, ','))) break;									
			loc->year = atoi(++p) - 2000;
			if (!(p = strchr(p, '/'))) break;
			loc->month = atoi(++p);
			if (!(p = strchr(p, '/'))) break;
			loc->day = atoi(++p);
			if (!(p = strchr(p, ','))) break;
			loc->hour = atoi(++p);
			if (!(p = strchr(p, ':'))) break;
			loc->minute = atoi(++p);
			if (!(p = strchr(p, ':'))) break;
			loc->second = atoi(++p);
			*/
				x_val = true;
				break;
			} while(0);
		}
#if DEB_data
			Serial.print(buffer);
			Serial.flush();
#endif
		strcpy_P(buffer,SIM7600_NETSTO);
		sendCommand(buffer, 5000);
	}	
	return x_val;
}

/**************************************************************************/
/*!
    @brief    Utility functions
*/
/**************************************************************************/
uint8_t CLTE_SIM76_STM::sendCommand(const char* cmd, unsigned int timeout, const char* expected)
{
  if (cmd != 0) {
#if DEB_cmd
    Serial.print(F("**"));
    Serial.println(cmd);
    Serial.flush();
#endif
    purgeSerial();
    SIM_SERIAL.println(cmd);
  }
  uint32_t t = millis();
  uint8_t n = 0;
  char c;
  do {
    if (SIM_SERIAL.available()) {
      c = SIM_SERIAL.read();
      if (n >= sizeof(buffer) - 1) { // buffer full, discard first half
        n = sizeof(buffer) / 2 - 1;
        memcpy(buffer, buffer + sizeof(buffer) / 2, n);
      }
      buffer[n++] = c;
      buffer[n] = 0;
      if (strstr(buffer, expected ? expected : "OK\r")) {
       return n;
      }
#if DEB_more
      Serial.print(c);
      Serial.flush();
#endif
  	  //if( c == '\r') n = 0; // do not use here, causes major trouble
    }
  } while (millis() - t < timeout);
#if DEB_more
   Serial.print(F("Timeout"));
   Serial.println(buffer);
   Serial.flush();
#endif
  return 0;
}

//#####################################
void CLTE_SIM76_STM::purgeSerial()
{
  while (SIM_SERIAL.available()){ SIM_SERIAL.read();delay(2);}
}

/*
// receive data from modul debug only
void CLTE_SIM76_STM::recv()
{
	uint32_t t = millis();
	uint8_t n = 0;
	do {
		if (SIM_SERIAL.available()) {      char c = SIM_SERIAL.read();
			if (n >= sizeof(buffer) - 1) { // buffer full, discard first half
				n = sizeof(buffer) / 2 - 1;
				memcpy(buffer, buffer + sizeof(buffer) / 2, n);
			}
			buffer[n++] = c;
			buffer[n] = 0;
#if DEB_data
			Serial.print(c);
			Serial.flush();
#endif
		}
	} while ((millis() - t < 8000));
}
*/