/*
* beelogger_STM32_EE.h
*  based on
*
* File:   at24cxx.h
* Author: RandallR
*
* Created on March 26, 2012, 06:52 PM
*
* // https://forum.arduino.cc/index.php?topic=90594.0
*
*
*/

#ifndef beelogger_STM32_EE_h
#define beelogger_STM32_EE_h


#define beelogger_STM32_EE_CTRL_ID_def 0x57 // default I2C address

class beelogger_STM32_EE
{
  // user-accessible "public" interface
  public:
    beelogger_STM32_EE(int addr_i2c = beelogger_STM32_EE_CTRL_ID_def);
    void end(void);
	
	uint8_t isPresent(void);      // check if the device is present
    int ReadMem(int iAddr, char Buf[], int iCnt);
    uint8_t WriteMem(int iAddr, uint8_t iVal);
    uint8_t WriteMem(int iAddr, const char *pBuf, int iCnt);

    int     ReadStr(int iAddr, char Buf[], int iBufLen);
    uint8_t WriteStr(int iAddr, const char *pBuf);

  private:
    int _i2c_addr;
};
#endif
