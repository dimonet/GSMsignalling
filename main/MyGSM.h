#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte pinBOOT);
    void Initialize();
    void ReInitialize();
    bool Available();
    bool NewRing;
    bool NewSms;    
    String RingNumber;
    String SmsNumber;
    String SmsText;
    String Read();
    void SendSMS(String *text, String phone);
    void Call(String phone);
    void RejectCall();    
    void RequestGsmCode(String code);  // запрос gsm кода (*#)
    void Refresh();    
    
  private:
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    String GetPhoneNumber(String str);
    int _gsmLED;
    int _pinBOOT;                             // нога BOOT или K на модеме       
};
